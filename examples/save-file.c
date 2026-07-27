#include <stdio.h>
#include <ui.h>

static uiWindow *mainwin;
static uiEntry *pathEntry;

static void onSaveFileClicked(uiButton *button, void *data)
{
	char *filename;

	filename = uiSaveFile(mainwin);
	if (filename == NULL) {
		uiEntrySetText(pathEntry, "(cancelled)");
		return;
	}

	uiEntrySetText(pathEntry, filename);
	uiFreeText(filename);
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

	mainwin = uiNewWindow("Save File", 480, 140, 0);
	uiWindowSetMargined(mainwin, 1);
	uiWindowOnClosing(mainwin, onClosing, NULL);

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	uiWindowSetChild(mainwin, uiControl(box));

	pathEntry = uiNewEntry();
	uiEntrySetReadOnly(pathEntry, 1);
	uiBoxAppend(box, uiControl(pathEntry), 0);

	button = uiNewButton("Save File");
	uiButtonOnClicked(button, onSaveFileClicked, NULL);
	uiBoxAppend(box, uiControl(button), 0);

	uiControlShow(uiControl(mainwin));
	uiMain();
	uiUninit();
	return 0;
}
