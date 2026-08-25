#include "../ui.h"
#include "uipriv.h"
#include "toolbar.h"

static char *toolbarStrdup(const char *s)
{
	size_t n;
	char *copy;

	n = strlen(s) + 1;
	copy = uiprivAlloc(n, "char[]");
	memcpy(copy, s, n);
	return copy;
}

static void defaultOnClicked(uiToolbarItem *item, void *data)
{
	/* do nothing */
}

static int validDisplayMode(uiToolbarDisplayMode mode)
{
	switch (mode) {
	case uiToolbarDisplayModeIconOnly:
	case uiToolbarDisplayModeIconAndTextHorizontal:
	case uiToolbarDisplayModeIconAndTextVertical:
	case uiToolbarDisplayModeTextOnly:
		return 1;
	}
	return 0;
}

static uiToolbarItem *appendItem(uiToolbar *t, uiprivToolbarItemType type,
	const char *text, uiImage *icon)
{
	uiToolbarItem *item;

	if (t == NULL) {
		uiprivUserBug("You cannot append an item to a NULL uiToolbar.");
		return NULL;
	}
	if (t->sealed) {
		uiprivUserBug("You cannot append items after a uiToolbar has been attached to a window.");
		return NULL;
	}
	if (text == NULL) {
		uiprivUserBug("You cannot append a uiToolbar button with NULL text.");
		return NULL;
	}
	item = uiprivNew(uiToolbarItem);
	item->toolbar = t;
	item->type = type;
	item->text = toolbarStrdup(text);
	item->tooltip = toolbarStrdup("");
	item->icon = icon;
	item->enabled = 1;
	item->onClicked = defaultOnClicked;
	if (t->len == t->cap) {
		size_t cap = t->cap == 0 ? 8 : t->cap * 2;
		t->items = uiprivRealloc(t->items,
			cap * sizeof (uiToolbarItem *), "uiToolbarItem *[]");
		t->cap = cap;
	}
	t->items[t->len++] = item;
	return item;
}

uiToolbar *uiNewToolbar(void)
{
	uiToolbar *t;

	t = uiprivNew(uiToolbar);
	t->displayMode = uiToolbarDisplayModeIconAndTextVertical;
	uiprivToolbarPlatformNew(t);
	return t;
}

uiToolbarDisplayMode uiToolbarGetDisplayMode(uiToolbar *t)
{
	return t->displayMode;
}

void uiToolbarSetDisplayMode(uiToolbar *t, uiToolbarDisplayMode mode)
{
	if (t == NULL) {
		uiprivUserBug("You cannot set the display mode of a NULL uiToolbar.");
		return;
	}
	if (t->sealed) {
		uiprivUserBug("You cannot set the display mode after a uiToolbar has been attached to a window.");
		return;
	}
	if (!validDisplayMode(mode)) {
		uiprivUserBug("Invalid uiToolbar display mode %u.", mode);
		return;
	}
	t->displayMode = mode;
}

static void freeToolbar(void *p)
{
	uiToolbar *t = p;
	size_t i;

	uiprivToolbarPlatformFree(t);
	for (i = 0; i < t->len; i++) {
		uiToolbarItem *item = t->items[i];
		uiprivFree(item->tooltip);
		uiprivFree(item->text);
		uiprivFree(item);
	}
	if (t->items != NULL)
		uiprivFree(t->items);
	uiprivFree(t);
}

void uiFreeToolbar(uiToolbar *t)
{
	if (t == NULL) {
		uiprivUserBug("You cannot free a NULL uiToolbar.");
		return;
	}
	if (t->window != NULL) {
		uiprivUserBug("You cannot free a uiToolbar while it is attached to a window.");
		return;
	}
	if (uiprivUserCallbackDeferFree(t, freeToolbar))
		return;
	freeToolbar(t);
}

uiToolbarItem *uiToolbarAppendButton(uiToolbar *t, const char *text, uiImage *icon)
{
	return appendItem(t, uiprivToolbarItemButton, text, icon);
}

uiToolbarItem *uiToolbarAppendToggleButton(uiToolbar *t, const char *text, uiImage *icon)
{
	return appendItem(t, uiprivToolbarItemToggleButton, text, icon);
}

