// 24 april 2015
#include "uipriv_windows.hpp"
#include <cstdlib>
#include "../common/menushortcut.h"
#include <vector>
#include <string>

// LONGTERM migrate to std::vector

static uiMenu **menus = NULL;
static size_t len = 0;
static size_t cap = 0;
static BOOL menusFinalized = FALSE;
static BOOL shortcutsFinalized = FALSE;
static HACCEL accelerators = NULL;
enum {
	firstMenuID = 100,			// start somewhere safe
	maxMenuID = 0xFFFF,			// WM_COMMAND only carries the low WORD
};

static UINT curID = firstMenuID;
static BOOL hasQuit = FALSE;
static BOOL hasPreferences = FALSE;
static BOOL hasAbout = FALSE;

struct uiMenu {
	WCHAR *name;
	uiMenuItem **items;
	size_t len;
	size_t cap;
};

struct uiMenuItem {
	WCHAR *name;
	int type;
	UINT id;
	void (*onClicked)(uiMenuItem *, uiWindow *, void *);
	void *onClickedData;
	BOOL disabled;				// template for new instances; kept in sync with everything else
	BOOL checked;
	unsigned int shortcutKey;
	uiModifiers shortcutModifiers;
	HMENU *hmenus;
	size_t len;
	size_t cap;
};

enum {
	typeRegular,
	typeCheckbox,
	typeQuit,
	typePreferences,
	typeAbout,
	typeSeparator,
};

static WORD shortcutVirtualKey(unsigned int key)
{
	if (key >= 'A' && key <= 'Z')
		return (WORD) key;
	if (key >= '0' && key <= '9')
		return (WORD) key;
	if (key >= uiExtKeyF1 && key <= uiExtKeyF12)
		return (WORD) (VK_F1 + key - uiExtKeyF1);
	switch (key) {
	case uiExtKeyDelete: return VK_DELETE;
	case uiExtKeyUp: return VK_UP;
	case uiExtKeyDown: return VK_DOWN;
	case uiExtKeyLeft: return VK_LEFT;
	case uiExtKeyRight: return VK_RIGHT;
	default: return 0;
	}
}

static BYTE shortcutFlags(uiModifiers modifiers)
{
	BYTE flags = FVIRTKEY;
	modifiers = uiprivShortcutEffectiveModifiers(modifiers);
	if (modifiers & uiModifierCtrl) flags |= FCONTROL;
	if (modifiers & uiModifierAlt) flags |= FALT;
	if (modifiers & uiModifierShift) flags |= FSHIFT;
	return flags;
}

void uiMenuItemSetShortcut(uiMenuItem *item, unsigned int key,
	uiModifiers modifiers)
{
	size_t i, j;
	uiMenuItem *other;

	if (shortcutsFinalized || menusFinalized)
		uiprivUserBug("Menu shortcuts must be set before the first window is created.");
	if (item->type == typeSeparator)
		uiprivUserBug("A separator cannot have a menu shortcut.");
	uiprivValidateMenuShortcut(key, modifiers);
	modifiers = uiprivShortcutEffectiveModifiers(modifiers);
	if (key != uiExtKeyNone)
		for (i = 0; i < len; i++)
			for (j = 0; j < menus[i]->len; j++) {
				other = menus[i]->items[j];
				if (other != item && other->shortcutKey == key &&
					other->shortcutModifiers == modifiers)
					uiprivUserBug("Duplicate menu shortcut.");
			}
	item->shortcutKey = key;
	item->shortcutModifiers = modifiers;
}

void freezeMenuShortcuts(void)
{
	std::vector<ACCEL> entries;
	ACCEL entry;
	size_t i, j;
	uiMenuItem *item;

	if (shortcutsFinalized)
		return;
	shortcutsFinalized = TRUE;
	for (i = 0; i < len; i++)
		for (j = 0; j < menus[i]->len; j++) {
			item = menus[i]->items[j];
			if (item->shortcutKey == uiExtKeyNone)
				continue;
			entry.fVirt = shortcutFlags(item->shortcutModifiers);
			entry.key = shortcutVirtualKey(item->shortcutKey);
			entry.cmd = (WORD) item->id;
			entries.push_back(entry);
		}
	if (entries.size() > 32767)
		uiprivUserBug("Too many menu shortcuts for a Windows accelerator table.");
	if (!entries.empty()) {
		accelerators = CreateAcceleratorTableW(entries.data(), (int) entries.size());
		if (accelerators == NULL)
			logLastError(L"error creating menu accelerator table");
	}
}

