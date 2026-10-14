#include <stdio.h>
#include <ui.h>

static uiWindow *mainwin;
static uiLabel *resultLabel;

static void showPaths(char **paths)
{
	size_t i;
	char summary[64];

	if (paths == NULL) {
		uiLabelSetText(resultLabel, "Cancelled");
		return;
	}
	for (i = 0; paths[i] != NULL; i++)
		printf("%s\n", paths[i]);
	snprintf(summary, sizeof summary, "%zu selected (see stdout)", i);
	uiLabelSetText(resultLabel, summary);
	uiFreeTextArray(paths);
}

static void onOpenFiles(uiButton *button, void *data)
{
	showPaths(uiOpenFiles(mainwin));
}

static void onOpenFolders(uiButton *button, void *data)
{
	showPaths(uiOpenFolders(mainwin));
}

static int onClosing(uiWindow *w, void *data)
{
	uiQuit();
	return 1;
}

int main(void)
{
	uiInitOptions options = {0};
	const char *error;
	uiBox *box;
	uiButton *button;

	error = uiInit(&options);
	if (error != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", error);
		uiFreeInitError(error);
		return 1;
	}
	mainwin = uiNewWindow("Open Multiple", 480, 180, 0);
	uiWindowSetMargined(mainwin, 1);
	uiWindowOnClosing(mainwin, onClosing, NULL);
	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	uiWindowSetChild(mainwin, uiControl(box));
	resultLabel = uiNewLabel("Select files or folders");
	uiBoxAppend(box, uiControl(resultLabel), 0);
	button = uiNewButton("Open Files");
	uiButtonOnClicked(button, onOpenFiles, NULL);
	uiBoxAppend(box, uiControl(button), 0);
	button = uiNewButton("Open Folders");
	uiButtonOnClicked(button, onOpenFolders, NULL);
	uiBoxAppend(box, uiControl(button), 0);
	uiControlShow(uiControl(mainwin));
	uiMain();
	uiUninit();
	return 0;
}
