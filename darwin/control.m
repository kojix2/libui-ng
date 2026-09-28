// 16 august 2015
#import "uipriv_darwin.h"
#import <objc/runtime.h>

typedef struct minimumSizeAxis minimumSizeAxis;
struct minimumSizeAxis {
	NSLayoutConstraint *constraint;
	NSLayoutPriority oldCompressionResistance;
	BOOL changedCompressionResistance;
};

@interface uiprivMinimumSizeState : NSObject {
@public
	minimumSizeAxis width;
	minimumSizeAxis height;
	int defaultWidth;
	int defaultHeight;
}
@end

@implementation uiprivMinimumSizeState

- (id)init
{
	self = [super init];
	if (self != nil) {
		defaultWidth = -1;
		defaultHeight = -1;
	}
	return self;
}

- (void)dealloc
{
	[width.constraint release];
	[height.constraint release];
	[super dealloc];
}

@end

static char minimumSizeStateAssociationKey;

static uiprivMinimumSizeState *minimumSizeState(NSView *view, BOOL create)
{
	uiprivMinimumSizeState *state;

	state = objc_getAssociatedObject(view, &minimumSizeStateAssociationKey);
	if (state == nil && create) {
		state = [uiprivMinimumSizeState new];
		objc_setAssociatedObject(view, &minimumSizeStateAssociationKey, state,
			OBJC_ASSOCIATION_RETAIN_NONATOMIC);
		[state release];
	}
	return state;
}

static void setMinimumSizeConstraint(NSView *view,
	uiprivMinimumSizeState *state, int value, NSLayoutAttribute attribute)
{
	minimumSizeAxis *axis;
	NSLayoutConstraintOrientation orientation;

	if (attribute == NSLayoutAttributeWidth) {
		axis = &(state->width);
		orientation = NSLayoutConstraintOrientationHorizontal;
	} else {
		axis = &(state->height);
		orientation = NSLayoutConstraintOrientationVertical;
	}

	if (value == -1) {
		if (axis->constraint != nil) {
			[view removeConstraint:axis->constraint];
			[axis->constraint release];
			axis->constraint = nil;
		}
		if (axis->changedCompressionResistance) {
			[view setContentCompressionResistancePriority:axis->oldCompressionResistance
				forOrientation:orientation];
			axis->changedCompressionResistance = NO;
		}
		return;
	}

	if (axis->constraint == nil) {
		axis->constraint = uiprivMkConstraint(view, attribute,
			NSLayoutRelationGreaterThanOrEqual,
			nil, NSLayoutAttributeNotAnAttribute,
			0, value, @"uiControl minimum size override");
		[view addConstraint:axis->constraint];
		[axis->constraint retain];
	} else
		[axis->constraint setConstant:value];

	if (!axis->changedCompressionResistance) {
		axis->oldCompressionResistance = [view contentCompressionResistancePriorityForOrientation:orientation];
		axis->changedCompressionResistance = YES;
	}
	// Keep the intrinsic preferred size while allowing compression to the minimum.
	[view setContentCompressionResistancePriority:NSLayoutPriorityDefaultLow
		forOrientation:orientation];
}

static void invalidateMinimumSizeLayout(NSView *view)
{
	NSView *ancestor;

	[view invalidateIntrinsicContentSize];
	[view setNeedsUpdateConstraints:YES];
	for (ancestor = view; ancestor != nil; ancestor = [ancestor superview])
		[ancestor setNeedsLayout:YES];
}

static int effectiveMinimumSize(int requested, int defaultValue)
{
	if (requested != -1)
		return requested;
	return defaultValue;
}

static void applyMinimumSizeState(uiControl *c, NSView *view,
	uiprivMinimumSizeState *state)
{
	int width, height;

	uiprivControlMinimumSizeGet(c, &width, &height);
	setMinimumSizeConstraint(view, state,
		effectiveMinimumSize(width, state->defaultWidth),
		NSLayoutAttributeWidth);
	setMinimumSizeConstraint(view, state,
		effectiveMinimumSize(height, state->defaultHeight),
		NSLayoutAttributeHeight);
	if (width == -1 && height == -1 &&
		state->defaultWidth == -1 && state->defaultHeight == -1)
		objc_setAssociatedObject(view, &minimumSizeStateAssociationKey, nil,
			OBJC_ASSOCIATION_RETAIN_NONATOMIC);
	invalidateMinimumSizeLayout(view);
}