HACCEL menuAccelerators(void)
{
	return accelerators;
}

static std::wstring shortcutLabel(uiMenuItem *item)
{
	std::wstring label(item->name);
	uiModifiers modifiers = item->shortcutModifiers;
	unsigned int key = item->shortcutKey;
	if (key == uiExtKeyNone)
		return label;
	label += L"\t";
	if (modifiers & uiModifierCtrl) label += L"Ctrl+";
	if (modifiers & uiModifierAlt) label += L"Alt+";
	if (modifiers & uiModifierShift) label += L"Shift+";
	if (key >= 'A' && key <= 'Z')
		label += (WCHAR) key;
	else if (key >= '0' && key <= '9')
		label += (WCHAR) key;
	else if (key >= uiExtKeyF1 && key <= uiExtKeyF12)
		label += L"F" + std::to_wstring(1 + key - uiExtKeyF1);
	else {
		switch (key) {
		case uiExtKeyDelete: label += L"Delete"; break;
		case uiExtKeyUp: label += L"Up"; break;
		case uiExtKeyDown: label += L"Down"; break;
		case uiExtKeyLeft: label += L"Left"; break;
		case uiExtKeyRight: label += L"Right"; break;
		}
	}
	return label;
}

#define grow 32

static void sync(uiMenuItem *item)
{
	size_t i;
	MENUITEMINFOW mi;

	ZeroMemory(&mi, sizeof (MENUITEMINFOW));
	mi.cbSize = sizeof (MENUITEMINFOW);
	mi.fMask = MIIM_STATE;
	if (item->disabled)
		mi.fState |= MFS_DISABLED;
	if (item->checked)
		mi.fState |= MFS_CHECKED;

	for (i = 0; i < item->len; i++)
		if (SetMenuItemInfo(item->hmenus[i], item->id, FALSE, &mi) == 0) {
			logLastError(L"error synchronizing menu items");
			std::abort();		// native and registered item state must agree
		}
}

static void defaultOnClicked(uiMenuItem *item, uiWindow *w, void *data)
{
	// do nothing
}

static void onQuitClicked(uiMenuItem *item, uiWindow *w, void *data)
{
	if (uiprivShouldQuit())
		uiQuit();
}

void uiMenuItemEnable(uiMenuItem *i)
{
	i->disabled = FALSE;
	sync(i);
}

void uiMenuItemDisable(uiMenuItem *i)
{
	i->disabled = TRUE;
	sync(i);
}

void uiMenuItemOnClicked(uiMenuItem *i, void (*f)(uiMenuItem *, uiWindow *, void *), void *data)
{
	if (i->type == typeQuit)
		uiprivUserBug("You can not call uiMenuItemOnClicked() on a Quit item; use uiOnShouldQuit() instead.");
	if (f == NULL)
		uiprivUserBug("uiMenuItemOnClicked() callback must not be NULL");
	i->onClicked = f;
	i->onClickedData = data;
}

int uiMenuItemChecked(uiMenuItem *i)
{
	return i->checked != FALSE;
}

void uiMenuItemSetChecked(uiMenuItem *i, int checked)
{
	// use explicit values
	i->checked = FALSE;
	if (checked)
		i->checked = TRUE;
	sync(i);
}

static uiMenuItem *newItem(uiMenu *m, int type, const char *name)
{
	uiMenuItem *item;

	if (menusFinalized)
		uiprivUserBug("You can not create a new menu item after menus have been finalized.");

	if (m->len >= m->cap) {
		m->cap += grow;
		m->items = (uiMenuItem **) uiprivRealloc(m->items, m->cap * sizeof (uiMenuItem *), "uiMenuitem *[]");
	}

	item = uiprivNew(uiMenuItem);

	m->items[m->len] = item;
	m->len++;

	item->type = type;
	switch (item->type) {
	case typeQuit:
		item->name = toUTF16("Quit");
		break;
	case typePreferences:
		item->name = toUTF16("Preferences...");
		break;
	case typeAbout:
		item->name = toUTF16("About");
		break;
	case typeSeparator:
		break;
	default:
		item->name = toUTF16(name);
		break;
	}

	if (item->type != typeSeparator) {
		if (curID > maxMenuID)
			uiprivUserBug("You can not create more than 65436 menu items on Windows.");
		item->id = curID;
		curID++;
	}

	if (item->type == typeQuit) {
		// can't call uiMenuItemOnClicked() here
		item->onClicked = onQuitClicked;
		item->onClickedData = NULL;
	} else
		uiMenuItemOnClicked(item, defaultOnClicked, NULL);

	return item;
}

