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
	uiGroup *group;
	uiForm *form;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("Group", 360, 180, 0);
	uiWindowSetMargined(w, 1);
	uiWindowOnClosing(w, onClosing, NULL);

	group = uiNewGroup("Account");
	uiGroupSetMargined(group, 1);
	uiWindowSetChild(w, uiControl(group));

	form = uiNewForm();
	uiFormSetPadded(form, 1);
	uiGroupSetChild(group, uiControl(form));

	uiFormAppend(form, "Name", uiControl(uiNewEntry()), 0);
	uiFormAppend(form, "Email", uiControl(uiNewEntry()), 0);

	uiControlShow(uiControl(w));
	uiMain();
	uiUninit();
	return 0;
}
