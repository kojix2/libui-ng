// 7 june 2016
#import "uipriv_darwin.h"

// An ordinary NSView has no intrinsic height. Give the trailing spacer an
// explicit zero natural height so it does not increase the Form's fitting size.
@interface formSpacerView : NSView
@end

@implementation formSpacerView

- (NSSize)intrinsicContentSize
{
	return NSZeroSize;
}

@end

@interface formChild : NSObject
@property uiControl *c;
@property (strong) NSTextField *label;
@property BOOL stretchy;
@property NSLayoutPriority oldHorzHuggingPri;
@property NSLayoutPriority oldVertHuggingPri;
- (NSView *)view;
@end

@interface formView : NSGridView {
	uiForm *f;
	NSMutableArray *children;
	NSMutableArray *stretchyConstraints;
	formSpacerView *spacer;
	BOOL padded;
	BOOL verticalExpansion;
	CGFloat nativeRowSpacing;
	CGFloat nativeColumnSpacing;
}
- (id)initWithF:(uiForm *)ff;
- (void)onDestroy;
- (void)syncEnableStates:(int)enabled;
- (void)updateRows;
- (void)append:(NSString *)label c:(uiControl *)c stretchy:(int)stretchy;
- (void)delete:(int)n;
- (int)numChildren;
- (int)isPadded;
- (void)setPadded:(int)p;
- (BOOL)hugsTrailing;
- (BOOL)hugsBottom;
@end

struct uiForm {
	uiDarwinControl c;
	formView *view;
};

@implementation formChild

- (void)dealloc
{
	[self.label release];
	[super dealloc];
}

- (NSView *)view
{
	return (NSView *) uiControlHandle(self.c);
}

@end

@implementation formView

- (id)initWithF:(uiForm *)ff
{
	self = [super initWithFrame:NSZeroRect];
	if (self != nil) {
		self->f = ff;
		self->children = [NSMutableArray new];
		self->stretchyConstraints = [NSMutableArray new];
		self->padded = NO;
		self->verticalExpansion = NO;
		self->nativeRowSpacing = [self rowSpacing];
		self->nativeColumnSpacing = [self columnSpacing];
		// NSGridView's native spacing can be smaller than the spacing needed
		// between libui controls (notably adjacent text fields). uiForm's
		// padded setting promises OS-appropriate space between controls, so
		// preserve larger native values while enforcing the established Darwin
		// padding as the minimum.
		self->nativeRowSpacing = MAX(self->nativeRowSpacing,
			uiDarwinPaddingAmount(NULL));
		self->nativeColumnSpacing = MAX(self->nativeColumnSpacing,
			uiDarwinPaddingAmount(NULL));
		// rowSpacing would also insert a gap before the trailing spacer row.
		// Preserve AppKit's native value and apply it only between visible
		// content rows as topPadding in updateRows.
		[self setRowSpacing:0];
		[self setColumnSpacing:0];
		self->spacer = [formSpacerView new];
		[self->spacer setContentHuggingPriority:NSLayoutPriorityRequired
			forOrientation:NSLayoutConstraintOrientationVertical];
		// NSGridView otherwise distributes spare height among ordinary rows.
		// When no row is stretchy, this trailing row receives that height so the
		// content stays top-aligned. This deliberately avoids caching fittingSize:
		// a descendant container can change its intrinsic height at any time.
		[self addRowWithViews:[NSArray arrayWithObjects:
			[NSGridCell emptyContentView], self->spacer, nil]];
		[[self columnAtIndex:0] setXPlacement:NSGridCellPlacementTrailing];
		[[self columnAtIndex:1] setXPlacement:NSGridCellPlacementFill];
	}
	return self;
}

- (void)onDestroy
{
	formChild *fc;

	if ([self->stretchyConstraints count] != 0)
		[self removeConstraints:self->stretchyConstraints];
	[self->stretchyConstraints release];
	while ([self numberOfRows] != 0)
		[self removeRowAtIndex:0];
	for (fc in self->children) {
		uiControlSetParent(fc.c, NULL);
		uiDarwinControlSetSuperview(uiDarwinControl(fc.c), nil);
		uiControlDestroy(fc.c);
	}
	[self->children release];
	[self->spacer release];
}

- (void)syncEnableStates:(int)enabled
{
	formChild *fc;

	for (fc in self->children)
		uiDarwinControlSyncEnableState(uiDarwinControl(fc.c), enabled);
}

