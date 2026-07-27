#include <stdio.h>
#include <ui.h>

static void onButtonClicked(uiButton *button, void *data)
{
	uiLabel *label = uiLabel(data);

	uiLabelSetText(label, "Clicked");
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
	uiButton *button;
	uiLabel *label;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("Button", 320, 160, 0);
	uiWindowSetMargined(w, 1);
	uiWindowOnClosing(w, onClosing, NULL);

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	uiWindowSetChild(w, uiControl(box));

	label = uiNewLabel("Not clicked");
	uiBoxAppend(box, uiControl(label), 0);

	button = uiNewButton("Click me");
	uiButtonOnClicked(button, onButtonClicked, label);
	uiBoxAppend(box, uiControl(button), 0);

	uiControlShow(uiControl(w));
	uiMain();
	uiUninit();
	return 0;
}
