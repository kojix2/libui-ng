#include <stdio.h>
#include <ui.h>

static uiWindow *mainwin;
static uiLabel *label;

static void onCheckItemClicked(uiMenuItem *item, uiWindow *w, void *data)
{
	if (uiMenuItemChecked(item))
		uiLabelSetText(label, "Menu check item is checked");
	else
		uiLabelSetText(label, "Menu check item is not checked");
}

static int onShouldQuit(void *data)
{
	uiControlDestroy(uiControl(mainwin));
	return 1;
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
	uiMenu *viewMenu;
	uiMenuItem *item;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	viewMenu = uiNewMenu("View");
	item = uiMenuAppendCheckItem(viewMenu, "Show Option");
	uiMenuItemSetChecked(item, 1);
	uiMenuItemOnClicked(item, onCheckItemClicked, NULL);
	uiMenuAppendSeparator(viewMenu);
	uiMenuAppendQuitItem(viewMenu);

	uiOnShouldQuit(onShouldQuit, NULL);

	mainwin = uiNewWindow("Menu Checkbox", 360, 160, 1);
	uiWindowSetMargined(mainwin, 1);
	uiWindowOnClosing(mainwin, onClosing, NULL);

	label = uiNewLabel("Menu check item is checked");
	uiWindowSetChild(mainwin, uiControl(label));

	uiControlShow(uiControl(mainwin));
	uiMain();
	uiUninit();
	return 0;
}
