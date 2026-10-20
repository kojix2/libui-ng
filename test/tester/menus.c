// 23 april 2015
#include "test.h"

uiMenu *fileMenu;
uiMenuItem *newItem;
uiMenuItem *openItem;
uiMenuItem *shouldQuitItem;
uiMenuItem *quitItem;
uiMenu *editMenu;
uiMenuItem *undoItem;
uiMenuItem *checkItem;
uiMenuItem *accelItem;
uiMenuItem *prefsItem;
uiMenu *testMenu;
uiMenuItem *enabledItem;
uiMenuItem *enableThisItem;
uiMenuItem *forceCheckedItem;
uiMenuItem *forceUncheckedItem;
uiMenuItem *whatWindowItem;
uiMenuItem *resizeableItem;
uiMenu *moreTestsMenu;
uiMenuItem *quitEnabledItem;
uiMenuItem *prefsEnabledItem;
uiMenuItem *aboutEnabledItem;
uiMenuItem *checkEnabledItem;
uiMenu *multiMenu;
uiMenu *helpMenu;
uiMenuItem *helpItem;
uiMenuItem *aboutItem;
static int shortcutActionCount;

static void shortcutActivated(uiMenuItem *item, uiWindow *w, void *data)
{
	printf("shortcut %s action #%d on window %p\n",
		(const char *) data, ++shortcutActionCount, (void *) w);
}

static void enableItemTest(uiMenuItem *item, uiWindow *w, void *data)
{
	if (uiMenuItemChecked(item))
		uiMenuItemEnable(uiMenuItem(data));
	else
		uiMenuItemDisable(uiMenuItem(data));
}

static void forceOn(uiMenuItem *item, uiWindow *w, void *data)
{
	uiMenuItemSetChecked(enabledItem, 1);
}

static void forceOff(uiMenuItem *item, uiWindow *w, void *data)
{
	uiMenuItemSetChecked(enabledItem, 0);
}

static void whatWindow(uiMenuItem *item, uiWindow *w, void *data)
{
	printf("Delete action #%d on window %p\n",
		++shortcutActionCount, (void *) w);
}

static void toggleResize(uiMenuItem *item, uiWindow *w, void *data)
{
	printf("resize action #%d on window %p\n",
		++shortcutActionCount, (void *) w);
	uiWindowSetResizeable(w, uiMenuItemChecked(item));
}

void initMenus(void)
{
	fileMenu = uiNewMenu("File");
	newItem = uiMenuAppendItem(fileMenu, "New");
	uiMenuItemSetShortcut(newItem, 'N',
		uiModifierPrimary);
	uiMenuItemOnClicked(newItem, shortcutActivated, "New");
	openItem = uiMenuAppendItem(fileMenu, "Open");
	uiMenuItemSetShortcut(openItem, 'O',
		uiModifierPrimary);
	uiMenuItemOnClicked(openItem, shortcutActivated, "Open");
	uiMenuAppendSeparator(fileMenu);
	shouldQuitItem = uiMenuAppendCheckItem(fileMenu, "Should Quit");
	quitItem = uiMenuAppendQuitItem(fileMenu);

	editMenu = uiNewMenu("Edit");
	undoItem = uiMenuAppendItem(editMenu, "Undo");
	uiMenuItemSetShortcut(undoItem, 'Z',
		uiModifierPrimary);
	uiMenuItemOnClicked(undoItem, shortcutActivated, "Undo (disabled)");
	uiMenuItemDisable(undoItem);
	uiMenuAppendSeparator(editMenu);
	checkItem = uiMenuAppendCheckItem(editMenu, "Check Me\tTest");
	accelItem = uiMenuAppendItem(editMenu, "A&ccele&&rator T_es__t");
	prefsItem = uiMenuAppendPreferencesItem(editMenu);

	testMenu = uiNewMenu("Test");
	enabledItem = uiMenuAppendCheckItem(testMenu, "Enable Below Item");
	uiMenuItemSetChecked(enabledItem, 1);
	enableThisItem = uiMenuAppendItem(testMenu, "This Will Be Enabled");
	uiMenuItemOnClicked(enabledItem, enableItemTest, enableThisItem);
	forceCheckedItem = uiMenuAppendItem(testMenu, "Force Above Checked");
	uiMenuItemOnClicked(forceCheckedItem, forceOn, NULL);
	forceUncheckedItem = uiMenuAppendItem(testMenu, "Force Above Unchecked");
	uiMenuItemOnClicked(forceUncheckedItem, forceOff, NULL);
	uiMenuAppendSeparator(testMenu);
	whatWindowItem = uiMenuAppendItem(testMenu, "What Window?");
	uiMenuItemSetShortcut(whatWindowItem, uiExtKeyDelete,
		uiModifierPrimary);
	uiMenuItemOnClicked(whatWindowItem, whatWindow, NULL);
	resizeableItem = uiMenuAppendCheckItem(testMenu, "Enable Resize");
	uiMenuItemSetShortcut(resizeableItem, 'R',
		uiModifierPrimary | uiModifierShift);
	uiMenuItemSetChecked(resizeableItem, 1);
	uiMenuItemOnClicked(resizeableItem, toggleResize, NULL);

	moreTestsMenu = uiNewMenu("More Tests");
	quitEnabledItem = uiMenuAppendCheckItem(moreTestsMenu, "Quit Item Enabled");
	uiMenuItemSetChecked(quitEnabledItem, 1);
	prefsEnabledItem = uiMenuAppendCheckItem(moreTestsMenu, "Preferences Item Enabled");
	uiMenuItemSetChecked(prefsEnabledItem, 1);
	aboutEnabledItem = uiMenuAppendCheckItem(moreTestsMenu, "About Item Enabled");
	uiMenuItemSetChecked(aboutEnabledItem, 1);
	uiMenuAppendSeparator(moreTestsMenu);
	checkEnabledItem = uiMenuAppendCheckItem(moreTestsMenu, "Check Me Item Enabled");
	uiMenuItemSetChecked(checkEnabledItem, 1);

	multiMenu = uiNewMenu("Multi");
	uiMenuAppendSeparator(multiMenu);
	uiMenuAppendSeparator(multiMenu);
	uiMenuAppendItem(multiMenu, "Item && Item && Item");
	uiMenuAppendSeparator(multiMenu);
	uiMenuAppendSeparator(multiMenu);
	uiMenuAppendItem(multiMenu, "Item __ Item __ Item");
	uiMenuAppendSeparator(multiMenu);
	uiMenuAppendSeparator(multiMenu);

	helpMenu = uiNewMenu("Help");
	helpItem = uiMenuAppendItem(helpMenu, "Help");
	uiMenuItemSetShortcut(helpItem, uiExtKeyF1, 0);
	uiMenuItemOnClicked(helpItem, shortcutActivated, "Help");
	aboutItem = uiMenuAppendAboutItem(helpMenu);

	uiMenuItemOnClicked(quitEnabledItem, enableItemTest, quitItem);
	uiMenuItemOnClicked(prefsEnabledItem, enableItemTest, prefsItem);
	uiMenuItemOnClicked(aboutEnabledItem, enableItemTest, aboutItem);
	uiMenuItemOnClicked(checkEnabledItem, enableItemTest, checkItem);
}
