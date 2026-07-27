#include <stdio.h>
#include <ui.h>

static void onCheckboxToggled(uiCheckbox *checkbox, void *data)
{
	uiLabel *label = uiLabel(data);

	if (uiCheckboxChecked(checkbox))
		uiLabelSetText(label, "Checked");
	else
		uiLabelSetText(label, "Not checked");
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
	uiCheckbox *checkbox;
	uiLabel *label;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("Checkbox", 320, 160, 0);
	uiWindowSetMargined(w, 1);
	uiWindowOnClosing(w, onClosing, NULL);

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	uiWindowSetChild(w, uiControl(box));

	checkbox = uiNewCheckbox("Enable option");
	uiBoxAppend(box, uiControl(checkbox), 0);

	label = uiNewLabel("Not checked");
	uiCheckboxOnToggled(checkbox, onCheckboxToggled, label);
	uiBoxAppend(box, uiControl(label), 0);

	uiControlShow(uiControl(w));
	uiMain();
	uiUninit();
	return 0;
}
