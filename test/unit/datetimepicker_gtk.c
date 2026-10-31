#include "unit.h"
#include <gtk/gtk.h>
#include <string.h>

static uiWindow *window;

static GtkWidget *findHours(GtkWidget *widget)
{
	GList *children, *p;
	GtkWidget *found = NULL;

	if (GTK_IS_SPIN_BUTTON(widget) &&
		gtk_adjustment_get_upper(gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(widget))) == 11)
		return widget;
	if (!GTK_IS_CONTAINER(widget))
		return NULL;
	children = gtk_container_get_children(GTK_CONTAINER(widget));
	for (p = children; p != NULL && found == NULL; p = p->next)
		found = findHours(GTK_WIDGET(p->data));
	g_list_free(children);
	return found;
}

static int setup(void **state)
{
	uiInitOptions options = {0};
	uiDateTimePicker *picker;
	GtkMenuButton *button;

	assert_null(uiInit(&options));
	window = uiNewWindow("Date/time input tests", 300, 100, 0);
	picker = uiNewTimePicker();
	uiWindowSetChild(window, uiControl(picker));
	button = GTK_MENU_BUTTON((void *) uiControlHandle(uiControl(picker)));
	*state = findHours(GTK_WIDGET(gtk_menu_button_get_popover(button)));
	assert_non_null(*state);
	return 0;
}

static int teardown(void **state)
{
	(void) state;
	uiControlDestroy(uiControl(window));
	uiUninit();
	return 0;
}

static void hoursInputAcceptsValidHours(void **state)
{
	GtkSpinButton *hours = GTK_SPIN_BUTTON(*state);
	const char *inputs[] = {"0", "1", "11", "12"};
	const int expected[] = {0, 1, 11, 0};
	size_t i;
	double value;
	gint result;

	for (i = 0; i < sizeof inputs / sizeof inputs[0]; i++) {
		gtk_entry_set_text(GTK_ENTRY(hours), inputs[i]);
		value = -1;
		g_signal_emit_by_name(hours, "input", &value, &result);
		assert_int_equal(result, TRUE);
		assert_float_equal(value, expected[i], 0.0);
	}
}

static void hoursInputRejectsOutOfRangeNumbers(void **state)
{
	GtkSpinButton *hours = GTK_SPIN_BUTTON(*state);
	char overflow[512];
	const char *inputs[] = {"13", "999999999999999999999999999999999", overflow};
	size_t i;
	double value;
	gint result;

	memset(overflow, '9', sizeof overflow - 1);
	overflow[sizeof overflow - 1] = '\0';
	for (i = 0; i < sizeof inputs / sizeof inputs[0]; i++) {
		gtk_entry_set_text(GTK_ENTRY(hours), inputs[i]);
		assert_string_equal(gtk_entry_get_text(GTK_ENTRY(hours)), inputs[i]);
		value = -1;
		g_signal_emit_by_name(hours, "input", &value, &result);
		assert_int_equal(result, GTK_INPUT_ERROR);
		assert_float_equal(value, -1.0, 0.0);
		gtk_spin_button_update(hours);
	}
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(hoursInputAcceptsValidHours),
		cmocka_unit_test(hoursInputRejectsOutOfRangeNumbers),
	};

	return cmocka_run_group_tests(tests, setup, teardown);
}
