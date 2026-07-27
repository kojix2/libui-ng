#include <stdio.h>
#include <ui.h>

static void onButtonClicked(uiButton *button, void *data)
{
	uiMultilineEntry *entry = uiMultilineEntry(data);

	uiMultilineEntryAppend(entry, "Appended line\n");
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
	uiMultilineEntry *entry;
	uiButton *button;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("Multiline Entry", 360, 240, 0);
	uiWindowSetMargined(w, 1);
	uiWindowOnClosing(w, onClosing, NULL);

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	uiWindowSetChild(w, uiControl(box));

	entry = uiNewMultilineEntry();
	uiMultilineEntrySetText(entry, "First line\nSecond line\n");
	uiBoxAppend(box, uiControl(entry), 1);

	button = uiNewButton("Append Line");
	uiButtonOnClicked(button, onButtonClicked, entry);
	uiBoxAppend(box, uiControl(button), 0);

	uiControlShow(uiControl(w));
	uiMain();
	uiUninit();
	return 0;
}
