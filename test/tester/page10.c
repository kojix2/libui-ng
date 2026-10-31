// 22 december 2015
#include "test.h"

static uiEntry *textString;
static uiFontButton *textFontButton;
static uiColorButton *textColorButton;
static uiEntry *textWidth;
static uiButton *textApply;
static uiCheckbox *noZ;
static uiArea *textArea;
static uiAreaHandler textAreaHandler;

static double entryDouble(uiEntry *e)
{
	char *s;
	double d;

	s = uiEntryText(e);
	d = atof(s);
	uiFreeText(s);
	return d;
}

static void handlerDraw(uiAreaHandler *a, uiArea *area, uiAreaDrawParams *dp)
{
	uiFontDescriptor font;
	uiDrawTextLayout *layout;
	uiDrawTextLayoutParams params;
	uiAttributedString *attrstr;
	double r, g, b, al;
	const char *surrogates;
	const char *composed;
	double width, height;

	uiFontButtonFont(textFontButton, &font);
	width = entryDouble(textWidth);

	attrstr = uiNewAttributedString("One two three four");
	uiAttributedStringSetAttribute(attrstr,
		uiNewColorAttribute(1, 0, 0, 1),
		4, 7);
	uiAttributedStringSetAttribute(attrstr,
		uiNewColorAttribute(1, 0, 0.5, 0.5),
		8, 14);
	uiColorButtonColor(textColorButton, &r, &g, &b, &al);
	uiAttributedStringSetAttribute(attrstr,
		uiNewColorAttribute(r, g, b, al),
		14, 18);
	params.String = attrstr;
	params.DefaultFont = &font;
	params.Width = width;
	params.Align = uiDrawTextAlignLeft;
	layout = uiDrawNewTextLayout(&params);
	uiDrawText(dp->Context, layout, 10, 10);
	uiDrawTextLayoutExtents(layout, &width, &height);
	uiDrawFreeTextLayout(layout);
	uiFreeAttributedString(attrstr);

	surrogates = "x\360\220\214\210y";		// U+10308

	attrstr = uiNewAttributedString(surrogates);
	uiAttributedStringSetAttribute(attrstr,
		uiNewColorAttribute(1, 0, 0.5, 0.5),
		uiAttributedStringGraphemeToByteIndex(attrstr, 1),
		uiAttributedStringGraphemeToByteIndex(attrstr, 2));
	params.String = attrstr;
	layout = uiDrawNewTextLayout(&params);
	uiDrawText(dp->Context, layout, 10, 10 + height);
	uiDrawFreeTextLayout(layout);
	uiFreeAttributedString(attrstr);

	composed = "zz\303\251zze\314\201zz";

	attrstr = uiNewAttributedString(composed);
	uiAttributedStringSetAttribute(attrstr,
		uiNewColorAttribute(1, 0, 0.5, 0.5),
		uiAttributedStringGraphemeToByteIndex(attrstr, 2),
		uiAttributedStringGraphemeToByteIndex(attrstr, 3));
	uiAttributedStringSetAttribute(attrstr,
		uiNewColorAttribute(1, 0, 0.5, 0.5),
		uiAttributedStringGraphemeToByteIndex(attrstr, 5),
		uiAttributedStringGraphemeToByteIndex(attrstr, 6));
	if (!uiCheckboxChecked(noZ))
		uiAttributedStringSetAttribute(attrstr,
			uiNewColorAttribute(0.5, 0, 1, 0.5),
			uiAttributedStringGraphemeToByteIndex(attrstr, 6),
			uiAttributedStringGraphemeToByteIndex(attrstr, 7));
	params.String = attrstr;
	layout = uiDrawNewTextLayout(&params);
	uiDrawText(dp->Context, layout, 10, 10 + height + height);
	uiDrawFreeTextLayout(layout);
	uiFreeAttributedString(attrstr);

	uiFreeFontButtonFont(&font);
}

static void handlerMouseEvent(uiAreaHandler *a, uiArea *area, uiAreaMouseEvent *e)
{
	// do nothing
}

static void handlerMouseCrossed(uiAreaHandler *ah, uiArea *a, int left)
{
	// do nothing
}

static void handlerDragBroken(uiAreaHandler *ah, uiArea *a)
{
	// do nothing
}

static int handlerKeyEvent(uiAreaHandler *ah, uiArea *a, uiAreaKeyEvent *e)
{
	// do nothing
	return 0;
}

static void onFontChanged(uiFontButton *b, void *data)
{
	uiAreaQueueRedrawAll(textArea);
}

static void onColorChanged(uiColorButton *b, void *data)
{
	uiAreaQueueRedrawAll(textArea);
}

static void onNoZ(uiCheckbox *b, void *data)
{
	uiAreaQueueRedrawAll(textArea);
}

static void onTextApply(uiButton *b, void *data)
{
	uiAreaQueueRedrawAll(textArea);
}

uiBox *makePage10(void)
{
	uiBox *page10;
	uiBox *vbox;
	uiBox *hbox;

	page10 = newVerticalBox();
	vbox = page10;

	hbox = newHorizontalBox();
	uiBoxAppend(vbox, uiControl(hbox), 0);

	textString = uiNewEntry();
	// TODO make it placeholder
	uiEntrySetText(textString, "Enter text here");
	uiBoxAppend(hbox, uiControl(textString), 1);

	textFontButton = uiNewFontButton();
	uiFontButtonOnChanged(textFontButton, onFontChanged, NULL);
	uiBoxAppend(hbox, uiControl(textFontButton), 1);

	textColorButton = uiNewColorButton();
	uiColorButtonOnChanged(textColorButton, onColorChanged, NULL);
	uiBoxAppend(hbox, uiControl(textColorButton), 1);

	hbox = newHorizontalBox();
	uiBoxAppend(vbox, uiControl(hbox), 0);

	textApply = uiNewButton("Apply");
	uiButtonOnClicked(textApply, onTextApply, NULL);
	uiBoxAppend(hbox, uiControl(textApply), 1);

	textWidth = uiNewEntry();
	uiEntrySetText(textWidth, "-1");
	uiBoxAppend(hbox, uiControl(textWidth), 1);

	noZ = uiNewCheckbox("No Z Color");
	uiCheckboxOnToggled(noZ, onNoZ, NULL);
	uiBoxAppend(hbox, uiControl(noZ), 0);

	textAreaHandler.Draw = handlerDraw;
	textAreaHandler.MouseEvent = handlerMouseEvent;
	textAreaHandler.MouseCrossed = handlerMouseCrossed;
	textAreaHandler.DragBroken = handlerDragBroken;
	textAreaHandler.KeyEvent = handlerKeyEvent;
	textArea = uiNewArea(&textAreaHandler);
	uiBoxAppend(vbox, uiControl(textArea), 1);

	// dummy objects to test single-activation
	hbox = newHorizontalBox();
	uiBoxAppend(vbox, uiControl(hbox), 0);
	uiBoxAppend(hbox, uiControl(uiNewFontButton()), 1);
	uiBoxAppend(hbox, uiControl(uiNewColorButton()), 1);

	return page10;
}
