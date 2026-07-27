#include <stdio.h>
#include <ui.h>

static void onSpinboxChanged(uiSpinbox *spinbox, void *data)
{
	uiLabel *label = uiLabel(data);
	char text[64];

	snprintf(text, sizeof text, "Value: %d", uiSpinboxValue(spinbox));
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
	uiSpinbox *spinbox;
	uiLabel *label;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("Spinbox", 320, 160, 0);
	uiWindowSetMargined(w, 1);
	uiWindowOnClosing(w, onClosing, NULL);

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	uiWindowSetChild(w, uiControl(box));

	spinbox = uiNewSpinbox(0, 100);
	uiSpinboxSetValue(spinbox, 25);
	uiBoxAppend(box, uiControl(spinbox), 0);

	label = uiNewLabel("Value: 25");
	uiSpinboxOnChanged(spinbox, onSpinboxChanged, label);
	uiBoxAppend(box, uiControl(label), 0);

	uiControlShow(uiControl(w));
	uiMain();
	uiUninit();
	return 0;
}
