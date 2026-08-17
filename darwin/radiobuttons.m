// 14 august 2015
#import "uipriv_darwin.h"

// In the old days you would use a NSMatrix for this; as of OS X 10.8 this was deprecated and now you need just a bunch of NSButtons with the same superview AND same action method.
// This is documented on the NSMatrix page, but the rest of the OS X documentation says to still use NSMatrix.
// NSMatrix has weird quirks anyway...

// LONGTERM 6 units of spacing between buttons, as suggested by Interface Builder?

@interface radioButtonsDelegate : NSObject {
	uiRadioButtons *libui_r;
}
- (id)initWithR:(uiRadioButtons *)r;
- (IBAction)onClicked:(id)sender;
@end

struct uiRadioButtons {
	uiDarwinControl c;
	NSStackView *view;
	NSMutableArray *buttons;
	radioButtonsDelegate *delegate;
	void (*onSelected)(uiRadioButtons *, void *);
	void *onSelectedData;
	int selected;
};

@implementation radioButtonsDelegate

- (id)initWithR:(uiRadioButtons *)r
{
	self = [super init];
	if (self)
		self->libui_r = r;
	return self;
}

- (IBAction)onClicked:(id)sender
{
	NSButton *b = (NSButton *)sender;
	uiRadioButtons *r = self->libui_r;
	NSInteger index;

	index = [r->buttons indexOfObject:b];
	if (index == r->selected)
		return;

	r->selected = index;
	(*(r->onSelected))(r, r->onSelectedData);
}

@end

uiDarwinControlDefaultHandle(uiRadioButtons, view)
uiDarwinControlDefaultParent(uiRadioButtons, view)
uiDarwinControlDefaultSetParent(uiRadioButtons, view)
uiDarwinControlDefaultToplevel(uiRadioButtons, view)
uiDarwinControlDefaultVisible(uiRadioButtons, view)
uiDarwinControlDefaultShow(uiRadioButtons, view)
uiDarwinControlDefaultHide(uiRadioButtons, view)
uiDarwinControlDefaultEnabled(uiRadioButtons, view)
uiDarwinControlDefaultEnable(uiRadioButtons, view)
uiDarwinControlDefaultDisable(uiRadioButtons, view)

static void uiRadioButtonsSyncEnableState(uiDarwinControl *c, int enabled)
{
	uiRadioButtons *r = uiRadioButtons(c);
	NSButton *b;

	if (uiDarwinShouldStopSyncEnableState(uiDarwinControl(r), enabled))
		return;
	for (b in r->buttons)
		[b setEnabled:enabled];
}

uiDarwinControlDefaultSetSuperview(uiRadioButtons, view)
uiDarwinControlDefaultHugsTrailingEdge(uiRadioButtons, view)
uiDarwinControlDefaultHugsBottom(uiRadioButtons, view)
uiDarwinControlDefaultChildEdgeHuggingChanged(uiRadioButtons, view)
uiDarwinControlDefaultHuggingPriority(uiRadioButtons, view)
uiDarwinControlDefaultSetHuggingPriority(uiRadioButtons, view)
uiDarwinControlDefaultChildVisibilityChanged(uiRadioButtons, view)

static void defaultOnSelected(uiRadioButtons *r, void *data)
{
	// do nothing
}

static void uiRadioButtonsDestroy(uiControl *c)
{
	uiRadioButtons *r = uiRadioButtons(c);
	NSButton *b;

	for (b in r->buttons)
		[b setTarget:nil];
	[r->buttons release];
	// destroy the delegate
	[r->delegate release];
	// and destroy ourselves
	[r->view release];
	uiFreeControl(uiControl(r));
}

void uiRadioButtonsAppend(uiRadioButtons *r, const char *text)
{
	NSButton *b;
	NSLayoutConstraint *constraint;

	b = [NSButton radioButtonWithTitle:uiprivToNSString(text)
		target:r->delegate
		action:@selector(onClicked:)];
	[b setTranslatesAutoresizingMaskIntoConstraints:NO];
	[b setEnabled:uiControlEnabledToUser(uiControl(r))];

	[r->buttons addObject:b];
	[r->view addArrangedSubview:b];
	constraint = uiprivMkConstraint(b, NSLayoutAttributeWidth,
		NSLayoutRelationEqual,
		r->view, NSLayoutAttributeWidth,
		1, 0,
		@"uiRadioButtons button width constraint");
	[r->view addConstraint:constraint];
}

int uiRadioButtonsSelected(uiRadioButtons *r)
{
	return r->selected;
}

void uiRadioButtonsSetSelected(uiRadioButtons *r, int n)
{
	NSButton *b;
	NSInteger state;
	NSInteger previous;
	NSUInteger count;

	count = [r->buttons count];
	if (n < -1) {
		uiprivUserBug("Index %d is out of range for a uiRadioButtons.", n);
		return;
	}
	if (n >= 0 && ((NSUInteger) n) >= count) {
		uiprivUserBug("Index %d is out of range for a uiRadioButtons.", n);
		return;
	}
	previous = r->selected;

	r->selected = n;

	state = NSOnState;
	if (n == -1) {
		n = previous;
		if (n == -1)		// from nothing to nothing; do nothing
			return;
		state = NSOffState;
	}
	b = (NSButton *) [r->buttons objectAtIndex:n];
	[b setState:state];
}

void uiRadioButtonsOnSelected(uiRadioButtons *r, void (*f)(uiRadioButtons *, void *), void *data)
{
	r->onSelected = f;
	r->onSelectedData = data;
}

uiRadioButtons *uiNewRadioButtons(void)
{
	uiRadioButtons *r;

	uiDarwinNewControl(uiRadioButtons, r);

	r->buttons = [NSMutableArray new];
	r->selected = -1;
	r->view = [[NSStackView alloc] initWithFrame:NSZeroRect];
	[r->view setOrientation:NSUserInterfaceLayoutOrientationVertical];
	[r->view setDistribution:NSStackViewDistributionFill];
	[r->view setSpacing:0];

	r->delegate = [[radioButtonsDelegate alloc] initWithR:r];

	uiRadioButtonsOnSelected(r, defaultOnSelected, NULL);

	return r;
}
