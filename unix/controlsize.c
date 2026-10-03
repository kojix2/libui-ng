// 27 september 2026
#include "uipriv_unix.h"

#define uiUnixControlSignature 0x556E6978

// GtkWidget cannot lower an arbitrary child's minimum request, so this bin
// replaces the minimum while preserving the child's natural size.
typedef struct uiprivMinimumSizeWrapper uiprivMinimumSizeWrapper;
typedef struct uiprivMinimumSizeWrapperClass uiprivMinimumSizeWrapperClass;

struct uiprivMinimumSizeWrapper {
	GtkBin parentInstance;
	int width;
	int height;
	gboolean preserveChildMinimum;
};

struct uiprivMinimumSizeWrapperClass {
	GtkBinClass parentClass;
};

#define UIPRIV_TYPE_MINIMUM_SIZE_WRAPPER (uipriv_minimum_size_wrapper_get_type())
#define UIPRIV_MINIMUM_SIZE_WRAPPER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), UIPRIV_TYPE_MINIMUM_SIZE_WRAPPER, uiprivMinimumSizeWrapper))

G_DEFINE_TYPE(uiprivMinimumSizeWrapper, uipriv_minimum_size_wrapper, GTK_TYPE_BIN)

static void applyMinimum(uiprivMinimumSizeWrapper *wrapper, gboolean width,
	gint *minimum, gint *natural)
{
	int requested;

	requested = width ? wrapper->width : wrapper->height;
	if (requested == -1)
		return;
	if (!wrapper->preserveChildMinimum || *minimum < requested)
		*minimum = requested;
	if (*natural < *minimum)
		*natural = *minimum;
}

static void wrapperGetPreferredWidth(GtkWidget *widget, gint *minimum,
	gint *natural)
{
	GTK_WIDGET_CLASS(uipriv_minimum_size_wrapper_parent_class)->get_preferred_width(widget,
		minimum, natural);
	applyMinimum(UIPRIV_MINIMUM_SIZE_WRAPPER(widget), TRUE, minimum, natural);
}

static void wrapperGetPreferredHeight(GtkWidget *widget, gint *minimum,
	gint *natural)
{
	GTK_WIDGET_CLASS(uipriv_minimum_size_wrapper_parent_class)->get_preferred_height(widget,
		minimum, natural);
	applyMinimum(UIPRIV_MINIMUM_SIZE_WRAPPER(widget), FALSE, minimum, natural);
}

static void wrapperGetPreferredHeightForWidth(GtkWidget *widget, gint width,
	gint *minimum, gint *natural)
{
	GTK_WIDGET_CLASS(uipriv_minimum_size_wrapper_parent_class)->get_preferred_height_for_width(widget,
		width, minimum, natural);
	applyMinimum(UIPRIV_MINIMUM_SIZE_WRAPPER(widget), FALSE, minimum, natural);
}

static void wrapperGetPreferredWidthForHeight(GtkWidget *widget, gint height,
	gint *minimum, gint *natural)
{
	GTK_WIDGET_CLASS(uipriv_minimum_size_wrapper_parent_class)->get_preferred_width_for_height(widget,
		height, minimum, natural);
	applyMinimum(UIPRIV_MINIMUM_SIZE_WRAPPER(widget), TRUE, minimum, natural);
}

static void wrapperSizeAllocate(GtkWidget *widget, GtkAllocation *allocation)
{
	GtkWidget *child;
	GtkAllocation childAllocation;

	gtk_widget_set_allocation(widget, allocation);
	child = gtk_bin_get_child(GTK_BIN(widget));
	if (child == NULL || !gtk_widget_get_visible(child))
		return;
	// Keep native allocations positive even when the requested minimum is zero.
	childAllocation = *allocation;
	if (childAllocation.width < 1)
		childAllocation.width = 1;
	if (childAllocation.height < 1)
		childAllocation.height = 1;
	gtk_widget_size_allocate(child, &childAllocation);
}

static void uipriv_minimum_size_wrapper_class_init(uiprivMinimumSizeWrapperClass *klass)
{
	GtkWidgetClass *widgetClass;

	widgetClass = GTK_WIDGET_CLASS(klass);
	widgetClass->get_preferred_width = wrapperGetPreferredWidth;
	widgetClass->get_preferred_height = wrapperGetPreferredHeight;
	widgetClass->get_preferred_height_for_width = wrapperGetPreferredHeightForWidth;
	widgetClass->get_preferred_width_for_height = wrapperGetPreferredWidthForHeight;
	widgetClass->size_allocate = wrapperSizeAllocate;
}

static void uipriv_minimum_size_wrapper_init(uiprivMinimumSizeWrapper *wrapper)
{
	wrapper->width = -1;
	wrapper->height = -1;
	gtk_widget_set_has_window(GTK_WIDGET(wrapper), FALSE);
}

