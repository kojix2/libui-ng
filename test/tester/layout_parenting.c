#include "test.h"

static uiBox *mainBox;
static uiTab *mainTab;
static uiBox *properties;

static uiLabel *movingLabel;
static uiBox *movingBoxes[2];
static int movingCurrent;

static void moveLabel(uiButton *b, void *data)
{
	int from, to;

	from = movingCurrent;
	to = 0;
	if (from == 0)
		to = 1;
	uiBoxDelete(movingBoxes[from], 0);
	uiBoxAppend(movingBoxes[to], uiControl(movingLabel), 0);
	movingCurrent = to;
}

static int moveBack;
#define moveOutText "Move Properties Out"
#define moveBackText "Move Properties Back"

void selectParenting(uiButton *b, void *data)
{
	uiTabSetSelected(uiTab(data), moveBack ? 0 : 1);
}

static void moveProperties(uiButton *b, void *data)
{
	if (moveBack) {
		uiBoxDelete(mainBox, 1);
		uiTabInsertAt(mainTab, "Properties", 0, uiControl(properties));
		uiButtonSetText(b, moveOutText);
		moveBack = 0;
		return;
	}
	uiTabDelete(mainTab, 0);
	uiBoxAppend(mainBox, uiControl(properties), 1);
	uiButtonSetText(b, moveBackText);
	moveBack = 1;
}

uiBox *makeLayoutParenting(uiBox *host, uiTab *tabs, uiBox *movable, uiGroup *group)
{
	uiBox *page, *vbox, *hbox;
	uiButton *button;

	// Properties is the first tab; moving it exercises tab/box reparenting.
	mainBox = host;
	mainTab = tabs;
	properties = movable;
	page = newVerticalBox();
	uiBoxAppend(page, uiControl(group), 0);
	vbox = newVerticalBox();
	uiGroupSetChild(group, uiControl(vbox));

	hbox = newHorizontalBox();
	button = uiNewButton("Move the Label!");
	uiButtonOnClicked(button, moveLabel, NULL);
	uiBoxAppend(hbox, uiControl(button), 1);
	// have a blank label for space
	uiBoxAppend(hbox, uiControl(uiNewLabel("")), 1);
	uiBoxAppend(vbox, uiControl(hbox), 0);

	hbox = newHorizontalBox();
	movingBoxes[0] = newVerticalBox();
	uiBoxAppend(hbox, uiControl(movingBoxes[0]), 1);
	movingBoxes[1] = newVerticalBox();
	uiBoxAppend(hbox, uiControl(movingBoxes[1]), 1);
	uiBoxAppend(vbox, uiControl(hbox), 0);

	movingCurrent = 0;
	movingLabel = uiNewLabel("This label moves!");
	uiBoxAppend(movingBoxes[movingCurrent], uiControl(movingLabel), 0);

	hbox = newHorizontalBox();
	button = uiNewButton(moveOutText);
	uiButtonOnClicked(button, moveProperties, NULL);
	uiBoxAppend(hbox, uiControl(button), 0);
	uiBoxAppend(page, uiControl(hbox), 0);
	moveBack = 0;

	return page;
}
