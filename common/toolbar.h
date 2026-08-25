#ifndef __LIBUI_TOOLBAR_H__
#define __LIBUI_TOOLBAR_H__

typedef enum uiprivToolbarItemType {
	uiprivToolbarItemButton,
	uiprivToolbarItemToggleButton,
	uiprivToolbarItemSeparator,
	uiprivToolbarItemSpace,
	uiprivToolbarItemFlexibleSpace,
} uiprivToolbarItemType;

struct uiToolbarItem {
	uiToolbar *toolbar;
	uiprivToolbarItemType type;
	char *text;
	char *tooltip;
	uiImage *icon;
	int enabled;
	int checked;
	void (*onClicked)(uiToolbarItem *, void *);
	void *onClickedData;
	void *native;
	uintptr_t nativeID;
	int nativeImage;
};

struct uiToolbar {
	uiToolbarItem **items;
	size_t len;
	size_t cap;
	uiWindow *window;
	int sealed;
	void *native;
	void *nativeAux;
};

extern int uiprivToolbarCanAttach(uiToolbar *, uiWindow *);
extern void uiprivToolbarAttach(uiToolbar *, uiWindow *);
extern void uiprivToolbarDetach(uiToolbar *, uiWindow *);
extern void uiprivToolbarItemClicked(uiToolbarItem *, int);

/* implemented by each platform */
extern void uiprivToolbarPlatformNew(uiToolbar *);
extern void uiprivToolbarPlatformFree(uiToolbar *);
extern void uiprivToolbarPlatformAttach(uiToolbar *, uiWindow *);
extern void uiprivToolbarPlatformDetach(uiToolbar *, uiWindow *);
extern void uiprivToolbarPlatformSyncItem(uiToolbarItem *);
extern void uiprivToolbarPlatformSyncItemIcon(uiToolbarItem *);
extern char *uiprivToolbarPlatformDupText(const char *);

#endif
