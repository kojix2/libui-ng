#include <stdio.h>
#include <ui.h>

static uiWindow *mainwin;
static uiControl *oldChild;

static void replaceChild(void *data)
{
	uiLabel *label;

	(void) data;

	label = uiNewLabel("The old child was detached and destroyed explicitly.");
	uiWindowSetChild(mainwin, uiControl(label));

	uiControlDestroy(oldChild);
	oldChild = NULL;
}

static void onReplaceClicked(uiButton *button, void *data)
{
	(void) button;
	(void) data;

	uiQueueMain(replaceChild, NULL);
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

	mainwin = uiNewWindow("Control Destroy", 420, 180, 0);
	uiWindowSetMargined(mainwin, 1);
	uiWindowOnClosing(mainwin, onClosing, NULL);

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	uiWindowSetChild(mainwin, uiControl(box));
	oldChild = uiControl(box);

	uiBoxAppend(box, uiControl(uiNewLabel("A window has exactly one child.")), 0);
	uiBoxAppend(box, uiControl(uiNewLabel("Replacing it detaches the old child.")), 0);

	button = uiNewButton("Replace Child");
	uiButtonOnClicked(button, onReplaceClicked, NULL);
	uiBoxAppend(box, uiControl(button), 0);

	uiControlShow(uiControl(mainwin));
	uiMain();
	uiUninit();
	return 0;
}
