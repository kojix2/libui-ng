#include <stdio.h>
#include <ui.h>

static void onComboboxSelected(uiCombobox *combobox, void *data)
{
	uiLabel *label = uiLabel(data);
	char text[64];

	snprintf(text, sizeof text, "Selected index: %d", uiComboboxSelected(combobox));
	uiLabelSetText(label, text);
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
	uiCombobox *combobox;
	uiLabel *label;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("Combobox", 320, 160, 0);
	uiWindowSetMargined(w, 1);
	uiWindowOnClosing(w, onClosing, NULL);

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	uiWindowSetChild(w, uiControl(box));

	combobox = uiNewCombobox();
	uiComboboxAppend(combobox, "First");
	uiComboboxAppend(combobox, "Second");
	uiComboboxAppend(combobox, "Third");
	uiComboboxSetSelected(combobox, 0);
	uiBoxAppend(box, uiControl(combobox), 0);

	label = uiNewLabel("Selected index: 0");
	uiComboboxOnSelected(combobox, onComboboxSelected, label);
	uiBoxAppend(box, uiControl(label), 0);

	uiControlShow(uiControl(w));
	uiMain();
	uiUninit();
	return 0;
}
