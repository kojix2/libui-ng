#include "test.h"

static uiControl *children[3];
static int deleted;

static void entryChanged(uiEntry *e, void *data)
{
	char *text;

	text = uiEntryText(e);
	printf("%s entry changed: %s\n", (const char *) data, text);
	uiFreeText(text);
}

static void showHide(uiButton *b, void *data)
{
	uiControl *c = uiControl(data);

	if (uiControlVisible(c))
		uiControlHide(c);
	else
		uiControlShow(c);
}

static void deleteFirst(uiButton *b, void *data)
{
	uiForm *f = uiForm(data);

	if (uiFormNumChildren(f) == 0)
		return;
	// Removal transfers ownership here; keep the child alive for Show/Hide.
	uiFormDelete(f, 0);
	deleted++;
}

uiBox *makeLayoutForm(void)
{
	uiBox *page;
	uiForm *f;
	uiEntry *e;
	uiButton *b;

	page = newVerticalBox();
	deleted = 0;

	f = newForm();

	e = uiNewPasswordEntry();
	children[0] = uiControl(e);
	uiEntryOnChanged(e, entryChanged, "password");
	uiFormAppend(f, "Password Entry", uiControl(e), 0);

	e = uiNewSearchEntry();
	children[1] = uiControl(e);
	uiEntryOnChanged(e, entryChanged, "search");
	uiFormAppend(f, "Search Box", uiControl(e), 0);

	children[2] = uiControl(uiNewMultilineEntry());
	uiFormAppend(f, "MLE", children[2], 1);

	b = uiNewButton("Show/Hide");
	uiButtonOnClicked(b, showHide, e);
	uiBoxAppend(page, uiControl(b), 0);
	b = uiNewButton("Delete First");
	uiButtonOnClicked(b, deleteFirst, f);
	uiBoxAppend(page, uiControl(b), 0);
	uiBoxAppend(page, uiControl(f), 1);

	return page;
}

void freeLayoutForm(void)
{
	int i;

	// The window destroys attached children; only removed children remain ours.
	for (i = 0; i < deleted; i++)
		uiControlDestroy(children[i]);
}
