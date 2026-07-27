#include <stdio.h>
#include <ui.h>

static void onRadioButtonsSelected(uiRadioButtons *radioButtons, void *data)
{
	uiLabel *label = uiLabel(data);
	char text[64];

	snprintf(text, sizeof text, "Selected index: %d", uiRadioButtonsSelected(radioButtons));
	uiLabelSetText(label, text);
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
	uiRadioButtons *radioButtons;
	uiLabel *label;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("Radio Buttons", 320, 180, 0);
	uiWindowSetMargined(w, 1);
	uiWindowOnClosing(w, onClosing, NULL);

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	uiWindowSetChild(w, uiControl(box));

	radioButtons = uiNewRadioButtons();
	uiRadioButtonsAppend(radioButtons, "First");
	uiRadioButtonsAppend(radioButtons, "Second");
	uiRadioButtonsAppend(radioButtons, "Third");
	uiRadioButtonsSetSelected(radioButtons, 0);
	uiBoxAppend(box, uiControl(radioButtons), 0);

	label = uiNewLabel("Selected index: 0");
	uiRadioButtonsOnSelected(radioButtons, onRadioButtonsSelected, label);
	uiBoxAppend(box, uiControl(label), 0);

	uiControlShow(uiControl(w));
	uiMain();
	uiUninit();
	return 0;
}