typedef struct minimumSizeState minimumSizeState;
struct minimumSizeState {
	uiControl *c;
	GtkWidget *wrapper;
	minimumSizeState *next;
};

static minimumSizeState *minimumSizeStates;

static minimumSizeState **findMinimumSizeState(uiControl *c)
{
	minimumSizeState **p;

	for (p = &minimumSizeStates; *p != NULL; p = &((*p)->next))
		if ((*p)->c == c)
			return p;
	return NULL;
}

GtkWidget *uiprivUnixControlLayoutWidget(uiControl *c)
{
	minimumSizeState **p;

	p = findMinimumSizeState(c);
	if (p != NULL)
		return (*p)->wrapper;
	return GTK_WIDGET(uiControlHandle(c));
}

static void copyLayoutProperties(GtkWidget *from, GtkWidget *to)
{
	gtk_widget_set_hexpand(to, gtk_widget_get_hexpand(from));
	gtk_widget_set_halign(to, gtk_widget_get_halign(from));
	gtk_widget_set_vexpand(to, gtk_widget_get_vexpand(from));
	gtk_widget_set_valign(to, gtk_widget_get_valign(from));
}

static void makeWrapperChildFill(GtkWidget *widget)
{
	// The wrapper owns alignment relative to the control's parent. The raw
	// widget must fill the allocation that the wrapper gives it rather than
	// applying the same alignment a second time.
	gtk_widget_set_halign(widget, GTK_ALIGN_FILL);
	gtk_widget_set_valign(widget, GTK_ALIGN_FILL);
}

static GtkWidget *refFocusedWidgetWithin(GtkWidget *widget)
{
	GtkWidget *toplevel;
	GtkWidget *focused;

	toplevel = gtk_widget_get_toplevel(widget);
	if (!GTK_IS_WINDOW(toplevel))
		return NULL;
	focused = gtk_window_get_focus(GTK_WINDOW(toplevel));
	if (focused == NULL)
		return NULL;
	if (focused != widget && !gtk_widget_is_ancestor(focused, widget))
		return NULL;
	return g_object_ref(focused);
}

static void restoreFocusedWidget(GtkWidget *focused)
{
	if (focused == NULL)
		return;
	gtk_widget_grab_focus(focused);
	g_object_unref(focused);
}

static gboolean isContainerControl(uiControl *c)
{
	switch (c->TypeSignature) {
	case uiBoxSignature:
	case uiFormSignature:
	case uiGridSignature:
	case uiGroupSignature:
	case uiTabSignature:
		return TRUE;
	}
	return FALSE;
}

static void ownRawWidget(uiUnixControl *c)
{
	if (!c->addedBefore) {
		g_object_ref_sink(GTK_WIDGET(uiControlHandle(uiControl(c))));
		c->addedBefore = TRUE;
	}
}

static void replaceInParent(GtkWidget *from, GtkWidget *to)
{
	GtkWidget *parent;

	parent = gtk_widget_get_parent(from);
	if (parent == NULL)
		return;

	if (GTK_IS_BOX(parent)) {
		gboolean expand, fill;
		guint padding;
		GtkPackType packType;
		gint position;

		gtk_box_query_child_packing(GTK_BOX(parent), from, &expand, &fill,
			&padding, &packType);
		gtk_container_child_get(GTK_CONTAINER(parent), from,
			"position", &position, NULL);
		gtk_container_remove(GTK_CONTAINER(parent), from);
		if (packType == GTK_PACK_END)
			gtk_box_pack_end(GTK_BOX(parent), to, expand, fill, padding);
		else
			gtk_box_pack_start(GTK_BOX(parent), to, expand, fill, padding);
		gtk_box_reorder_child(GTK_BOX(parent), to, position);
	} else if (GTK_IS_GRID(parent)) {
		gint left, top, width, height;

		gtk_container_child_get(GTK_CONTAINER(parent), from,
			"left-attach", &left,
			"top-attach", &top,
			"width", &width,
			"height", &height,
			NULL);
		gtk_container_remove(GTK_CONTAINER(parent), from);
		gtk_grid_attach(GTK_GRID(parent), to, left, top, width, height);
	} else if (GTK_IS_BIN(parent)) {
		gtk_container_remove(GTK_CONTAINER(parent), from);
		gtk_container_add(GTK_CONTAINER(parent), to);
	} else {
		uiprivImplBug("unsupported GTK parent %s for minimum-size wrapper",
			G_OBJECT_TYPE_NAME(parent));
	}
}

