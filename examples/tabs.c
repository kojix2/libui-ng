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
	uiTab *tab;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("Tabs", 360, 220, 0);
	uiWindowSetMargined(w, 1);
	uiWindowOnClosing(w, onClosing, NULL);

	tab = uiNewTab();
	uiWindowSetChild(w, uiControl(tab));

	uiTabAppend(tab, "One", uiControl(uiNewLabel("First page")));
	uiTabSetMargined(tab, 0, 1);

	uiTabAppend(tab, "Two", uiControl(uiNewLabel("Second page")));
	uiTabSetMargined(tab, 1, 1);

	uiControlShow(uiControl(w));
	uiMain();
	uiUninit();
	return 0;
}
