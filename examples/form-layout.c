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
	uiForm *form;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("Form Layout", 360, 180, 0);
	uiWindowSetMargined(w, 1);
	uiWindowOnClosing(w, onClosing, NULL);

	form = uiNewForm();
	uiFormSetPadded(form, 1);
	uiWindowSetChild(w, uiControl(form));

	uiFormAppend(form, "Name", uiControl(uiNewEntry()), 0);
	uiFormAppend(form, "Password", uiControl(uiNewPasswordEntry()), 0);
	uiFormAppend(form, "Notes", uiControl(uiNewMultilineEntry()), 1);

	uiControlShow(uiControl(w));
	uiMain();
	uiUninit();
	return 0;
}
