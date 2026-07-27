#include <stdio.h>
#include <ui.h>

static uiWindow *mainwin;
static uiLabel *label;

static void onNewClicked(uiMenuItem *item, uiWindow *w, void *data)
{
	uiLabelSetText(label, "New menu item clicked");
}

static void onAboutClicked(uiMenuItem *item, uiWindow *w, void *data)
{
	uiMsgBox(w, "About", "This menu was created with libui-ng.");
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
	uiMenu *fileMenu;
	uiMenu *helpMenu;
	uiMenuItem *item;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	fileMenu = uiNewMenu("File");
	item = uiMenuAppendItem(fileMenu, "New");
	uiMenuItemOnClicked(item, onNewClicked, NULL);
	uiMenuAppendSeparator(fileMenu);
	uiMenuAppendQuitItem(fileMenu);

	helpMenu = uiNewMenu("Help");
	item = uiMenuAppendAboutItem(helpMenu);
	uiMenuItemOnClicked(item, onAboutClicked, NULL);

	uiOnShouldQuit(onShouldQuit, NULL);

	mainwin = uiNewWindow("Menu", 360, 160, 1);
	uiWindowSetMargined(mainwin, 1);
	uiWindowOnClosing(mainwin, onClosing, NULL);

	label = uiNewLabel("Use the menu bar.");
	uiWindowSetChild(mainwin, uiControl(label));

	uiControlShow(uiControl(mainwin));
	uiMain();
	uiUninit();
	return 0;
}
