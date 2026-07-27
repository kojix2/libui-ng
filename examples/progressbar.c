#include <stdio.h>
#include <ui.h>

static int value;

static void onButtonClicked(uiButton *button, void *data)
{
	uiProgressBar *progressbar = uiProgressBar(data);

	value += 10;
	if (value > 100) {
		uiProgressBarSetValue(progressbar, -1);
		value = 0;
		return;
	}
	uiProgressBarSetValue(progressbar, value);
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
	uiProgressBar *progressbar;
	uiButton *button;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("Progress Bar", 320, 160, 0);
	uiWindowSetMargined(w, 1);
	uiWindowOnClosing(w, onClosing, NULL);

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	uiWindowSetChild(w, uiControl(box));

	progressbar = uiNewProgressBar();
	uiProgressBarSetValue(progressbar, value);
	uiBoxAppend(box, uiControl(progressbar), 0);

	button = uiNewButton("Advance");
	uiButtonOnClicked(button, onButtonClicked, progressbar);
	uiBoxAppend(box, uiControl(button), 0);

	uiControlShow(uiControl(w));
	uiMain();
	uiUninit();
	return 0;
}
