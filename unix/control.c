// 16 august 2015
#include "uipriv_unix.h"

static GtkWidget *focusTarget(GtkWidget *widget);

static void findFocusTarget(GtkWidget *widget, gpointer data)
{
	GtkWidget **target = data;

	if (*target == NULL)
		*target = focusTarget(widget);
}

static GtkWidget *focusTarget(GtkWidget *widget)
{
	GtkWidget *target = NULL;

	if (!gtk_widget_get_mapped(widget) || !gtk_widget_is_sensitive(widget))
		return NULL;
	// A scrolled window can itself take focus. Prefer its content so areas,
	// multiline entries, and tables receive keyboard input.
	if (GTK_IS_SCROLLED_WINDOW(widget)) {
		widget = gtk_bin_get_child(GTK_BIN(widget));
		return widget == NULL ? NULL : focusTarget(widget);
	}
	if (GTK_IS_COMBO_BOX(widget) &&
		gtk_combo_box_get_has_entry(GTK_COMBO_BOX(widget)))
		return focusTarget(gtk_bin_get_child(GTK_BIN(widget)));
	if (gtk_widget_get_can_focus(widget))
		return widget;
	if (!GTK_IS_CONTAINER(widget))
		return NULL;
	// Include internal children: a non-editable GtkComboBox delegates focus
	// to a button that gtk_container_get_children() does not expose.
	gtk_container_forall(GTK_CONTAINER(widget), findFocusTarget, &target);
	return target;
}

void uiprivControlSetFocus(uiControl *c)
{
	GtkWidget *target;

	target = focusTarget(GTK_WIDGET(uiControlHandle(c)));
	if (target != NULL)
		gtk_widget_grab_focus(target);
}

int uiprivControlFocused(uiControl *c)
{
	GtkWidget *widget, *toplevel, *focused;

	widget = GTK_WIDGET(uiControlHandle(c));
	if (!gtk_widget_get_mapped(widget))
		return 0;
	toplevel = gtk_widget_get_toplevel(widget);
	if (!GTK_IS_WINDOW(toplevel))
		return 0;
	focused = gtk_window_get_focus(GTK_WINDOW(toplevel));
	return focused != NULL &&
		(focused == widget || gtk_widget_is_ancestor(focused, widget));
}

void uiUnixControlSetContainer(uiUnixControl *c, GtkContainer *container, gboolean remove)
{
	(*(c->SetContainer))(c, container, remove);
}

#define uiUnixControlSignature 0x556E6978

uiUnixControl *uiUnixAllocControl(size_t n, uint32_t typesig, const char *typenamestr)
{
	return uiUnixControl(uiAllocControl(n, uiUnixControlSignature, typesig, typenamestr));
}