uiMenuItem *uiMenuAppendItem(uiMenu *m, const char *name)
{
	return newItem(m, typeRegular, name);
}

uiMenuItem *uiMenuAppendCheckItem(uiMenu *m, const char *name)
{
	return newItem(m, typeCheckbox, name);
}

uiMenuItem *uiMenuAppendQuitItem(uiMenu *m)
{
	if (hasQuit)
		uiprivUserBug("You can not have multiple Quit menu items in a program.");
	hasQuit = TRUE;
	newItem(m, typeSeparator, NULL);
	return newItem(m, typeQuit, NULL);
}

uiMenuItem *uiMenuAppendPreferencesItem(uiMenu *m)
{
	if (hasPreferences)
		uiprivUserBug("You can not have multiple Preferences menu items in a program.");
	hasPreferences = TRUE;
	newItem(m, typeSeparator, NULL);
	return newItem(m, typePreferences, NULL);
}

uiMenuItem *uiMenuAppendAboutItem(uiMenu *m)
{
	if (hasAbout)
		uiprivUserBug("You can not have multiple About menu items in a program.");
	hasAbout = TRUE;
	newItem(m, typeSeparator, NULL);
	return newItem(m, typeAbout, NULL);
}

void uiMenuAppendSeparator(uiMenu *m)
{
	newItem(m, typeSeparator, NULL);
}

uiMenu *uiNewMenu(const char *name)
{
	uiMenu *m;

	if (menusFinalized)
		uiprivUserBug("You can not create a new menu after menus have been finalized.");
	if (len >= cap) {
		cap += grow;
		menus = (uiMenu **) uiprivRealloc(menus, cap * sizeof (uiMenu *), "uiMenu *[]");
	}

	m = uiprivNew(uiMenu);

	menus[len] = m;
	len++;

	m->name = toUTF16(name);

	return m;
}

static void appendMenuItem(HMENU menu, uiMenuItem *item)
{
	UINT uFlags;
	std::wstring label;

	uFlags = MF_SEPARATOR;
	if (item->type != typeSeparator) {
		uFlags = MF_STRING;
		if (item->disabled)
			uFlags |= MF_DISABLED | MF_GRAYED;
		if (item->checked)
			uFlags |= MF_CHECKED;
	}
	if (item->type != typeSeparator)
		label = shortcutLabel(item);
	if (AppendMenuW(menu, uFlags, item->id,
		item->type == typeSeparator ? NULL : label.c_str()) == 0) {
		logLastError(L"error appending menu item in appendMenuItem()");
		std::abort();		// the menu must match the registered item list
	}

	if (item->len >= item->cap) {
		item->cap += grow;
		item->hmenus = (HMENU *) uiprivRealloc(item->hmenus, item->cap * sizeof (HMENU), "HMENU[]");
	}
	item->hmenus[item->len] = menu;
	item->len++;
}

static HMENU makeMenu(uiMenu *m)
{
	HMENU menu;
	size_t i;

	menu = CreatePopupMenu();
	if (menu == NULL) {
		logLastError(L"error creating popup menu");
		std::abort();
	}
	for (i = 0; i < m->len; i++)
		appendMenuItem(menu, m->items[i]);
	return menu;
}

static void freeMenu(uiMenu *m, HMENU submenu);

HMENU makeMenubar(void)
{
	HMENU menubar;
	HMENU menu;
	size_t i;

	menusFinalized = TRUE;

	menubar = CreateMenu();
	if (menubar == NULL) {
		logLastError(L"error creating menubar");
		std::abort();
	}

	for (i = 0; i < len; i++) {
		menu = makeMenu(menus[i]);
		if (AppendMenuW(menubar, MF_POPUP | MF_STRING, (UINT_PTR) menu, menus[i]->name) == 0) {
			logLastError(L"error appending menu to menubar");
			std::abort();
		}
	}

	return menubar;
}

