#include "unit.h"

#if !defined(_WIN32) && !defined(__APPLE__)
#include <gtk/gtk.h>
#include "../../ui_unix.h"
#endif

#define uiAreaFromState(s) ((uiArea *) (((struct state *) *(s))->c))

static int dragBrokenCalls;

static void areaDraw(uiAreaHandler *handler, uiArea *area,
	uiAreaDrawParams *params)
{
}

static void areaMouseEvent(uiAreaHandler *handler, uiArea *area,
	uiAreaMouseEvent *event)
{
}

static void areaMouseCrossed(uiAreaHandler *handler, uiArea *area, int left)
{
}

static void areaDragBroken(uiAreaHandler *handler, uiArea *area)
{
	dragBrokenCalls++;
}

static int areaKeyEvent(uiAreaHandler *handler, uiArea *area,
	uiAreaKeyEvent *event)
{
	return 0;
}

static uiAreaHandler handler = {
	.Draw = areaDraw,
	.MouseEvent = areaMouseEvent,
	.MouseCrossed = areaMouseCrossed,
	.DragBroken = areaDragBroken,
	.KeyEvent = areaKeyEvent,
};

static int areaSetup(void **state)
{
	struct state *s;

	if (unitTestSetup(state) != 0)
		return -1;
	dragBrokenCalls = 0;
	s = (struct state *) *state;
	s->c = uiControl(uiNewArea(&handler));
	return 0;
}

static int scrollingAreaSetup(void **state)
{
	struct state *s;

	if (unitTestSetup(state) != 0)
		return -1;
	s = (struct state *) *state;
	s->c = uiControl(uiNewScrollingArea(&handler, 640, 480));
	return 0;
}

static void areaNew(void **state)
{
	assert_non_null(uiAreaFromState(state));
}

static void areaQueueRedraw(void **state)
{
	uiAreaQueueRedrawAll(uiAreaFromState(state));
}

static void scrollingAreaSetSizeAndScroll(void **state)
{
	uiArea *area = uiAreaFromState(state);

	uiAreaSetSize(area, 800, 600);
	uiAreaScrollTo(area, 10, 20, 100, 80);
	uiAreaScrollTo(area, 110, 100, -100, -80);
}

#if !defined(_WIN32) && !defined(__APPLE__)
static void scrollingAreaScrollAfterSetSize(void **state)
{
	uiArea *area = uiAreaFromState(state);
	GtkWidget *window;
	GtkWidget *widget;
	GtkScrolledWindow *sw;
	GtkAdjustment *hadj;
	double value, page;
	int i;

	sw = GTK_SCROLLED_WINDOW((void *) uiControlHandle(uiControl(area)));
	widget = uiprivUnixControlPrepareWidget(uiUnixControl(area));
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), 200, 150);
	gtk_container_add(GTK_CONTAINER(window), widget);
	gtk_widget_show_all(window);
	uiMainSteps();
	for (i = 0; i < 30; i++) {
		while (g_main_context_pending(NULL))
			g_main_context_iteration(NULL, FALSE);
		g_usleep(10000);
	}
	hadj = gtk_scrolled_window_get_hadjustment(sw);

	uiAreaSetSize(area, 1000, 480);
	uiAreaScrollTo(area, 800, 0, 20, 20);
	uiAreaScrollTo(area, 900, 0, 20, 20);
	for (i = 0; i < 30; i++) {
		while (g_main_context_pending(NULL))
			g_main_context_iteration(NULL, FALSE);
		g_usleep(10000);
	}

	page = gtk_adjustment_get_page_size(hadj);
	value = gtk_adjustment_get_value(hadj);
	assert_true(gtk_adjustment_get_upper(hadj) >= 1000);
	assert_true(page > 0);
	assert_true(value + page >= 920);
	assert_true(value <= 900);
	gtk_container_remove(GTK_CONTAINER(window), widget);
	gtk_widget_destroy(window);
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
	assert_false(handled);
	gdk_event_free(event);
}

static void areaDragBrokenForImplicitPointerGrab(void **state)
{
	uiArea *area = uiAreaFromState(state);

	emitGrabBroken(area, 1, 1);
	emitGrabBroken(area, 0, 0);
	assert_int_equal(dragBrokenCalls, 0);
	emitGrabBroken(area, 0, 1);
	assert_int_equal(dragBrokenCalls, 1);
}
#endif

int areaRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		UNIT_TEST_NAMED("areaNew", areaNew,
			areaSetup, unitTestTeardown),
		UNIT_TEST_NAMED("scrollingAreaNew", areaNew,
			scrollingAreaSetup, unitTestTeardown),
		UNIT_TEST_NAMED("areaQueueRedraw", areaQueueRedraw,
			areaSetup, unitTestTeardown),
		UNIT_TEST_NAMED("scrollingAreaQueueRedraw", areaQueueRedraw,
			scrollingAreaSetup, unitTestTeardown),
		UNIT_TEST_NAMED("scrollingAreaSetSizeAndScroll",
			scrollingAreaSetSizeAndScroll,
			scrollingAreaSetup, unitTestTeardown),
#if !defined(_WIN32) && !defined(__APPLE__)
		UNIT_TEST_NAMED("scrollingAreaScrollAfterSetSize",
			scrollingAreaScrollAfterSetSize,
			scrollingAreaSetup, unitTestTeardown),
		UNIT_TEST_NAMED("areaDragBrokenForImplicitPointerGrab",
			areaDragBrokenForImplicitPointerGrab,
			areaSetup, unitTestTeardown),
#endif
	};

	return cmocka_run_group_tests_name("uiArea", tests,
		unitTestsSetup, unitTestsTeardown);
}
