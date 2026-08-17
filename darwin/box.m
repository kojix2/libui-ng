// 15 august 2015
#import "uipriv_darwin.h"

@interface boxChild : NSObject
@property uiControl *c;
@property BOOL stretchy;
@property NSLayoutPriority oldPrimaryHuggingPri;
@property NSLayoutPriority oldSecondaryHuggingPri;
@property (strong) NSLayoutConstraint *secondaryConstraint;
- (NSView *)view;
@end

@interface boxView : NSStackView {
	uiBox *b;
	NSMutableArray *children;
	NSMutableArray *stretchyConstraints;
	BOOL vertical;
	BOOL padded;
	BOOL primaryExpansion;

	NSLayoutAttribute primarySize;
	NSLayoutAttribute secondarySize;
	NSLayoutConstraintOrientation primaryOrientation;
	NSLayoutConstraintOrientation secondaryOrientation;
}
- (id)initWithVertical:(BOOL)vert b:(uiBox *)bb;
- (void)onDestroy;
- (void)syncEnableStates:(int)enabled;
- (void)updateLayout;
- (void)append:(uiControl *)c stretchy:(int)stretchy;
- (void)delete:(int)n;
- (int)numChildren;
- (int)isPadded;
- (void)setPadded:(int)p;
- (BOOL)hugsTrailing;
- (BOOL)hugsBottom;
@end

struct uiBox {
	uiDarwinControl c;
	boxView *view;
};

@implementation boxChild

- (void)dealloc
{
	[self.secondaryConstraint release];
	[super dealloc];
}

- (NSView *)view
{
	return (NSView *) uiControlHandle(self.c);
}

@end

@implementation boxView

- (id)initWithVertical:(BOOL)vert b:(uiBox *)bb
{
	self = [super initWithFrame:NSZeroRect];
	if (self != nil) {
		self->b = bb;
		self->vertical = vert;
		self->padded = NO;
		self->primaryExpansion = NO;
		self->children = [NSMutableArray new];
		self->stretchyConstraints = [NSMutableArray new];
		[self setDistribution:NSStackViewDistributionFill];
		[self setDetachesHiddenViews:YES];
		[self setSpacing:0];

		if (self->vertical) {
			[self setOrientation:NSUserInterfaceLayoutOrientationVertical];
			[self setAlignment:NSLayoutAttributeLeading];
			self->primarySize = NSLayoutAttributeHeight;
			self->secondarySize = NSLayoutAttributeWidth;
			self->primaryOrientation = NSLayoutConstraintOrientationVertical;
			self->secondaryOrientation = NSLayoutConstraintOrientationHorizontal;
		} else {
			[self setOrientation:NSUserInterfaceLayoutOrientationHorizontal];
			[self setAlignment:NSLayoutAttributeTop];
			self->primarySize = NSLayoutAttributeWidth;
			self->secondarySize = NSLayoutAttributeHeight;
			self->primaryOrientation = NSLayoutConstraintOrientationHorizontal;
			self->secondaryOrientation = NSLayoutConstraintOrientationVertical;
		}
	}
	return self;
}

- (void)onDestroy
{
	boxChild *bc;

	if ([self->stretchyConstraints count] != 0)
		[self removeConstraints:self->stretchyConstraints];
	[self->stretchyConstraints release];
	for (bc in self->children) {
		[self removeConstraint:bc.secondaryConstraint];
		bc.secondaryConstraint = nil;
		uiControlSetParent(bc.c, NULL);
		uiDarwinControlSetSuperview(uiDarwinControl(bc.c), nil);
		uiControlDestroy(bc.c);
	}
	[self->children release];
}

- (void)syncEnableStates:(int)enabled
{
	boxChild *bc;

	for (bc in self->children)
		uiDarwinControlSyncEnableState(uiDarwinControl(bc.c), enabled);
}

- (void)updateLayout
{
	boxChild *bc;
	NSView *firstStretchy;
	NSLayoutConstraint *constraint;
	BOOL hasPrimaryExpansion;

	if ([self->stretchyConstraints count] != 0) {
		[self removeConstraints:self->stretchyConstraints];
		[self->stretchyConstraints removeAllObjects];
	}
	firstStretchy = nil;
	for (bc in self->children) {
		if (!uiControlVisible(bc.c) || !bc.stretchy)
			continue;
		if (firstStretchy == nil) {
			firstStretchy = [bc view];
			continue;
		}
		constraint = uiprivMkConstraint(firstStretchy, self->primarySize,
			NSLayoutRelationEqual,
			[bc view], self->primarySize,
			1, 0,
			@"uiBox stretchy size constraint");
		[self addConstraint:constraint];
		[self->stretchyConstraints addObject:constraint];
	}
	hasPrimaryExpansion = firstStretchy != nil;
	if (hasPrimaryExpansion != self->primaryExpansion) {
		self->primaryExpansion = hasPrimaryExpansion;
		uiDarwinNotifyEdgeHuggingChanged(uiDarwinControl(self->b));
	}
}

