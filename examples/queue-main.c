#include <stdio.h>
#include <ui.h>

static uiLabel *label;

static void queuedUpdate(void *data)
{
	uiLabelSetText(label, "Queued callback ran on the main thread");
}

static void onButtonClicked(uiButton *button, void *data)
{
	uiQueueMain(queuedUpdate, NULL);
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

	w = uiNewWindow("Queue Main", 420, 160, 0);
	uiWindowSetMargined(w, 1);
	uiWindowOnClosing(w, onClosing, NULL);

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	uiWindowSetChild(w, uiControl(box));

	label = uiNewLabel("Click the button to queue an update.");
	uiBoxAppend(box, uiControl(label), 0);

	button = uiNewButton("Queue Update");
	uiButtonOnClicked(button, onButtonClicked, NULL);
	uiBoxAppend(box, uiControl(button), 0);

	uiControlShow(uiControl(w));
	uiMain();
	uiUninit();
	return 0;
}
