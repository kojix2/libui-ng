#include "uipriv_unix.h"
#include "../common/toolbar.h"

static GtkToolbarStyle nativeToolbarStyle(uiToolbarDisplayMode mode)
{
	switch (mode) {
	case uiToolbarDisplayModeIconOnly:
		return GTK_TOOLBAR_ICONS;
	case uiToolbarDisplayModeIconAndTextHorizontal:
		return GTK_TOOLBAR_BOTH_HORIZ;
	case uiToolbarDisplayModeIconAndTextVertical:
		return GTK_TOOLBAR_BOTH;
	case uiToolbarDisplayModeTextOnly:
		return GTK_TOOLBAR_TEXT;
	}
	return GTK_TOOLBAR_BOTH;
}

static void onClicked(GtkToolButton *button, gpointer data)
{
	uiToolbarItem *item = data;
	int checked = 0;

	if (item->type == uiprivToolbarItemToggleButton)
		checked = gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(button));
	uiprivToolbarItemClicked(item, checked);
}

static GtkWidget *newIcon(uiToolbarItem *item)
{
	cairo_surface_t *surface;
	GtkWidget *image;

	if (item->toolbar->displayMode == uiToolbarDisplayModeTextOnly)
		return NULL;
	if (item->icon == NULL) {
		if (item->toolbar->displayMode != uiToolbarDisplayModeIconOnly)
			return NULL;
		image = gtk_label_new(item->text);
		gtk_widget_show(image);
		return image;
	}
	surface = uiprivImageAppropriateSurface(item->icon,
		GTK_WIDGET(item->toolbar->native));
	if (surface == NULL)
		return NULL;
	image = gtk_image_new_from_surface(surface);
	gtk_widget_show(image);
	return image;
}

static void syncItem(uiToolbarItem *item)
{
	GtkToolItem *native = GTK_TOOL_ITEM(item->native);
	GtkWidget *icon;

	if (native == NULL)
		return;
	gtk_widget_set_sensitive(GTK_WIDGET(native), item->enabled != 0);
	gtk_tool_item_set_tooltip_text(native,
		item->tooltip[0] == '\0' ? NULL : item->tooltip);
	if (item->type != uiprivToolbarItemButton &&
		item->type != uiprivToolbarItemToggleButton)
		return;
	gtk_tool_item_set_is_important(native,
		item->toolbar->displayMode == uiToolbarDisplayModeIconAndTextHorizontal);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(native), item->text);
	icon = newIcon(item);
	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(native), icon);
	if (item->type == uiprivToolbarItemToggleButton)
		gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(native),
			item->checked != 0);
}

void uiprivToolbarPlatformNew(uiToolbar *t)
{
	/* Native objects are created on first attachment. */
}

void uiprivToolbarPlatformFree(uiToolbar *t)
{
	if (t->native != NULL)
		g_object_unref(t->native);
}

void uiprivToolbarPlatformAttach(uiToolbar *t, uiWindow *w)
{
	GtkToolbar *toolbar;
	size_t i;

	if (t->native == NULL) {
		toolbar = GTK_TOOLBAR(gtk_toolbar_new());
		gtk_toolbar_set_style(toolbar, nativeToolbarStyle(t->displayMode));
		g_object_ref_sink(toolbar);
		t->native = toolbar;
		for (i = 0; i < t->len; i++) {
			uiToolbarItem *item = t->items[i];
			GtkToolItem *native;

			switch (item->type) {
			case uiprivToolbarItemButton:
				native = gtk_tool_button_new(NULL, item->text);
				break;
			case uiprivToolbarItemToggleButton:
				native = gtk_toggle_tool_button_new();
				break;
			case uiprivToolbarItemSeparator:
				native = gtk_separator_tool_item_new();
				gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(native),
					item->type == uiprivToolbarItemSeparator);
				break;
			}
			item->native = native;
			gtk_toolbar_insert(toolbar, native, -1);
			if (item->type == uiprivToolbarItemButton ||
				item->type == uiprivToolbarItemToggleButton)
				g_signal_connect(native, "clicked", G_CALLBACK(onClicked), item);
			syncItem(item);
			gtk_widget_show(GTK_WIDGET(native));
		}
	}
}

void uiprivToolbarPlatformDetach(uiToolbar *t, uiWindow *w)
{
	GtkWidget *native = GTK_WIDGET(t->native);
	GtkWidget *parent;

	if (native == NULL)
		return;
	parent = gtk_widget_get_parent(native);
	if (parent != NULL)
		gtk_container_remove(GTK_CONTAINER(parent), native);
}

void uiprivToolbarPlatformSyncItem(uiToolbarItem *item)
{
	if (item->toolbar->native != NULL)
		syncItem(item);
}

void uiprivToolbarPlatformSyncItemIcon(uiToolbarItem *item)
{
	uiprivToolbarPlatformSyncItem(item);
}

char *uiprivToolbarPlatformDupText(const char *text)
{
	return g_strdup(text);
}

GtkWidget *uiprivToolbarWidget(uiToolbar *t)
{
	return GTK_WIDGET(t->native);
}
