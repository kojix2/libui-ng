#include <stdio.h>
#include <ui.h>

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
	uiBox *outer;
	uiBox *row;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("Box Layout", 360, 180, 0);
	uiWindowSetMargined(w, 1);
	uiWindowOnClosing(w, onClosing, NULL);

	outer = uiNewVerticalBox();
	uiBoxSetPadded(outer, 1);
	uiWindowSetChild(w, uiControl(outer));

	uiBoxAppend(outer, uiControl(uiNewLabel("Vertical box item")), 0);

	row = uiNewHorizontalBox();
	uiBoxSetPadded(row, 1);
	uiBoxAppend(row, uiControl(uiNewButton("Left")), 0);
	uiBoxAppend(row, uiControl(uiNewButton("Right")), 0);
	uiBoxAppend(outer, uiControl(row), 0);

	uiBoxAppend(outer, uiControl(uiNewLabel("Stretchy item")), 1);

	uiControlShow(uiControl(w));
	uiMain();
	uiUninit();
	return 0;
}
