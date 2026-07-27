#include <stdio.h>
#include <ui.h>

static void updateLabel(uiFontButton *button, uiLabel *label)
{
	uiFontDescriptor font;

	uiFontButtonFont(button, &font);
	uiLabelSetText(label, font.Family);
	uiFreeFontButtonFont(&font);
}

static void onFontChanged(uiFontButton *button, void *data)
{
	updateLabel(button, uiLabel(data));
}

static int onClosing(uiWindow *w, void *data)
{
	uiQuit();
	return 1;
}

int main(void)
{
	uiInitOptions o = {0};
	const char *err;
	uiWindow *w;
	uiBox *box;
	uiFontButton *button;
	uiLabel *label;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("Font Button", 420, 160, 0);
	uiWindowSetMargined(w, 1);
	uiWindowOnClosing(w, onClosing, NULL);

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	uiWindowSetChild(w, uiControl(box));

	button = uiNewFontButton();
	uiBoxAppend(box, uiControl(button), 0);

	label = uiNewLabel("");
	updateLabel(button, label);
	uiFontButtonOnChanged(button, onFontChanged, label);
	uiBoxAppend(box, uiControl(label), 0);

	uiControlShow(uiControl(w));
	uiMain();
	uiUninit();
	return 0;
}
