#include <stdio.h>
#include <ui.h>

static uiWindow *mainwin;

static void onErrorMessageBoxClicked(uiButton *button, void *data)
{
	uiMsgBoxError(mainwin,
		"Error",
		"This error message was shown with uiMsgBoxError().");
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
	uiBox *box;
	uiButton *button;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	mainwin = uiNewWindow("Error Message Box", 360, 140, 0);
	uiWindowSetMargined(mainwin, 1);
	uiWindowOnClosing(mainwin, onClosing, NULL);

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	uiWindowSetChild(mainwin, uiControl(box));

	button = uiNewButton("Show Error");
	uiButtonOnClicked(button, onErrorMessageBoxClicked, NULL);
	uiBoxAppend(box, uiControl(button), 0);

	uiControlShow(uiControl(mainwin));
	uiMain();
	uiUninit();
	return 0;
}
