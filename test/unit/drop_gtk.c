#include "unit.h"
#include <gtk/gtk.h>

// Exercise the real adapter's asynchronous ownership and GLib sources while
// replacing just protocol completion. Pointer routing needs a native drag.
static int finishes;
static gboolean lastAccepted;
static void testFinish(GdkDragContext *context, gboolean success, gboolean delete, guint time)
{
	assert_false(delete);
	finishes++;
	lastAccepted = success;
}
#define gtk_drag_finish testFinish
#include "../../unix/drop.c"
#undef gtk_drag_finish

#define LIBUI_DROP_TEST_NATIVE_HEADERS
#include "drop_test.h"

static int releases, leaves, drops;
static uiDropResult countDrop(const uiDropInfo *i, const uiDropData *p, void *data)
{
	fail_msg("Cancelled transfer reached application Drop");
	return uiDropRejected;
}
static void countRelease(void *data) { releases++; }
static void countLeave(void *data) { leaves++; }
static uiDropResult acceptAndRemove(const uiDropInfo *info, const uiDropData *payload, void *data)
{
	uiprivDropEndpoint *e = data;
	assert_string_equal(payload->Text, "hello");
	drops++;
	assert_int_equal(uiprivDropSetHandler(e, 0, NULL, NULL), uiDropStatusOK);
	assert_int_equal(releases, 0);
	return uiDropAccepted;
}
static uiprivDropEndpoint *newNativeEndpoint(void)
{
	uiprivDropEndpoint *old = newEndpoint();
	GtkWidget *widget = gtk_drawing_area_new();
	uiprivDropEndpoint *e;
	uiDropHandler h = {sizeof h, countDrop, NULL, countLeave, countRelease};
	g_object_ref_sink(widget);
	e = uiprivUnixDropNew(old->owner, widget, widget);
	((testControl *) old->owner)->endpoint = e;
	uiprivDropRelease(old);
	g_object_unref(widget);
	assert_int_equal(uiprivDropSetHandler(e, uiDropFormatText, &h, NULL), uiDropStatusOK);
	finishes = releases = leaves = 0;
	return e;
}

static GdkDragContext *newContext(void)
{
	// These tests only need GObject identity/qdata and references. No native
	// GDK protocol method is invoked on this stand-in context.
	return (GdkDragContext *) g_object_new(G_TYPE_OBJECT, NULL);
}
static void addPending(uiprivDropEndpoint *e, GdkDragContext *context, guint milliseconds)
{
	unixDrop *d = e->native;
	transfer *t = uiprivNew(transfer);
	t->endpoint = e;
	uiprivDropRetain(e);
	t->context = g_object_ref(context);
	t->generation = e->generation;
	t->refs = 2; // pending list and timer
	d->pending = g_list_prepend(d->pending, t);
	t->timer = g_timeout_add_full(G_PRIORITY_DEFAULT, milliseconds, timedOut, t, transferRelease);
}
static hover *addHover(uiprivDropEndpoint *e, GdkDragContext *context)
{
	unixDrop *d = e->native;
	hover *h = uiprivNew(hover);
	h->endpoint = e;
	uiprivDropRetain(e);
	h->context = context;
	h->generation = e->generation;
	h->refs = 1;
	h->showing = 1;
	h->action = uiDropActionCopy;
	d->hovers = g_list_prepend(d->hovers, h);
	g_object_set_qdata_full(G_OBJECT(context), hoverKey(), h, hoverRetire);
	return h;
}

static void removalCancelsOnceAndReleasesData(void **state)
{
	uiprivDropEndpoint *e = newNativeEndpoint();
	unixDrop *d = e->native;
	GdkDragContext *a = newContext(), *b = newContext();
	addPending(e, a, 10000);
	addPending(e, b, 10000);
	assert_int_equal(uiprivDropSetHandler(e, 0, NULL, NULL), uiDropStatusOK);
	assert_int_equal(releases, 1);
	assert_int_equal(finishes, 2);
	assert_false(lastAccepted);
	assert_null(d->pending);
	cancel(e);
	nativeFinish(a, FALSE, 0); // duplicate/late completion is suppressed
	assert_int_equal(finishes, 2);
	g_object_unref(a);
	g_object_unref(b);
	destroyEndpoint(e);
}

static void timeoutUsesMainContextAndFinishesOnce(void **state)
{
	uiprivDropEndpoint *e = newNativeEndpoint();
	unixDrop *d = e->native;
	GdkDragContext *context = newContext();
	addPending(e, context, 1);
	while (d->pending != NULL)
		g_main_context_iteration(NULL, TRUE);
	assert_int_equal(finishes, 1);
	assert_false(lastAccepted);
	assert_int_equal(releases, 0);
	cancel(e);
	assert_int_equal(finishes, 1);
	g_object_unref(context);
	destroyEndpoint(e);
	assert_int_equal(releases, 1);
}

