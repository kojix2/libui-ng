#include "uipriv_darwin.h"

// https://developer.apple.com/documentation/appkit/nsview/1483541-tooltip?language=objc

void uiControlSetTooltip(uiControl *c, const char *tooltip)
{
	NSView *view;

	if (c->TypeSignature == uiWindowSignature)
		view = [(NSWindow *)uiControlHandle(c) contentView];
	else
		view = (NSView *)uiControlHandle(c);

	if (c->TypeSignature == uiSliderSignature && tooltip != NULL)
		uiprivSliderSetControlTooltip(uiSlider(c), 1);
	[view setToolTip:(tooltip == NULL ? nil : uiprivToNSString(tooltip))];
	if (c->TypeSignature == uiSliderSignature && tooltip == NULL)
		uiprivSliderSetControlTooltip(uiSlider(c), 0);
}
