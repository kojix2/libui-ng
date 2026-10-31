#include "test.h"

static void echoReadOnlyText(uiEntry *e, void *data)
{
	char *text;

	text = uiEntryText(e);
	uiEntrySetText(uiEntry(data), text);
	uiFreeText(text);
}

uiBox *makeEntries(void)
{
	uiBox *page;
	uiEntry *entry, *readonly;

	page = newVerticalBox();

	entry = uiNewEntry();
	readonly = uiNewEntry();
	uiEntryOnChanged(entry, echoReadOnlyText, readonly);
	uiEntrySetText(readonly, "If you can see this, uiEntryReadOnly() isn't working properly.");
	uiEntrySetReadOnly(readonly, 1);
	if (uiEntryReadOnly(readonly))
		uiEntrySetText(readonly, "");
	uiBoxAppend(page, uiControl(uiNewLabel("Editable (type here)")), 0);
	uiBoxAppend(page, uiControl(entry), 0);
	uiBoxAppend(page, uiControl(uiNewLabel("Read-only echo")), 0);
	uiBoxAppend(page, uiControl(readonly), 0);

	return page;
}
