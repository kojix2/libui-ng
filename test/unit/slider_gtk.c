#include "../support/fixture.h"
#include <gtk/gtk.h>

static void onChanged(uiSlider *slider, void *data)
{
	(void) slider;
	(void) data;
	fail_msg("Programmatic range changes must not invoke OnChanged");
}

static int setup(void **state)
{
	struct state *s = *state;
	uiSlider *slider;
	int result = unitTestSetup(state);

	if (result != 0)
		return result;
	slider = uiNewSlider(0, 100);
	s->c = uiControl(slider);
	uiWindowSetChild(s->w, s->c);
	uiSliderOnChanged(slider, onChanged, NULL);
	return 0;
}

static void assertTooltip(uiSlider *slider, const char *expected)
{
	GtkWidget *widget;
	char *text;

	widget = GTK_WIDGET((void *) uiControlHandle(uiControl(slider)));
	text = gtk_widget_get_tooltip_text(widget);
	assert_non_null(text);
	assert_string_equal(text, expected);
	g_free(text);
}

static void rangeChangesRefreshValueTooltip(void **state)
{
	uiSlider *slider = (uiSlider *) ((struct state *) *state)->c;

	uiSliderSetValue(slider, 90);
	assertTooltip(slider, "90");
	uiSliderSetRange(slider, 0, 10);
	assert_int_equal(uiSliderValue(slider), 10);
	assertTooltip(slider, "10");
	uiSliderSetRange(slider, 20, 30);
	assert_int_equal(uiSliderValue(slider), 20);
	assertTooltip(slider, "20");
	uiSliderSetRange(slider, 5, 1);
	assert_int_equal(uiSliderValue(slider), 5);
	assertTooltip(slider, "5");
	uiSliderSetRange(slider, 2, 2);
	assert_int_equal(uiSliderValue(slider), 2);
	assertTooltip(slider, "2");
}

static void rangeChangesKeepTooltipDisabled(void **state)
{
	uiSlider *slider = (uiSlider *) ((struct state *) *state)->c;

	uiSliderSetHasToolTip(slider, 0);
	uiSliderSetRange(slider, 200, 300);
	assert_false(uiSliderHasToolTip(slider));
	uiSliderSetHasToolTip(slider, 1);
	assertTooltip(slider, "200");
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(rangeChangesRefreshValueTooltip,
			setup, unitTestTeardownHidden),
		cmocka_unit_test_setup_teardown(rangeChangesKeepTooltipDisabled,
			setup, unitTestTeardownHidden),
	};

	return cmocka_run_group_tests(tests, unitTestsSetup, unitTestsTeardown);
}
