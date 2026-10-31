// 22 april 2015
#include "test.h"

// TODOs
// - blank page affects menus negatively on Windows

void die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "[test program] ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	abort();
}

int onClosing(uiWindow *w, void *data)
{
	printf("in onClosing()\n");
	uiQuit();
	return 1;
}

int onShouldQuit(void *data)
{
	printf("in onShouldQuit()\n");
	if (uiMenuItemChecked(shouldQuitItem)) {
		uiControlDestroy(uiControl(data));
		return 1;
	}
	return 0;
}

static void onTabSelected(uiTab *t , void* data) {
	int index = uiTabSelected(t);
	printf("Tab with index %d selected\n", index);
}

uiBox *(*newhbox)(void);
uiBox *(*newvbox)(void);

int main(int argc, char *argv[])
{
	uiInitOptions o;
	int i;
	const char *err;
	uiWindow *w;
	uiBox *mainBox;
	uiBox *properties;
	uiGroup *movingGroup;
	uiTab *categories, *controls, *layout, *drawing, *windows, *tables;
	int nomenus = 0;
	int startspaced = 0;
	int steps = 0;

	newhbox = uiNewHorizontalBox;
	newvbox = uiNewVerticalBox;

	memset(&o, 0, sizeof (uiInitOptions));
	for (i = 1; i < argc; i++)
		if (strcmp(argv[i], "nomenus") == 0)
			nomenus = 1;
		else if (strcmp(argv[i], "startspaced") == 0)
			startspaced = 1;
		else if (strcmp(argv[i], "swaphv") == 0) {
			newhbox = uiNewVerticalBox;
			newvbox = uiNewHorizontalBox;
		} else if (strcmp(argv[i], "steps") == 0)
			steps = 1;
		else {
			fprintf(stderr, "%s: unrecognized option %s\n", argv[0], argv[i]);
			return 1;
		}

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "error initializing ui: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	if (!nomenus)
		initMenus();

	w = newWindow("Main Window", 320, 240, 1);
	uiWindowOnClosing(w, onClosing, NULL);
	printf("main window %p\n", (void *) w);

	uiOnShouldQuit(onShouldQuit, w);

	mainBox = newHorizontalBox();
	uiWindowSetChild(w, uiControl(mainBox));

	categories = newTab();
	uiBoxAppend(mainBox, uiControl(categories), 1);

	controls = newTab();
	uiTabAppend(categories, "Controls", uiControl(controls));
	uiTabAppend(controls, "Entries", uiControl(makeEntries()));
	uiTabAppend(controls, "Multiline", uiControl(makeMultiline()));
	uiTabAppend(controls, "Numbers", uiControl(makeNumbers()));
	uiTabAppend(controls, "Selection", uiControl(makeSelection()));
	uiTabAppend(controls, "Date/Time", uiControl(makeDateTime()));
	uiTabAppend(controls, "State", uiControl(makeControlState()));

	layout = newTab();
	uiTabAppend(categories, "Layout", uiControl(layout));
	// Keep Properties first: Parenting removes and reinserts this tab.
	movingGroup = newGroup("Moving Label");
	properties = makeProperties(w, movingGroup, layout);
	uiTabAppend(layout, "Properties", uiControl(properties));
	uiTabAppend(layout, "Parenting", uiControl(makeLayoutParenting(
		mainBox, layout, properties, movingGroup)));
	uiTabAppend(layout, "Visibility", uiControl(makeLayoutVisibility()));
	uiTabAppend(layout, "Sizes", uiControl(makeLayoutSizes()));
	uiTabAppend(layout, "Form", uiControl(makeLayoutForm()));
	uiTabAppend(layout, "Grid", uiControl(makeLayoutGrid()));
	uiTabAppend(layout, "Empty", uiControl(uiNewHorizontalBox()));

	drawing = newTab();
	uiTabAppend(categories, "Drawing", uiControl(drawing));
	uiTabAppend(drawing, "Area", uiControl(makeArea()));
	uiTabAppend(drawing, "Arc", uiControl(makeAreaArc()));
	uiTabAppend(drawing, "Mouse", uiControl(makeAreaMouse()));
	uiTabAppend(drawing, "Scroll", uiControl(makeAreaScroll()));
	uiTabAppend(drawing, "Text", uiControl(makeText()));

	windows = newTab();
	uiTabAppend(categories, "Windows", uiControl(windows));
	uiTabAppend(windows, "Geometry & Focus", uiControl(makeWindows(w)));
	uiTabAppend(windows, "Dialogs", uiControl(makeDialogs(w)));

	tables = newTab();
	uiTabAppend(categories, "Tables", uiControl(tables));
	uiTabAppend(tables, "Columns & Selection", uiControl(makeTable()));
	uiTabAppend(tables, "Model Updates", uiControl(makeTableModel()));

	uiTabOnSelected(categories, onTabSelected, NULL);

	if (startspaced)
		setSpaced(1);

	uiControlShow(uiControl(w));
	if (!steps)
		uiMain();
	else {
		uiMainSteps();
		while (uiMainStep(1))
			;
	}
	printf("after uiMain()\n");
	freeLayoutForm();
	freeTableModel();
	freeTable();
	uiUninit();
	printf("after uiUninit()\n");
	return 0;
}