void uiToolbarAppendSeparator(uiToolbar *t)
{
	(void) appendItem(t, uiprivToolbarItemSeparator, "", NULL);
}

void uiToolbarAppendSpace(uiToolbar *t)
{
	(void) appendItem(t, uiprivToolbarItemSpace, "", NULL);
}

void uiToolbarAppendFlexibleSpace(uiToolbar *t)
{
	(void) appendItem(t, uiprivToolbarItemFlexibleSpace, "", NULL);
}

char *uiToolbarItemText(uiToolbarItem *item)
{
	return uiprivToolbarPlatformDupText(item->text);
}

void uiToolbarItemSetText(uiToolbarItem *item, const char *text)
{
	if (text == NULL) {
		uiprivUserBug("You cannot set a uiToolbarItem's text to NULL.");
		return;
	}
	uiprivFree(item->text);
	item->text = toolbarStrdup(text);
	uiprivToolbarPlatformSyncItem(item);
}

void uiToolbarItemSetIcon(uiToolbarItem *item, uiImage *icon)
{
	item->icon = icon;
	uiprivToolbarPlatformSyncItemIcon(item);
}

char *uiToolbarItemTooltip(uiToolbarItem *item)
{
	return uiprivToolbarPlatformDupText(item->tooltip);
}

void uiToolbarItemSetTooltip(uiToolbarItem *item, const char *tooltip)
{
	if (tooltip == NULL) {
		uiprivUserBug("You cannot set a uiToolbarItem's tooltip to NULL.");
		return;
	}
	uiprivFree(item->tooltip);
	item->tooltip = toolbarStrdup(tooltip);
	uiprivToolbarPlatformSyncItem(item);
}

int uiToolbarItemEnabled(uiToolbarItem *item)
{
	return item->enabled;
}

void uiToolbarItemEnable(uiToolbarItem *item)
{
	item->enabled = 1;
	uiprivToolbarPlatformSyncItem(item);
}

void uiToolbarItemDisable(uiToolbarItem *item)
{
	item->enabled = 0;
	uiprivToolbarPlatformSyncItem(item);
}

void uiToolbarItemOnClicked(uiToolbarItem *item,
	void (*f)(uiToolbarItem *, void *), void *data)
{
	if (f == NULL) {
		uiprivUserBug("You cannot set a uiToolbarItem callback to NULL.");
		return;
	}
	item->onClicked = f;
	item->onClickedData = data;
}

int uiToolbarItemChecked(uiToolbarItem *item)
{
	if (item->type != uiprivToolbarItemToggleButton) {
		uiprivUserBug("You can only get the checked state of a toggle uiToolbarItem.");
		return 0;
	}
	return item->checked;
}

void uiToolbarItemSetChecked(uiToolbarItem *item, int checked)
{
	if (item->type != uiprivToolbarItemToggleButton) {
		uiprivUserBug("You can only set the checked state of a toggle uiToolbarItem.");
		return;
	}
	item->checked = checked != 0;
	uiprivToolbarPlatformSyncItem(item);
}

int uiprivToolbarCanAttach(uiToolbar *t, uiWindow *w)
{
	if (t == NULL)
		return 1;
	if (t->window != NULL && t->window != w) {
		uiprivUserBug("A uiToolbar cannot be attached to more than one window.");
		return 0;
	}
	return 1;
}

void uiprivToolbarAttach(uiToolbar *t, uiWindow *w)
{
	t->window = w;
	t->sealed = 1;
	uiprivToolbarPlatformAttach(t, w);
}

void uiprivToolbarDetach(uiToolbar *t, uiWindow *w)
{
	if (t == NULL)
		return;
	uiprivToolbarPlatformDetach(t, w);
	t->window = NULL;
}

void uiprivToolbarItemClicked(uiToolbarItem *item, int checked)
{
	if (item->type == uiprivToolbarItemToggleButton)
		item->checked = checked != 0;
	if (!uiprivUserCallbackEnter(NULL))
		return;
	(*(item->onClicked))(item, item->onClickedData);
	uiprivUserCallbackLeave();
}