void runMenuEvent(UINT id, uiWindow *w)
{
	uiMenu *m;
	uiMenuItem *item;
	size_t i, j;

	// this isn't optimal, but it works, and it should be just fine for most cases
	for (i = 0; i < len; i++) {
		m = menus[i];
		for (j = 0; j < m->len; j++) {
			item = m->items[j];
			if (item->id == id)
				goto found;
		}
	}
	// no match
	uiprivImplBug("unknown menu ID %u in runMenuEvent()", id);
	return;

found:
	if (item->disabled)
		return;
	// first toggle checkboxes, if any
	if (item->type == typeCheckbox)
		uiMenuItemSetChecked(item, !uiMenuItemChecked(item));

	// then run the event
	if (!uiprivUserCallbackEnter(uiControl(w)))
		return;
	(*(item->onClicked))(item, w, item->onClickedData);
	uiprivUserCallbackLeave();
}

static void freeMenu(uiMenu *m, HMENU submenu)
{
	size_t i;
	uiMenuItem *item;
	size_t j;

	for (i = 0; i < m->len; i++) {
		item = m->items[i];
		for (j = 0; j < item->len; j++)
			if (item->hmenus[j] == submenu)
				break;
		if (j >= item->len)
			continue;
		for (; j < item->len - 1; j++)
			item->hmenus[j] = item->hmenus[j + 1];
		item->hmenus[j] = NULL;
		item->len--;
	}
}

static uiMenu *findMenuForSubmenu(HMENU submenu)
{
	uiMenu *m;
	uiMenuItem *item;
	size_t i, j, k;

	for (i = 0; i < len; i++) {
		m = menus[i];
		for (j = 0; j < m->len; j++) {
			item = m->items[j];
			for (k = 0; k < item->len; k++)
				if (item->hmenus[k] == submenu)
					return m;
		}
	}

	return NULL;
}

void freeMenubar(HMENU menubar)
{
	int count;
	int i;
	MENUITEMINFOW mi;
	uiMenu *m;

	count = GetMenuItemCount(menubar);
	if (count == -1) {
		logLastError(L"error getting menubar item count");
		std::abort();		// menu references cannot be released
	}

	for (i = 0; i < count; i++) {
		ZeroMemory(&mi, sizeof (MENUITEMINFOW));
		mi.cbSize = sizeof (MENUITEMINFOW);
		mi.fMask = MIIM_SUBMENU;
		if (GetMenuItemInfoW(menubar, i, TRUE, &mi) == 0) {
			logLastError(L"error getting menu to delete item references from");
			std::abort();
		}
		m = findMenuForSubmenu(mi.hSubMenu);
		if (m != NULL)
			freeMenu(m, mi.hSubMenu);
	}
	// no need to worry about destroying any menus; destruction of the window they're in will do it for us
}

void uninitMenus(void)
{
	uiMenu *m;
	uiMenuItem *item;
	size_t i, j;
	if (accelerators != NULL) {
		DestroyAcceleratorTable(accelerators);
		accelerators = NULL;
	}

	for (i = 0; i < len; i++) {
		m = menus[i];
		uiprivFree(m->name);
		for (j = 0; j < m->len; j++) {
			item = m->items[j];
			if (item->len != 0)
				// LONGTERM uiprivUserBug()?
				uiprivImplBug("menu item %p (%ws) still has uiWindows attached; did you forget to destroy some windows?", item, item->name);
			if (item->name != NULL)
				uiprivFree(item->name);
			if (item->hmenus != NULL)
				uiprivFree(item->hmenus);
			uiprivFree(item);
		}
		if (m->items != NULL)
			uiprivFree(m->items);
		uiprivFree(m);
	}
	if (menus != NULL)
		uiprivFree(menus);

	/* Reset global state. */
	menus = NULL;
	len = 0;
	cap = 0;
	menusFinalized = FALSE;
	shortcutsFinalized = FALSE;
	curID = firstMenuID;
	hasQuit = FALSE;
	hasPreferences = FALSE;
	hasAbout = FALSE;
}
