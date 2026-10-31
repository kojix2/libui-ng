#include "entry_test.h"
#include <gtk/gtk.h>

struct searchChangedState {
	int calls;
	int normalize;
};

static void countSearchChanged(uiEntry *e, void *data)
{
	struct searchChangedState *changed = data;

	changed->calls++;
	if (changed->normalize)
		uiEntrySetText(e, "normalized");
}

static void runSearchEventsFor(gint64 milliseconds)
{
	gint64 deadline;

	deadline = g_get_monotonic_time() + milliseconds * 1000;
	while (g_get_monotonic_time() < deadline) {
		while (g_main_context_iteration(NULL, FALSE))
			;
		g_usleep(1000);
	}
}

void searchEntrySetTextDoesNotNotifyLater(void **state)
{
	uiEntry **e = uiEntryPtrFromState(state);
	struct searchChangedState changed = {0, 1};
	GtkEditable *editable;
	int position;

	uiEntryOnChanged(*e, countSearchChanged, &changed);
	uiEntrySetText(*e, "query");
	runSearchEventsFor(300);
	assert_int_equal(changed.calls, 0);

	editable = GTK_EDITABLE(uiControlHandle(uiControl(*e)));
	position = 0;
	gtk_editable_insert_text(editable, "x", 1, &position);
	runSearchEventsFor(300);
	assert_int_equal(changed.calls, 1);
	runSearchEventsFor(300);
	assert_int_equal(changed.calls, 1);
}

void searchEntrySetterCancelsPendingUserChange(void **state)
{
	uiEntry **e = uiEntryPtrFromState(state);
	struct searchChangedState changed = {0, 0};
	GtkEditable *editable;
	int position;

	uiEntryOnChanged(*e, countSearchChanged, &changed);
	editable = GTK_EDITABLE(uiControlHandle(uiControl(*e)));
	position = 0;
	gtk_editable_insert_text(editable, "x", 1, &position);
	uiEntrySetText(*e, "query");
	runSearchEventsFor(300);
	assert_int_equal(changed.calls, 0);

	position = 0;
	gtk_editable_insert_text(editable, "y", 1, &position);
	runSearchEventsFor(300);
	assert_int_equal(changed.calls, 1);
}
