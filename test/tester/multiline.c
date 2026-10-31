// 22 may 2016
#include "test.h"

// TODO OS X: if the hboxes are empty, the text views don't show up

static void meChanged(uiMultilineEntry *e, void *data)
{
	printf("%s changed\n", (char *) data);
}

static void setClicked(uiButton *b, void *data)
{
	uiMultilineEntrySetText(uiMultilineEntry(data), "set");
}

static void appendClicked(uiButton *b, void *data)
{
	uiMultilineEntryAppend(uiMultilineEntry(data), "append\n");
}

static uiBox *half(uiMultilineEntry *(*mk)(void), const char *which)
{
	uiBox *vbox, *hbox;
	uiMultilineEntry *me;
	uiButton *button;

	vbox = newVerticalBox();
	uiBoxAppend(vbox, uiControl(uiNewLabel(which)), 0);

	me = (*mk)();
	uiMultilineEntryOnChanged(me, meChanged, (void *) which);
	uiBoxAppend(vbox, uiControl(me), 1);

	hbox = newHorizontalBox();
	uiBoxAppend(vbox, uiControl(hbox), 0);

	button = uiNewButton("Set");
	uiButtonOnClicked(button, setClicked, me);
	uiBoxAppend(hbox, uiControl(button), 0);

	button = uiNewButton("Append");
	uiButtonOnClicked(button, appendClicked, me);
	uiBoxAppend(hbox, uiControl(button), 0);

	return vbox;
}

uiBox *makeMultiline(void)
{
	uiBox *page;
	uiBox *b;

	page = newHorizontalBox();

	b = half(uiNewMultilineEntry, "wrap");
	uiBoxAppend(page, uiControl(b), 1);
	b = half(uiNewNonWrappingMultilineEntry, "no wrap");
	uiBoxAppend(page, uiControl(b), 1);

	return page;
}
