#include "area_test.h"
#include <gtk/gtk.h>

static void waitForScroll(GtkAdjustment *adj, double upper,
	double start, double end)
{
	gint64 deadline = g_get_monotonic_time() + G_TIME_SPAN_SECOND;
	double value, page;

	/* Layout updates the range first; the deferred ScrollTo idle then moves
	 * the page. An empty event queue alone does not imply either is done. */
	do {
		if (!g_main_context_iteration(NULL, FALSE))
			g_usleep(1000);
		page = gtk_adjustment_get_page_size(adj);
		value = gtk_adjustment_get_value(adj);
		if (gtk_adjustment_get_upper(adj) >= upper && page > 0 &&
			page < upper && value <= start && value + page >= end)
			return;
	} while (g_get_monotonic_time() < deadline);
	fail_msg("scroll timed out: upper=%g (expected >=%g), page=%g, "
		"value=%g (expected <=%g and value+page >=%g)",
		gtk_adjustment_get_upper(adj), upper, page, value, start, end);
}

void scrollingAreaScrollAfterSetSize(void **state)
{
	uiArea *area = uiAreaFromState(state);
	struct state *s = *state;
	GtkScrolledWindow *sw;
	GtkAdjustment *hadj;
	double value, page;

	sw = GTK_SCROLLED_WINDOW((void *) uiControlHandle(uiControl(area)));
	/* Transfer ownership before assertions so teardown also handles failure. */
	uiWindowSetChild(s->w, s->c);
	uiWindowSetContentSize(s->w, 200, 150);
	uiControlShow(uiControl(s->w));
	uiMainSteps();
	hadj = gtk_scrolled_window_get_hadjustment(sw);
	waitForScroll(hadj, 640, 0, 0);

	uiAreaSetSize(area, 1000, 480);
	uiAreaScrollTo(area, 800, 0, 20, 20);
	uiAreaScrollTo(area, 900, 0, 20, 20);
	waitForScroll(hadj, 1000, 900, 920);

	page = gtk_adjustment_get_page_size(hadj);
	value = gtk_adjustment_get_value(hadj);
	assert_true(gtk_adjustment_get_upper(hadj) >= 1000);
	assert_true(page > 0);
	assert_true(value + page >= 920);
	assert_true(value <= 900);
}

static void emitGrabBroken(uiArea *area, int keyboard, int implicit)
{
	GdkEvent *event;
	gboolean handled;
	GtkWidget *widget;

	event = gdk_event_new(GDK_GRAB_BROKEN);
	event->grab_broken.keyboard = keyboard;
	event->grab_broken.implicit = implicit;
	handled = FALSE;
	widget = GTK_WIDGET((void *) uiControlHandle(uiControl(area)));
	g_signal_emit_by_name(widget, "grab-broken-event", event, &handled);
	gdk_event_free(event);
	assert_false(handled);
}

void areaDragBrokenForImplicitPointerGrab(void **state)
{
	uiArea *area = uiAreaFromState(state);

	emitGrabBroken(area, 1, 1);
	emitGrabBroken(area, 0, 0);
	assert_int_equal(dragBrokenCalls, 0);
	emitGrabBroken(area, 0, 1);
	assert_int_equal(dragBrokenCalls, 1);
}