- (void)append:(uiControl *)c stretchy:(int)stretchy
{
	boxChild *bc;
	NSLayoutConstraint *constraint;
	NSLayoutPriority priority;

	bc = [boxChild new];
	bc.c = c;
	bc.stretchy = stretchy != 0;
	bc.oldPrimaryHuggingPri = uiDarwinControlHuggingPriority(uiDarwinControl(bc.c), self->primaryOrientation);
	bc.oldSecondaryHuggingPri = uiDarwinControlHuggingPriority(uiDarwinControl(bc.c), self->secondaryOrientation);

	uiControlSetParent(bc.c, uiControl(self->b));
	uiDarwinControlSetSuperview(uiDarwinControl(bc.c), self);
	uiDarwinControlSyncEnableState(uiDarwinControl(bc.c), uiControlEnabledToUser(uiControl(self->b)));
	[self addArrangedSubview:[bc view]];

	priority = bc.stretchy ? NSLayoutPriorityDefaultLow : NSLayoutPriorityRequired;
	uiDarwinControlSetHuggingPriority(uiDarwinControl(bc.c), priority, self->primaryOrientation);
	uiDarwinControlSetHuggingPriority(uiDarwinControl(bc.c), NSLayoutPriorityDefaultLow, self->secondaryOrientation);

	constraint = uiprivMkConstraint([bc view], self->secondarySize,
		NSLayoutRelationEqual,
		self, self->secondarySize,
		1, 0,
		@"uiBox secondary fill constraint");
	[self addConstraint:constraint];
	bc.secondaryConstraint = constraint;

	[self->children addObject:bc];
	[self updateLayout];
	[bc release];
}

- (void)delete:(int)n
{
	boxChild *bc;

	bc = (boxChild *) [self->children objectAtIndex:n];
	if ([self->stretchyConstraints count] != 0) {
		[self removeConstraints:self->stretchyConstraints];
		[self->stretchyConstraints removeAllObjects];
	}
	[self removeConstraint:bc.secondaryConstraint];
	bc.secondaryConstraint = nil;

	uiControlSetParent(bc.c, NULL);
	uiDarwinControlSetSuperview(uiDarwinControl(bc.c), nil);
	uiDarwinControlSetHuggingPriority(uiDarwinControl(bc.c), bc.oldPrimaryHuggingPri, self->primaryOrientation);
	uiDarwinControlSetHuggingPriority(uiDarwinControl(bc.c), bc.oldSecondaryHuggingPri, self->secondaryOrientation);
	[self->children removeObjectAtIndex:n];

	[self updateLayout];
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
	[self setSpacing:self->padded ? uiDarwinPaddingAmount(NULL) : 0];
}

- (BOOL)hugsTrailing
{
	if (self->vertical)
		return YES;
	return self->primaryExpansion;
}

- (BOOL)hugsBottom
{
	if (!self->vertical)
		return YES;
	return self->primaryExpansion;
}

@end

static void uiBoxDestroy(uiControl *c)
{
	uiBox *b = uiBox(c);

	[b->view onDestroy];
	[b->view release];
	uiFreeControl(uiControl(b));
}

uiDarwinControlDefaultHandle(uiBox, view)
uiDarwinControlDefaultParent(uiBox, view)
uiDarwinControlDefaultSetParent(uiBox, view)
uiDarwinControlDefaultToplevel(uiBox, view)
uiDarwinControlDefaultVisible(uiBox, view)
uiDarwinControlDefaultShow(uiBox, view)
uiDarwinControlDefaultHide(uiBox, view)
uiDarwinControlDefaultEnabled(uiBox, view)
uiDarwinControlDefaultEnable(uiBox, view)
uiDarwinControlDefaultDisable(uiBox, view)

static void uiBoxSyncEnableState(uiDarwinControl *c, int enabled)
{
	uiBox *b = uiBox(c);

	if (uiDarwinShouldStopSyncEnableState(uiDarwinControl(b), enabled))
		return;
	[b->view syncEnableStates:enabled];
}

uiDarwinControlDefaultSetSuperview(uiBox, view)

static BOOL uiBoxHugsTrailingEdge(uiDarwinControl *c)
{
	uiBox *b = uiBox(c);

	return [b->view hugsTrailing];
}

static BOOL uiBoxHugsBottom(uiDarwinControl *c)
{
	uiBox *b = uiBox(c);

	return [b->view hugsBottom];
}

uiDarwinControlDefaultChildEdgeHuggingChanged(uiBox, view)
uiDarwinControlDefaultHuggingPriority(uiBox, view)
uiDarwinControlDefaultSetHuggingPriority(uiBox, view)

static void uiBoxChildVisibilityChanged(uiDarwinControl *c)
{
	uiBox *b = uiBox(c);

	[b->view updateLayout];
}

void uiBoxAppend(uiBox *b, uiControl *c, int stretchy)
{
	// LONGTERM on other platforms
	// or at leat allow this and implicitly turn it into a spacer
	if (c == NULL)
		uiprivUserBug("You cannot add NULL to a uiBox.");
	[b->view append:c stretchy:stretchy];
}

void uiBoxDelete(uiBox *b, int n)
{
	[b->view delete:n];
}

int uiBoxNumChildren(uiBox *b)
{
	return [b->view numChildren];
}

int uiBoxPadded(uiBox *b)
{
	return [b->view isPadded];
}

void uiBoxSetPadded(uiBox *b, int padded)
{
	[b->view setPadded:padded];
}

static uiBox *finishNewBox(BOOL vertical)
{
	uiBox *b;

	uiDarwinNewControl(uiBox, b);

	b->view = [[boxView alloc] initWithVertical:vertical b:b];

	return b;
}

uiBox *uiNewHorizontalBox(void)
{
	return finishNewBox(NO);
}

uiBox *uiNewVerticalBox(void)
{
	return finishNewBox(YES);
}
