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
	uiGrid *grid;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("Grid Layout", 360, 180, 0);
	uiWindowSetMargined(w, 1);
	uiWindowOnClosing(w, onClosing, NULL);

	grid = uiNewGrid();
	uiGridSetPadded(grid, 1);
	uiWindowSetChild(w, uiControl(grid));

	uiGridAppend(grid, uiControl(uiNewLabel("Name")),
		0, 0, 1, 1,
		0, uiAlignEnd, 0, uiAlignCenter);
	uiGridAppend(grid, uiControl(uiNewEntry()),
		1, 0, 1, 1,
		1, uiAlignFill, 0, uiAlignCenter);

	uiGridAppend(grid, uiControl(uiNewLabel("Notes")),
		0, 1, 1, 1,
		0, uiAlignEnd, 0, uiAlignStart);
	uiGridAppend(grid, uiControl(uiNewMultilineEntry()),
		1, 1, 1, 1,
		1, uiAlignFill, 1, uiAlignFill);

	uiControlShow(uiControl(w));
	uiMain();
	uiUninit();
	return 0;
}
