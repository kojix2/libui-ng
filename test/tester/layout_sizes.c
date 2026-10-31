#include "test.h"

static int winClose(uiWindow *w, void *data)
{
	return 1;
}

static void openTestWindow(uiBox *(*mkf)(void))
{
	uiWindow *w;
	uiBox *b;
	uiCombobox *c;
	uiEditableCombobox *e;
	uiRadioButtons *r;

	w = uiNewWindow("Test", 100, 100, 0);
	uiWindowOnClosing(w, winClose, NULL);
	uiWindowSetMargined(w, 1);
	b = (*mkf)();
	uiWindowSetChild(w, uiControl(b));

#define BA(x) uiBoxAppend(b, uiControl(x), 0)
	BA(uiNewButton(""));
	BA(uiNewCheckbox(""));
	BA(uiNewEntry());
	BA(uiNewLabel(""));
	BA(uiNewSpinbox(0, 100));
	BA(uiNewProgressBar());
	BA(uiNewSlider(0, 100));
	BA(uiNewHorizontalSeparator());
	c = uiNewCombobox();
	uiComboboxAppend(c, "");
	BA(c);
	e = uiNewEditableCombobox();
	uiEditableComboboxAppend(e, "");
	BA(e);
	r = uiNewRadioButtons();
	uiRadioButtonsAppend(r, "");
	BA(r);
	BA(uiNewDateTimePicker());
	BA(uiNewDatePicker());
	BA(uiNewTimePicker());
	BA(uiNewMultilineEntry());
	// TODO nonscrolling and scrolling areas?
	BA(uiNewFontButton());
	BA(uiNewColorButton());
	BA(uiNewPasswordEntry());
	BA(uiNewSearchEntry());
	BA(uiNewVerticalSeparator());

	uiControlShow(uiControl(w));
}

static void hButtonClicked(uiButton *b, void *data)
{
	openTestWindow(uiNewHorizontalBox);
}

static void vButtonClicked(uiButton *b, void *data)
{
	openTestWindow(uiNewVerticalBox);
}

uiBox *makeLayoutSizes(void)
{
	uiBox *page;
	uiButton *b;

	page = newVerticalBox();
	uiBoxAppend(page, uiControl(uiNewLabel("Open a window of empty controls to inspect their minimum sizes.")), 0);

	b = uiNewButton("Horizontal");
	uiButtonOnClicked(b, hButtonClicked, NULL);
	uiBoxAppend(page, uiControl(b), 0);

	b = uiNewButton("Vertical");
	uiButtonOnClicked(b, vButtonClicked, NULL);
	uiBoxAppend(page, uiControl(b), 0);

	return page;
}