- (void)updateRows
{
	formChild *fc;
	CGFloat labelColumnWidth;
	NSView *firstStretchy;
	NSLayoutConstraint *constraint;
	NSInteger rowIndex;
	BOOL hasVisibleRows;
	BOOL hasPreviousVisibleRow;
	BOOL hasVerticalExpansion;

	if ([self->stretchyConstraints count] != 0) {
		[self removeConstraints:self->stretchyConstraints];
		[self->stretchyConstraints removeAllObjects];
	}
	firstStretchy = nil;
	labelColumnWidth = 0;
	hasVisibleRows = NO;
	hasPreviousVisibleRow = NO;
	rowIndex = 0;
	for (fc in self->children) {
		NSGridRow *row;
		NSSize labelSize;

		row = [self rowAtIndex:rowIndex++];
		[row setHidden:!uiControlVisible(fc.c)];
		if (!uiControlVisible(fc.c))
			continue;
		// Padding belongs between visible rows, never above the first one.
		// Tracking visibility here also avoids gaps left by hidden rows.
		[row setTopPadding:self->padded && hasPreviousVisibleRow ?
			self->nativeRowSpacing : 0];
		hasPreviousVisibleRow = YES;
		hasVisibleRows = YES;
		labelSize = [fc.label intrinsicContentSize];
		if (labelColumnWidth < labelSize.width)
			labelColumnWidth = labelSize.width;
		if (!fc.stretchy)
			continue;
		if (firstStretchy == nil) {
			firstStretchy = [fc view];
			continue;
		}
		constraint = uiprivMkConstraint([fc view], NSLayoutAttributeHeight,
			NSLayoutRelationEqual,
			firstStretchy, NSLayoutAttributeHeight,
			1, 0,
			@"uiForm stretchy constraint");
		[self addConstraint:constraint];
		[self->stretchyConstraints addObject:constraint];
	}
	// NSGridView can assign spare width to the label column when another
	// column contains a view without an intrinsic width, such as NSScrollView.
	// Keep labels content-sized so the control column receives the spare width.
	if ([self numberOfColumns] != 0)
		[[self columnAtIndex:0] setWidth:hasVisibleRows ?
			labelColumnWidth : NSGridViewSizeForContent];
	hasVerticalExpansion = firstStretchy != nil;
	// A stretchy child, rather than the spacer, must receive spare height.
	// Equal-height constraints above make multiple stretchy rows share it.
	[[self rowAtIndex:[self->children count]] setHidden:hasVerticalExpansion];
	if (hasVerticalExpansion != self->verticalExpansion) {
		self->verticalExpansion = hasVerticalExpansion;
		uiDarwinNotifyEdgeHuggingChanged(uiDarwinControl(self->f));
	}
}

- (void)append:(NSString *)label c:(uiControl *)c stretchy:(int)stretchy
{
	formChild *fc;
	NSTextField *labelView;
	NSLayoutPriority priority;
	NSGridRow *row;

	fc = [formChild new];
	fc.c = c;
	fc.stretchy = stretchy != 0;
	labelView = uiprivNewLabel(label);
	fc.label = labelView;
	[labelView release];
	[fc.label setTranslatesAutoresizingMaskIntoConstraints:NO];
	[fc.label setContentHuggingPriority:NSLayoutPriorityRequired forOrientation:NSLayoutConstraintOrientationHorizontal];
	[fc.label setContentHuggingPriority:NSLayoutPriorityRequired forOrientation:NSLayoutConstraintOrientationVertical];
	[fc.label setContentCompressionResistancePriority:NSLayoutPriorityRequired forOrientation:NSLayoutConstraintOrientationHorizontal];
	[fc.label setContentCompressionResistancePriority:NSLayoutPriorityRequired forOrientation:NSLayoutConstraintOrientationVertical];

	fc.oldHorzHuggingPri = uiDarwinControlHuggingPriority(uiDarwinControl(fc.c), NSLayoutConstraintOrientationHorizontal);
	fc.oldVertHuggingPri = uiDarwinControlHuggingPriority(uiDarwinControl(fc.c), NSLayoutConstraintOrientationVertical);

	uiControlSetParent(fc.c, uiControl(self->f));
	uiDarwinControlSetSuperview(uiDarwinControl(fc.c), self);
	uiDarwinControlSyncEnableState(uiDarwinControl(fc.c), uiControlEnabledToUser(uiControl(self->f)));

	// Stretchy controls divide additional height equally; all other controls
	// keep their intrinsic height.
	priority = fc.stretchy ? NSLayoutPriorityDefaultLow : NSLayoutPriorityRequired;
	uiDarwinControlSetHuggingPriority(uiDarwinControl(fc.c), priority, NSLayoutConstraintOrientationVertical);
	uiDarwinControlSetHuggingPriority(uiDarwinControl(fc.c), NSLayoutPriorityDefaultLow, NSLayoutConstraintOrientationHorizontal);

	row = [self insertRowAtIndex:[self->children count]
		withViews:[NSArray arrayWithObjects:fc.label, [fc view], nil]];
	[self->children addObject:fc];
	// Text-bearing controls use their baseline. Controls whose visual axis is
	// horizontal use center alignment, while tall or baseline-less composites
	// use top alignment. Treating every AppKit view as baseline-bearing can
	// move a control down by much of its height (notably NSColorWell).
	if ([[fc view] isKindOfClass:[NSScrollView class]]) {
		[[row cellAtIndex:0] setYPlacement:NSGridCellPlacementTop];
		[[row cellAtIndex:1] setYPlacement:NSGridCellPlacementFill];
	} else if ([[fc view] isKindOfClass:[NSSlider class]] ||
		[[fc view] isKindOfClass:[NSProgressIndicator class]] ||
		[[fc view] isKindOfClass:[NSColorWell class]])
		[row setYPlacement:NSGridCellPlacementCenter];
	else if ([[fc view] firstBaselineOffsetFromTop] == 0)
		[row setYPlacement:NSGridCellPlacementTop];
	else
		[row setRowAlignment:NSGridRowAlignmentFirstBaseline];
	[self updateRows];
	[fc release];
}