void uiDarwinControlSyncEnableState(uiDarwinControl *c, int state)
{
	(*(c->SyncEnableState))(c, state);
}

void uiDarwinControlSetSuperview(uiDarwinControl *c, NSView *superview)
{
	(*(c->SetSuperview))(c, superview);
}

BOOL uiDarwinControlHugsTrailingEdge(uiDarwinControl *c)
{
	return (*(c->HugsTrailingEdge))(c);
}

BOOL uiDarwinControlHugsBottom(uiDarwinControl *c)
{
	return (*(c->HugsBottom))(c);
}

void uiDarwinControlChildEdgeHuggingChanged(uiDarwinControl *c)
{
	(*(c->ChildEdgeHuggingChanged))(c);
}

NSLayoutPriority uiDarwinControlHuggingPriority(uiDarwinControl *c, NSLayoutConstraintOrientation orientation)
{
	return (*(c->HuggingPriority))(c, orientation);
}

void uiDarwinControlSetHuggingPriority(uiDarwinControl *c, NSLayoutPriority priority, NSLayoutConstraintOrientation orientation)
{
	(*(c->SetHuggingPriority))(c, priority, orientation);
}

void uiDarwinControlChildVisibilityChanged(uiDarwinControl *c)
{
	(*(c->ChildVisibilityChanged))(c);
}

void uiDarwinSetControlFont(NSControl *c, NSControlSize size)
{
	[c setFont:[NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:size]]];
}

#define uiDarwinControlSignature 0x44617277

void uiprivControlMinimumSizeChanged(uiControl *c)
{
	NSView *view;
	uiprivMinimumSizeState *state;
	int width, height;

	if (c->OSSignature != uiDarwinControlSignature)
		return;
	view = (NSView *) uiControlHandle(c);
	uiprivControlMinimumSizeGet(c, &width, &height);
	state = minimumSizeState(view, width != -1 || height != -1);
	if (state == nil)
		return;
	applyMinimumSizeState(c, view, state);
}

void uiprivDarwinControlSetDefaultMinimumSize(uiControl *c, int width, int height)
{
	NSView *view;
	uiprivMinimumSizeState *state;

	if (width < -1 || height < -1)
		uiprivImplBug("invalid Darwin default minimum size (%d, %d)",
			width, height);
	view = (NSView *) uiControlHandle(c);
	state = minimumSizeState(view, width != -1 || height != -1);
	if (state == nil)
		return;
	state->defaultWidth = width;
	state->defaultHeight = height;
	applyMinimumSizeState(c, view, state);
}

void uiprivControlMinimumSizeDestroyed(uiControl *c)
{
	// The native view owns its associated state.
}

uiDarwinControl *uiDarwinAllocControl(size_t n, uint32_t typesig, const char *typenamestr)
{
	return uiDarwinControl(uiAllocControl(n, uiDarwinControlSignature, typesig, typenamestr));
}

BOOL uiDarwinShouldStopSyncEnableState(uiDarwinControl *c, BOOL enabled)
{
	int ce;

	ce = uiControlEnabled(uiControl(c));
	// only stop if we're going from disabled back to enabled; don't stop under any other condition
	// (if we stop when going from enabled to disabled then enabled children of a disabled control won't get disabled at the OS level)
	if (!ce && enabled)
		return YES;
	return NO;
}

void uiDarwinNotifyEdgeHuggingChanged(uiDarwinControl *c)
{
	uiControl *parent;

	parent = uiControlParent(uiControl(c));
	if (parent != NULL)
		uiDarwinControlChildEdgeHuggingChanged(uiDarwinControl(parent));
}

void uiDarwinNotifyVisibilityChanged(uiDarwinControl *c)
{
	uiControl *parent;

	parent = uiControlParent(uiControl(c));
	if (parent != NULL)
		uiDarwinControlChildVisibilityChanged(uiDarwinControl(parent));
}
