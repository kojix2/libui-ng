#include <stdio.h>
#include <ui.h>

static uiWindow *mainwin;

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

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	fileMenu = uiNewMenu("File");
	uiMenuAppendQuitItem(fileMenu);

	uiOnShouldQuit(onShouldQuit, NULL);

	mainwin = uiNewWindow("Should Quit", 360, 140, 1);
	uiWindowSetMargined(mainwin, 1);
	uiWindowOnClosing(mainwin, onClosing, NULL);
	uiWindowSetChild(mainwin, uiControl(uiNewLabel("Use File > Quit.")));

	uiControlShow(uiControl(mainwin));
	uiMain();
	uiUninit();
	return 0;
}