- (void)delete:(int)n
{
	formChild *fc;

	fc = (formChild *) [self->children objectAtIndex:n];

	if ([self->stretchyConstraints count] != 0) {
		[self removeConstraints:self->stretchyConstraints];
		[self->stretchyConstraints removeAllObjects];
	}
	[self removeRowAtIndex:n];
	uiControlSetParent(fc.c, NULL);
	uiDarwinControlSetSuperview(uiDarwinControl(fc.c), nil);
	uiDarwinControlSetHuggingPriority(uiDarwinControl(fc.c), fc.oldHorzHuggingPri, NSLayoutConstraintOrientationHorizontal);
	uiDarwinControlSetHuggingPriority(uiDarwinControl(fc.c), fc.oldVertHuggingPri, NSLayoutConstraintOrientationVertical);
	[self->children removeObjectAtIndex:n];

	[self updateRows];
}

- (int)numChildren
{
	return (int) [self->children count];
}

- (int)isPadded
{
	return self->padded;
}

- (void)setPadded:(int)p
{
	self->padded = p != 0;
	[self setColumnSpacing:self->padded ? self->nativeColumnSpacing : 0];
	[self updateRows];
}

- (BOOL)hugsTrailing
{
	return YES;
}

- (BOOL)hugsBottom
{
	return self->verticalExpansion;
}

@end

static void uiFormDestroy(uiControl *c)
{
	uiForm *f = uiForm(c);

	[f->view onDestroy];
	[f->view release];
	uiFreeControl(uiControl(f));
}

uiDarwinControlDefaultHandle(uiForm, view)
uiDarwinControlDefaultParent(uiForm, view)
uiDarwinControlDefaultSetParent(uiForm, view)
uiDarwinControlDefaultToplevel(uiForm, view)
uiDarwinControlDefaultVisible(uiForm, view)
uiDarwinControlDefaultShow(uiForm, view)
uiDarwinControlDefaultHide(uiForm, view)
uiDarwinControlDefaultEnabled(uiForm, view)
uiDarwinControlDefaultEnable(uiForm, view)
uiDarwinControlDefaultDisable(uiForm, view)

static void uiFormSyncEnableState(uiDarwinControl *c, int enabled)
{
	uiForm *f = uiForm(c);

	if (uiDarwinShouldStopSyncEnableState(uiDarwinControl(f), enabled))
		return;
	[f->view syncEnableStates:enabled];
}

uiDarwinControlDefaultSetSuperview(uiForm, view)

static BOOL uiFormHugsTrailingEdge(uiDarwinControl *c)
{
	uiForm *f = uiForm(c);

	return [f->view hugsTrailing];
}

static BOOL uiFormHugsBottom(uiDarwinControl *c)
{
	uiForm *f = uiForm(c);

	return [f->view hugsBottom];
}

static void uiFormChildEdgeHuggingChanged(uiDarwinControl *c)
{
	uiForm *f = uiForm(c);

	[f->view updateRows];
}

uiDarwinControlDefaultHuggingPriority(uiForm, view)
uiDarwinControlDefaultSetHuggingPriority(uiForm, view)

static void uiFormChildVisibilityChanged(uiDarwinControl *c)
{
	uiForm *f = uiForm(c);

	[f->view updateRows];
}

void uiFormAppend(uiForm *f, const char *label, uiControl *c, int stretchy)
{
	// LONGTERM on other platforms
	// or at leat allow this and implicitly turn it into a spacer
	if (c == NULL)
		uiprivUserBug("You cannot add NULL to a uiForm.");
	[f->view append:uiprivToNSString(label) c:c stretchy:stretchy];
}

void uiFormDelete(uiForm *f, int n)
{
	[f->view delete:n];
}

int uiFormNumChildren(uiForm *f)
{
	return [f->view numChildren];
}

int uiFormPadded(uiForm *f)
{
	return [f->view isPadded];
}

void uiFormSetPadded(uiForm *f, int padded)
{
	[f->view setPadded:padded];
}

uiForm *uiNewForm(void)
{
	uiForm *f;

	uiDarwinNewControl(uiForm, f);

	f->view = [[formView alloc] initWithF:f];

	return f;
}
