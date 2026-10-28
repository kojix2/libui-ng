#include <stdio.h>
#include <string.h>
#include "ui.h"

static uiWindow *window;
static uiLabel *status;
static uiArea *zone;
static int hovering;

static uiDropResult dropped(const uiDropInfo *info, const uiDropData *payload, void *data)
{
	char message[160];

	if (info->Format == uiDropFormatFiles) {
		size_t count = payload->FileCount;
		snprintf(message, sizeof message, "%zu paths at (%.1f, %.1f)",
			count, info->X, info->Y);
		for (size_t i = 0; i < count; i++)
			printf("%s\n", payload->Files[i]);
	} else {
		// uiLabelSetText copies the borrowed string before this call returns.
		uiLabelSetText(status, payload->Text);
		return uiDropAccepted;
	}
	uiLabelSetText(status, message);
	return uiDropAccepted;
}

static uiDropAction over(const uiDropInfo *info, void *data)
{
	hovering = 1;
	uiAreaQueueRedrawAll(zone);
	return uiDropActionCopy;
}
static void leave(void *data)
{
	hovering = 0;
	uiAreaQueueRedrawAll(zone);
}
static void draw(uiAreaHandler *handler, uiArea *area, uiAreaDrawParams *p)
{
	uiDrawBrush brush;
	uiDrawPath *path = uiDrawNewPath(uiDrawFillModeWinding);
	memset(&brush, 0, sizeof brush);
	brush.Type = uiDrawBrushTypeSolid;
	brush.R = hovering ? 0.3 : 0.85;
	brush.G = hovering ? 0.7 : 0.9;
	brush.B = 0.95;
	brush.A = 1;
	uiDrawPathAddRectangle(path, 0, 0, p->AreaWidth, p->AreaHeight);
	uiDrawPathEnd(path);
	uiDrawFill(p->Context, path, &brush);
	uiDrawFreePath(path);
}
static void mouse(uiAreaHandler *h, uiArea *a, uiAreaMouseEvent *e) {}
static void crossed(uiAreaHandler *h, uiArea *a, int left) {}
static void broken(uiAreaHandler *h, uiArea *a) {}
static int key(uiAreaHandler *h, uiArea *a, uiAreaKeyEvent *e) { return 0; }
static int closing(uiWindow *w, void *data) { uiQuit(); return 1; }

int main(void)
{
	uiInitOptions options = {0};
	uiAreaHandler areaHandler = {draw, mouse, crossed, broken, key, NULL};
	uiDropHandler dropHandler = {0};
	uiBox *box;
	uiEntry *entry;
	const char *error = uiInit(&options);
	if (error != NULL) {
		fprintf(stderr, "%s\n", error);
		uiFreeInitError(error);
		return 1;
	}
	window = uiNewWindow("Drag files or text here", 520, 360, 0);
	uiWindowOnClosing(window, closing, NULL);
	uiWindowSetMargined(window, 1);
	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	status = uiNewLabel("Drop files or text onto the blue area or window background.");
	uiBoxAppend(box, uiControl(status), 0);
	entry = uiNewEntry();
	uiEntrySetText(entry, "This entry keeps its native text drop behavior.");
	uiBoxAppend(box, uiControl(entry), 0);
	zone = uiNewArea(&areaHandler);
	uiBoxAppend(box, uiControl(zone), 1);
	uiWindowSetChild(window, uiControl(box));
	dropHandler.Size = sizeof dropHandler;
	dropHandler.Drop = dropped;
	if (uiWindowSetDropHandler(window, uiDropFormatText | uiDropFormatFiles,
		&dropHandler, NULL) != uiDropStatusOK)
		goto failed;
	dropHandler.Over = over;
	dropHandler.Leave = leave;
	if (uiAreaSetDropHandler(zone, uiDropFormatText | uiDropFormatFiles,
		&dropHandler, NULL) != uiDropStatusOK)
		goto failed;
	uiControlShow(uiControl(window));
	uiMain();
	uiUninit();
	return 0;
failed:
	fprintf(stderr, "Could not register drop destination\n");
	uiControlDestroy(uiControl(window));
	uiUninit();
	return 1;
}
