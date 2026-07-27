#include <stdio.h>
#include <ui.h>

static uiWindow *mainwin;

static void onMessageBoxClicked(uiButton *button, void *data)
{
	uiMsgBox(mainwin,
		"Message",
		"This message was shown with uiMsgBox().");
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

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("Message Box", 320, 140, 0);
	mainwin = w;
	uiWindowSetMargined(w, 1);
	uiWindowOnClosing(w, onClosing, NULL);

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	uiWindowSetChild(w, uiControl(box));

	button = uiNewButton("Show Message");
	uiButtonOnClicked(button, onMessageBoxClicked, NULL);
	uiBoxAppend(box, uiControl(button), 0);

	uiControlShow(uiControl(w));
	uiMain();
	uiUninit();
	return 0;
}
