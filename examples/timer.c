#include <stdio.h>
#include <ui.h>

static uiLabel *label;
static int seconds;

static int onTimer(void *data)
{
	char text[64];

	seconds++;
	snprintf(text, sizeof text, "Timer tick: %d", seconds);
	uiLabelSetText(label, text);

	return seconds < 10;
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

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("Timer", 320, 120, 0);
	uiWindowSetMargined(w, 1);
	uiWindowOnClosing(w, onClosing, NULL);

	label = uiNewLabel("Timer tick: 0");
	uiWindowSetChild(w, uiControl(label));

	uiTimer(1000, onTimer, NULL);

	uiControlShow(uiControl(w));
	uiMain();
	uiUninit();
	return 0;
}