static void leavePreservesNegotiationButClearDoesNotPinIt(void **state)
{
	uiprivDropEndpoint *e = newNativeEndpoint();
	unixDrop *d = e->native;
	GdkDragContext *context = newContext();
	hover *h = addHover(e, context);
	leave(d->widget, context, 0, e);
	assert_int_equal(leaves, 1);
	assert_int_equal(h->action, uiDropActionCopy);
	assert_false(h->showing);
	assert_int_equal(uiprivDropSetHandler(e, 0, NULL, NULL), uiDropStatusOK);
	assert_int_equal(releases, 1);
	assert_null(d->hovers);
	assert_null(g_object_get_qdata(G_OBJECT(context), hoverKey()));
	destroyEndpoint(e); // context is still alive, but endpoint must be freed
	g_object_unref(context);
	assert_int_equal(allocations, 0);
}

static void removalFromLeaveDoesNotFreeStolenHover(void **state)
{
	uiprivDropEndpoint *e = newNativeEndpoint();
	unixDrop *d = e->native;
	GdkDragContext *context = newContext();
	hover *h = addHover(e, context);
	assert_ptr_equal(g_object_steal_qdata(G_OBJECT(context), hoverKey()), h);
	hoverUnlink(h); // Drop takes ownership before invoking Leave.
	assert_int_equal(uiprivDropSetHandler(e, 0, NULL, NULL), uiDropStatusOK);
	assert_null(d->hovers);
	assert_int_equal(h->action, uiDropActionCopy);
	hoverRelease(h);
	g_object_unref(context);
	destroyEndpoint(e);
}

static void deliveredDropCanRemoveItselfAndCompleteOnce(void **state)
{
	uiprivDropEndpoint *e = newNativeEndpoint();
	unixDrop *d = e->native;
	GdkDragContext *context = newContext();
	uiDropHandler h = {sizeof h, acceptAndRemove, NULL, NULL, countRelease};
	uiprivDropData payload = {0};
	transfer *t;
	assert_int_equal(uiprivDropSetHandler(e, uiDropFormatText, &h, e), uiDropStatusOK);
	releases = drops = 0;
	addPending(e, context, 10000);
	t = d->pending->data;
	t->refs++; // Keep the transfer around to simulate a late second delivery.
	t->info = uiprivDropMakeInfo(e, uiDropFormatText, 20, 40);
	assert_true(uiprivDropSetText(&payload, "hello", 5));
	finish(t, &payload.data);
	assert_int_equal(drops, 1);
	assert_int_equal(releases, 1);
	assert_int_equal(finishes, 1);
	assert_true(lastAccepted);
	assert_null(d->pending);
	finish(t, &payload.data);
	assert_int_equal(finishes, 1);
	assert_int_equal(drops, 1);
	transferRelease(t);
	uiprivDropDataClear(&payload);
	g_object_unref(context);
	destroyEndpoint(e);
}

static void cancelledResponseDoesNotReachReplacement(void **state)
{
	uiprivDropEndpoint *e = newNativeEndpoint();
	unixDrop *d = e->native;
	GdkDragContext *context = newContext();
	uiDropHandler h = {sizeof h, countDrop, NULL, NULL, countRelease};
	uiprivDropData payload = {0};
	transfer *t;
	addPending(e, context, 10000);
	t = d->pending->data;
	t->refs++;
	t->info = uiprivDropMakeInfo(e, uiDropFormatText, 0, 0);
	assert_int_equal(uiprivDropSetHandler(e, uiDropFormatText, &h, NULL), uiDropStatusOK);
	assert_int_equal(releases, 1); // The late transfer must not retain userdata.
	assert_true(uiprivDropSetText(&payload, "hello", 5));
	finish(t, &payload.data);
	assert_int_equal(finishes, 1);
	assert_false(lastAccepted);
	transferRelease(t);
	uiprivDropDataClear(&payload);
	g_object_unref(context);
	destroyEndpoint(e);
	assert_int_equal(releases, 2);
}

int main(int argc, char **argv)
{
	if (!gtk_init_check(&argc, &argv)) return 77;
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(removalCancelsOnceAndReleasesData),
		cmocka_unit_test(timeoutUsesMainContextAndFinishesOnce),
		cmocka_unit_test(leavePreservesNegotiationButClearDoesNotPinIt),
		cmocka_unit_test(removalFromLeaveDoesNotFreeStolenHover),
		cmocka_unit_test(deliveredDropCanRemoveItselfAndCompleteOnce),
		cmocka_unit_test(cancelledResponseDoesNotReachReplacement),
	};
	return cmocka_run_group_tests_name("drop GTK transfers", tests, NULL, NULL);
}
