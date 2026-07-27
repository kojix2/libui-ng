#include <stdio.h>
#include <ui.h>

static void onEditableComboboxChanged(uiEditableCombobox *combobox, void *data)
{
	uiLabel *label = uiLabel(data);
	char *text;

	text = uiEditableComboboxText(combobox);
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
	uiEditableCombobox *combobox;
	uiLabel *label;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("Editable Combobox", 360, 160, 0);
	uiWindowSetMargined(w, 1);
	uiWindowOnClosing(w, onClosing, NULL);

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	uiWindowSetChild(w, uiControl(box));

	combobox = uiNewEditableCombobox();
	uiEditableComboboxAppend(combobox, "First");
	uiEditableComboboxAppend(combobox, "Second");
	uiEditableComboboxAppend(combobox, "Third");
	uiEditableComboboxSetText(combobox, "Editable text");
	uiBoxAppend(box, uiControl(combobox), 0);

	label = uiNewLabel("Editable text");
	uiEditableComboboxOnChanged(combobox, onEditableComboboxChanged, label);
	uiBoxAppend(box, uiControl(label), 0);

	uiControlShow(uiControl(w));
	uiMain();
	uiUninit();
	return 0;
}
