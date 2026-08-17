// 9 june 2016
#include "uipriv_unix.h"

struct gridChild {
	uiControl *c;
	int left;
	int top;
	int xspan;
	int yspan;
};

struct uiGrid {
	uiUnixControl c;
	GtkWidget *widget;
	GtkContainer *container;
	GtkGrid *grid;
	GArray *children;
	int padded;
};

uiUnixControlAllDefaultsExceptDestroy(uiGrid)

#define ctrl(g, i) &g_array_index(g->children, struct gridChild, i)

static void uiGridDestroy(uiControl *c)
{
	uiGrid *g = uiGrid(c);
	struct gridChild *gc;
	guint i;

	// free all controls
	for (i = 0; i < g->children->len; i++) {
		gc = ctrl(g, i);
		uiControlSetParent(gc->c, NULL);
		uiUnixControlSetContainer(uiUnixControl(gc->c), g->container, TRUE);
		uiControlDestroy(gc->c);
	}
	g_array_free(g->children, TRUE);
	// and then ourselves
	g_object_unref(g->widget);
	uiFreeControl(uiControl(g));
}

#define TODO_MASSIVE_HACK(c) \
	if (!uiUnixControl(c)->addedBefore) { \
		g_object_ref_sink(GTK_WIDGET(uiControlHandle(uiControl(c)))); \
		gtk_widget_show(GTK_WIDGET(uiControlHandle(uiControl(c)))); \
		uiUnixControl(c)->addedBefore = TRUE; \
	}

static const GtkAlign gtkAligns[] = {
	[uiAlignFill] = GTK_ALIGN_FILL,
	[uiAlignStart] = GTK_ALIGN_START,
	[uiAlignCenter] = GTK_ALIGN_CENTER,
	[uiAlignEnd] = GTK_ALIGN_END,
};

static GtkWidget *prepare(struct gridChild *gc, uiControl *c, int hexpand, uiAlign halign, int vexpand, uiAlign valign)
{
	GtkWidget *widget;

	if (c == NULL)
		uiprivUserBug("You cannot add NULL to a uiGrid.");
	gc->c = c;
	widget = GTK_WIDGET(uiControlHandle(gc->c));
	gtk_widget_set_hexpand(widget, hexpand != 0);
	gtk_widget_set_halign(widget, gtkAligns[halign]);
	gtk_widget_set_vexpand(widget, vexpand != 0);
	gtk_widget_set_valign(widget, gtkAligns[valign]);
	return widget;
}

static void validateSpans(int xspan, int yspan)
{
	if (xspan < 1)
		uiprivUserBug("uiGrid xspan must be at least 1.");
	if (yspan < 1)
		uiprivUserBug("uiGrid yspan must be at least 1.");
}

static int checkedEnd(int origin, int span, const char *axis)
{
	gint64 end;

	end = ((gint64) origin) + span;
	if (end > G_MAXINT)
		uiprivUserBug("uiGrid %s coordinate and span overflow.", axis);
	return (int) end;
}

static int checkedStart(int origin, int span, const char *axis)
{
	gint64 start;

	start = ((gint64) origin) - span;
	if (start < G_MININT)
		uiprivUserBug("uiGrid %s insertion coordinate overflow.", axis);
	return (int) start;
}

static gboolean rangesOverlap(int astart, int aend, int bstart, int bend)
{
	return astart < bend && bstart < aend;
}

static void validateChild(uiGrid *g, struct gridChild *candidate)
{
	struct gridChild *child;
	int candidateRight, candidateBottom;
	guint i;

	candidateRight = checkedEnd(candidate->left, candidate->xspan, "horizontal");
	candidateBottom = checkedEnd(candidate->top, candidate->yspan, "vertical");
	for (i = 0; i < g->children->len; i++) {
		int right, bottom;

		child = ctrl(g, i);
		right = checkedEnd(child->left, child->xspan, "horizontal");
		bottom = checkedEnd(child->top, child->yspan, "vertical");
		if (rangesOverlap(candidate->left, candidateRight, child->left, right) &&
			rangesOverlap(candidate->top, candidateBottom, child->top, bottom))
			uiprivUserBug("Controls in a uiGrid cannot overlap.");
	}
}

void uiGridAppend(uiGrid *g, uiControl *c, int left, int top, int xspan, int yspan, int hexpand, uiAlign halign, int vexpand, uiAlign valign)
{
	struct gridChild gc;
	GtkWidget *widget;

	validateSpans(xspan, yspan);
	widget = prepare(&gc, c, hexpand, halign, vexpand, valign);
	gc.left = left;
	gc.top = top;
	gc.xspan = xspan;
	gc.yspan = yspan;
	validateChild(g, &gc);
	uiControlSetParent(gc.c, uiControl(g));
	TODO_MASSIVE_HACK(uiUnixControl(gc.c));
	gtk_grid_attach(g->grid, widget,
		left, top,
		xspan, yspan);
	g_array_append_val(g->children, gc);
}

void uiGridInsertAt(uiGrid *g, uiControl *c, uiControl *existing, uiAt at, int xspan, int yspan, int hexpand, uiAlign halign, int vexpand, uiAlign valign)
{
	struct gridChild gc;
	struct gridChild *other;
	GtkWidget *widget;
	guint i;

	validateSpans(xspan, yspan);
	widget = prepare(&gc, c, hexpand, halign, vexpand, valign);
	other = NULL;
	for (i = 0; i < g->children->len; i++) {
		struct gridChild *candidate;

		candidate = ctrl(g, i);
		if (candidate->c == existing) {
			other = candidate;
			break;
		}
	}
	if (other == NULL)
		uiprivUserBug("Existing control %p is not in grid %p.", existing, g);
	gc.left = other->left;
	gc.top = other->top;
	gc.xspan = xspan;
	gc.yspan = yspan;
	switch (at) {
	case uiAtLeading:
		gc.left = checkedStart(other->left, gc.xspan, "horizontal");
		break;
	case uiAtTop:
		gc.top = checkedStart(other->top, gc.yspan, "vertical");
		break;
	case uiAtTrailing:
		gc.left = checkedEnd(other->left, other->xspan, "horizontal");
		break;
	case uiAtBottom:
		gc.top = checkedEnd(other->top, other->yspan, "vertical");
		break;
	default:
		uiprivUserBug("Invalid uiAt value %d.", at);
	}
	validateChild(g, &gc);
	uiControlSetParent(gc.c, uiControl(g));
	TODO_MASSIVE_HACK(uiUnixControl(gc.c));
	gtk_grid_attach(g->grid, widget, gc.left, gc.top, xspan, yspan);
	g_array_append_val(g->children, gc);
}

int uiGridPadded(uiGrid *g)
{
	return g->padded;
}

void uiGridSetPadded(uiGrid *g, int padded)
{
	g->padded = padded;
	if (g->padded) {
		gtk_grid_set_row_spacing(g->grid, uiprivGTKYPadding);
		gtk_grid_set_column_spacing(g->grid, uiprivGTKXPadding);
	} else {
		gtk_grid_set_row_spacing(g->grid, 0);
		gtk_grid_set_column_spacing(g->grid, 0);
	}
}

uiGrid *uiNewGrid(void)
{
	uiGrid *g;

	uiUnixNewControl(uiGrid, g);

	g->widget = gtk_grid_new();
	g->container = GTK_CONTAINER(g->widget);
	g->grid = GTK_GRID(g->widget);

	g->children = g_array_new(FALSE, TRUE, sizeof (struct gridChild));

	return g;
}
