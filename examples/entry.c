#include <stdio.h>
#include <ui.h>

static void onEntryChanged(uiEntry *entry, void *data)
{
	uiLabel *label = uiLabel(data);
	char *text;

	text = uiEntryText(entry);
	uiLabelSetText(label, text);
	uiFreeText(text);
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
	uiEntry *entry;
	uiLabel *label;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("Entry", 320, 160, 0);
	uiWindowSetMargined(w, 1);
	uiWindowOnClosing(w, onClosing, NULL);

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	uiWindowSetChild(w, uiControl(box));

	entry = uiNewEntry();
	uiEntrySetText(entry, "Edit this text");
	uiBoxAppend(box, uiControl(entry), 0);

	label = uiNewLabel("Edit this text");
	uiEntryOnChanged(entry, onEntryChanged, label);
	uiBoxAppend(box, uiControl(label), 0);

	uiControlShow(uiControl(w));
	uiMain();
	uiUninit();
	return 0;
}
