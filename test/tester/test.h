// 22 april 2015
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include "../../ui.h"

// main.c
extern void die(const char *, ...);
extern uiBox *(*newhbox)(void);
extern uiBox *(*newvbox)(void);

// spaced.c
extern void setSpaced(int);
extern void querySpaced(char[12]);
extern uiWindow *newWindow(const char *title, int width, int height, int hasMenubar);
extern uiBox *newHorizontalBox(void);
extern uiBox *newVerticalBox(void);
extern uiTab *newTab(void);
extern uiGroup *newGroup(const char *);
extern uiForm *newForm(void);
extern uiGrid *newGrid(void);

// menus.c
extern uiMenuItem *shouldQuitItem;
extern void initMenus(void);

// Control scenarios
extern uiBox *makeEntries(void);
extern uiBox *makeMultiline(void);
extern uiBox *makeNumbers(void);
extern uiBox *makeSelection(void);
extern uiBox *makeDateTime(void);
extern uiBox *makeControlState(void);

// Layout scenarios. Properties is the movable first tab of the supplied tab.
extern uiBox *makeProperties(uiWindow *, uiGroup *, uiTab *);
extern void selectParenting(uiButton *, void *);
extern uiBox *makeLayoutParenting(uiBox *, uiTab *, uiBox *, uiGroup *);
extern uiBox *makeLayoutVisibility(void);
extern uiBox *makeLayoutSizes(void);
extern uiBox *makeLayoutForm(void);
extern void freeLayoutForm(void);
extern uiTab *makeLayoutGrid(void);

// Drawing and input scenarios
extern uiBox *makeArea(void);
extern uiGroup *makeAreaArc(void);
extern uiGroup *makeAreaMouse(void);
extern uiGroup *makeAreaScroll(void);
extern uiBox *makeText(void);
extern void runDrawTest(int, uiAreaDrawParams *);
extern void populateComboboxWithTests(uiCombobox *);

// Window scenarios
extern uiBox *makeWindows(uiWindow *);
extern uiBox *makeDialogs(uiWindow *);

// Table resources outlive their controls and are freed after uiMain().
extern uiBox *makeTable(void);
extern void freeTable(void);
extern uiBox *makeTableModel(void);
extern void freeTableModel(void);

// images.c
extern void appendImageNamed(uiImage *img, const char *name);