static void wrapInCurrentParent(GtkWidget *widget, GtkWidget *wrapper)
{
	gboolean visible;
	GtkWidget *focused;

	visible = gtk_widget_get_visible(widget);
	focused = refFocusedWidgetWithin(widget);
	copyLayoutProperties(widget, wrapper);
	replaceInParent(widget, wrapper);
	makeWrapperChildFill(widget);
	gtk_container_add(GTK_CONTAINER(wrapper), widget);
	if (visible)
		gtk_widget_show(wrapper);
	restoreFocusedWidget(focused);
}

static void unwrapFromCurrentParent(GtkWidget *widget, GtkWidget *wrapper)
{
	gboolean visible;
	GtkWidget *focused;

	visible = gtk_widget_get_visible(wrapper);
	focused = refFocusedWidgetWithin(widget);
	copyLayoutProperties(wrapper, widget);
	gtk_container_remove(GTK_CONTAINER(wrapper), widget);
	replaceInParent(wrapper, widget);
	if (visible)
		gtk_widget_show(widget);
	restoreFocusedWidget(focused);
}

GtkWidget *uiprivUnixControlPrepareWidget(uiUnixControl *c)
{
	uiControl *control;
	GtkWidget *widget;

	control = uiControl(c);
	ownRawWidget(c); // owned by the control's Destroy() path
	widget = GTK_WIDGET(uiControlHandle(control));
	if (!c->explicitlyHidden)
		gtk_widget_show(widget);
	widget = uiprivUnixControlLayoutWidget(control);
	if (!c->explicitlyHidden)
		gtk_widget_show(widget);
	return widget;
}

void uiprivUnixControlShow(uiUnixControl *c)
{
	GtkWidget *raw;

	raw = GTK_WIDGET(uiControlHandle(uiControl(c)));
	gtk_widget_show(raw);
	gtk_widget_show(uiprivUnixControlLayoutWidget(uiControl(c)));
}

void uiprivUnixControlHide(uiUnixControl *c)
{
	GtkWidget *raw;

	// The form label follows the raw widget's visibility.
	raw = GTK_WIDGET(uiControlHandle(uiControl(c)));
	gtk_widget_hide(raw);
	gtk_widget_hide(uiprivUnixControlLayoutWidget(uiControl(c)));
}

void uiprivControlMinimumSizeChanged(uiControl *c)
{
	minimumSizeState **p;
	minimumSizeState *state;
	GtkWidget *raw;
	int width, height;

	if (c->OSSignature != uiUnixControlSignature)
		return;
	uiprivControlMinimumSizeGet(c, &width, &height);
	p = findMinimumSizeState(c);
	if (width == -1 && height == -1) {
		if (p == NULL)
			return;
		state = *p;
		raw = GTK_WIDGET(uiControlHandle(c));
		uiprivUnixBoxChildLayoutWidgetChanged(c, state->wrapper, raw);
		uiprivUnixFormChildLayoutWidgetChanged(c, state->wrapper, raw);
		unwrapFromCurrentParent(raw, state->wrapper);
		*p = state->next;
		g_object_unref(state->wrapper);
		uiprivFree(state);
		gtk_widget_queue_resize(raw);
		return;
	}

	if (p == NULL) {
		state = uiprivNew(minimumSizeState);
		state->c = c;
		state->wrapper = GTK_WIDGET(g_object_new(UIPRIV_TYPE_MINIMUM_SIZE_WRAPPER,
			NULL));
		UIPRIV_MINIMUM_SIZE_WRAPPER(state->wrapper)->preserveChildMinimum =
			isContainerControl(c);
		g_object_ref_sink(state->wrapper);
		state->next = minimumSizeStates;
		minimumSizeStates = state;
		raw = GTK_WIDGET(uiControlHandle(c));
		ownRawWidget(uiUnixControl(c));
		uiprivUnixBoxChildLayoutWidgetChanged(c, raw, state->wrapper);
		uiprivUnixFormChildLayoutWidgetChanged(c, raw, state->wrapper);
		wrapInCurrentParent(raw, state->wrapper);
	} else
		state = *p;

	UIPRIV_MINIMUM_SIZE_WRAPPER(state->wrapper)->width = width;
	UIPRIV_MINIMUM_SIZE_WRAPPER(state->wrapper)->height = height;
	gtk_widget_queue_resize(state->wrapper);
}

void uiprivControlMinimumSizeDestroyed(uiControl *c)
{
	minimumSizeState **p;
	minimumSizeState *state;
	GtkWidget *raw;

	if (c->OSSignature != uiUnixControlSignature)
		return;
	p = findMinimumSizeState(c);
	if (p == NULL)
		return;
	state = *p;
	*p = state->next;
	raw = gtk_bin_get_child(GTK_BIN(state->wrapper));
	if (raw != NULL)
		gtk_container_remove(GTK_CONTAINER(state->wrapper), raw);
	g_object_unref(state->wrapper);
	uiprivFree(state);
}
