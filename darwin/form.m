// 7 june 2016
#import "uipriv_darwin.h"

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
		[self setRowSpacing:0];
		[self setColumnSpacing:0];
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
	BOOL hasVerticalExpansion;

	if ([self->stretchyConstraints count] != 0) {
		[self removeConstraints:self->stretchyConstraints];
		[self->stretchyConstraints removeAllObjects];
	}
	firstStretchy = nil;
	labelColumnWidth = 0;
	hasVisibleRows = NO;
	rowIndex = 0;
	for (fc in self->children) {
		NSGridRow *row;
		NSSize labelSize;

		row = [self rowAtIndex:rowIndex++];
		[row setHidden:!uiControlVisible(fc.c)];
		if (!uiControlVisible(fc.c))
			continue;
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

	[self->children addObject:fc];
	row = [self addRowWithViews:[NSArray arrayWithObjects:fc.label, [fc view], nil]];
	if ([self numberOfRows] == 1) {
		[[self columnAtIndex:0] setXPlacement:NSGridCellPlacementTrailing];
		[[self columnAtIndex:1] setXPlacement:NSGridCellPlacementFill];
	}
	if ([[fc view] isKindOfClass:[NSScrollView class]]) {
		[[row cellAtIndex:0] setYPlacement:NSGridCellPlacementTop];
		[[row cellAtIndex:1] setYPlacement:NSGridCellPlacementFill];
	} else
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
	[self setRowSpacing:self->padded ? self->nativeRowSpacing : 0];
	[self setColumnSpacing:self->padded ? self->nativeColumnSpacing : 0];
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
