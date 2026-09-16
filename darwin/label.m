// 14 august 2015
#import "uipriv_darwin.h"
#include <float.h>

struct uiLabel {
	uiDarwinControl c;
	NSTextField *textfield;
	double fontSize;
	double defaultFontSize;
};

uiDarwinControlAllDefaults(uiLabel, textfield)

char *uiLabelText(uiLabel *l)
{
	return uiDarwinNSStringToText([l->textfield stringValue]);
}

void uiLabelSetText(uiLabel *l, const char *text)
{
	[l->textfield setStringValue:uiprivToNSString(text)];
}

double uiLabelFontSize(uiLabel *l)
{
	return l->fontSize;
}

static void setLabelFontSize(uiLabel *l, double size)
{
	NSFont *font;

	font = [l->textfield font];
	[l->textfield setFont:[font fontWithSize:(CGFloat) size]];
	[l->textfield invalidateIntrinsicContentSize];
	l->fontSize = size;
}

void uiLabelSetFontSize(uiLabel *l, double size)
{
	if (!(size > 0) || size > DBL_MAX) {
		uiprivUserBug("uiLabelSetFontSize() size must be finite and positive.");
		return;
	}
	setLabelFontSize(l, size);
}

void uiLabelResetFontSize(uiLabel *l)
{
	setLabelFontSize(l, l->defaultFontSize);
}

NSTextField *uiprivNewLabel(NSString *str)
{
	NSTextField *tf;

	tf = [[NSTextField alloc] initWithFrame:NSZeroRect];
	[tf setStringValue:str];
	[tf setEditable:NO];
	[tf setSelectable:NO];
	[tf setDrawsBackground:NO];
	uiprivNSTextFieldSetStyleLabel(tf);
	return tf;
}

uiLabel *uiNewLabel(const char *text)
{
	uiLabel *l;

	uiDarwinNewControl(uiLabel, l);

	l->textfield = uiprivNewLabel(uiprivToNSString(text));
	l->fontSize = (double) [[l->textfield font] pointSize];
	l->defaultFontSize = l->fontSize;

	return l;
}
