#include "unit.h"

#if !defined(_WIN32) && !defined(__APPLE__)
#include <gtk/gtk.h>
#endif

typedef struct focusState focusState;
struct focusState {
	uiWindow *window;
	uiBox *box;
	uiEntry *entry;
	uiEntry *detached;
	int radioChanges;
};

static int setup(void **state)
{
	uiInitOptions options = {0};
	focusState *s;

	assert_null(uiInit(&options));
	s = calloc(1, sizeof (focusState));
	assert_non_null(s);
	s->window = uiNewWindow("Focus tests", 400, 400, 0);
	s->box = uiNewVerticalBox();
	s->entry = uiNewEntry();
	uiBoxAppend(s->box, uiControl(s->entry), 0);
	uiWindowSetChild(s->window, uiControl(s->box));
	*state = s;
	return 0;
}

static int teardown(void **state)
{
	focusState *s = *state;

	if (s->detached != NULL)
		uiControlDestroy(uiControl(s->detached));
	uiControlDestroy(uiControl(s->window));
	uiUninit();
	free(s);
	return 0;
}

static void show(focusState *s)
{
	int i;

	uiControlShow(uiControl(s->window));
	uiMainSteps();
	for (i = 0; i < 32; i++)
		uiMainStep(0);
	uiControlSetFocus(uiControl(s->entry));
	assert_true(uiControlFocused(uiControl(s->entry)));
}

static void entriesTransferFocus(void **state)
{
	focusState *s = *state;
	uiEntry *second = uiNewEntry();

	uiBoxAppend(s->box, uiControl(second), 0);
	show(s);
	assert_false(uiControlFocused(uiControl(second)));
	uiControlSetFocus(uiControl(second));
	assert_true(uiControlFocused(uiControl(second)));
	assert_false(uiControlFocused(uiControl(s->entry)));
	uiControlSetFocus(uiControl(second));
	assert_true(uiControlFocused(uiControl(second)));
	uiControlSetFocus(uiControl(s->entry));
	assert_false(uiControlFocused(uiControl(second)));
}

static void unsupportedControlsDoNotStealFocus(void **state)
{
	focusState *s = *state;
	uiLabel *label = uiNewLabel("Label");

	uiBoxAppend(s->box, uiControl(label), 0);
	show(s);
	uiControlSetFocus(uiControl(s->box));
	uiControlSetFocus(uiControl(s->window));
	uiControlSetFocus(uiControl(label));
	assert_false(uiControlFocused(uiControl(s->box)));
	assert_false(uiControlFocused(uiControl(s->window)));
	assert_false(uiControlFocused(uiControl(label)));
	assert_true(uiControlFocused(uiControl(s->entry)));
}

static void unavailableControlsDoNotStealFocus(void **state)
{
	focusState *s = *state;
	uiEntry *hidden = uiNewEntry();
	uiEntry *disabled = uiNewEntry();
	uiEntry *detached = s->detached = uiNewEntry();

	uiBoxAppend(s->box, uiControl(hidden), 0);
	uiBoxAppend(s->box, uiControl(disabled), 0);
	uiControlHide(uiControl(hidden));
	uiControlDisable(uiControl(disabled));
	uiControlSetFocus(uiControl(s->entry));
	assert_false(uiControlFocused(uiControl(s->entry)));
	show(s);
	uiControlSetFocus(uiControl(hidden));
	uiControlSetFocus(uiControl(disabled));
	uiControlSetFocus(uiControl(detached));
	assert_false(uiControlFocused(uiControl(hidden)));
	assert_false(uiControlFocused(uiControl(disabled)));
	assert_false(uiControlFocused(uiControl(detached)));
	assert_true(uiControlFocused(uiControl(s->entry)));
	uiBoxAppend(s->box, uiControl(detached), 0);
	s->detached = NULL;
}

static void unavailableAncestorsPreventFocus(void **state)
{
	focusState *s = *state;
	uiBox *box = uiNewVerticalBox();
	uiEntry *entry = uiNewEntry();

	uiBoxAppend(box, uiControl(entry), 0);
	uiBoxAppend(s->box, uiControl(box), 0);
	show(s);
	uiControlDisable(uiControl(box));
	uiControlSetFocus(uiControl(entry));
	assert_false(uiControlFocused(uiControl(entry)));
	assert_true(uiControlFocused(uiControl(s->entry)));
	uiControlEnable(uiControl(box));
	uiControlHide(uiControl(box));
	uiControlSetFocus(uiControl(entry));
	assert_false(uiControlFocused(uiControl(entry)));
	assert_true(uiControlFocused(uiControl(s->entry)));
	uiControlShow(uiControl(box));
	uiControlSetFocus(uiControl(entry));
	assert_true(uiControlFocused(uiControl(entry)));
}

static void hiddenTabDoesNotReceiveFocus(void **state)
{
	focusState *s = *state;
	uiTab *tab = uiNewTab();
	uiEntry *first = uiNewEntry();
	uiEntry *second = uiNewEntry();

	uiTabAppend(tab, "First", uiControl(first));
	uiTabAppend(tab, "Second", uiControl(second));
	uiBoxAppend(s->box, uiControl(tab), 1);
	uiTabSetSelected(tab, 0);
	show(s);
	uiControlSetFocus(uiControl(second));
	assert_false(uiControlFocused(uiControl(second)));
	assert_true(uiControlFocused(uiControl(s->entry)));
	assert_int_equal(uiTabSelected(tab), 0);
	uiControlSetFocus(uiControl(first));
	assert_true(uiControlFocused(uiControl(first)));
	assert_false(uiControlFocused(uiControl(tab)));
}

static void compositeControlsOwnInternalFocus(void **state)
{
	focusState *s = *state;
	uiControl *controls[3];
	size_t i;

	controls[0] = uiControl(uiNewSpinbox(0, 100));
	controls[1] = uiControl(uiNewEditableCombobox());
	controls[2] = uiControl(uiNewMultilineEntry());
	for (i = 0; i < 3; i++)
		uiBoxAppend(s->box, controls[i], i == 2);
	show(s);
	for (i = 0; i < 3; i++) {
		uiControlSetFocus(controls[i]);
		assert_true(uiControlFocused(controls[i]));
		assert_false(uiControlFocused(uiControl(s->entry)));
#if !defined(_WIN32) && !defined(__APPLE__)
		{
			GtkWidget *window = GTK_WIDGET(uiControlHandle(uiControl(s->window)));
			GtkWidget *focused = gtk_window_get_focus(GTK_WINDOW(window));

			if (i == 2)
				assert_true(GTK_IS_TEXT_VIEW(focused));
			else
				assert_true(GTK_IS_ENTRY(focused));
		}
#endif
		uiControlSetFocus(uiControl(s->entry));
		assert_false(uiControlFocused(controls[i]));
	}
}

static void countRadioChanges(uiRadioButtons *radio, void *data)
{
	focusState *s = data;

	s->radioChanges++;
}

static void radioButtonsPreserveSelection(void **state)
{
	focusState *s = *state;
	uiRadioButtons *radio = uiNewRadioButtons();
	int selected[] = {-1, 1, -1};
	size_t i;

	uiRadioButtonsAppend(radio, "First");
	uiRadioButtonsAppend(radio, "Second");
	uiRadioButtonsOnSelected(radio, countRadioChanges, s);
	uiBoxAppend(s->box, uiControl(radio), 0);
	show(s);
	for (i = 0; i < 3; i++) {
		uiRadioButtonsSetSelected(radio, selected[i]);
		uiControlSetFocus(uiControl(radio));
		assert_true(uiControlFocused(uiControl(radio)));
		assert_int_equal(uiRadioButtonsSelected(radio), selected[i]);
		assert_int_equal(s->radioChanges, 0);
		uiControlSetFocus(uiControl(s->entry));
	}
}

static void nativeInputControlsReceiveFocus(void **state)
{
	focusState *s = *state;
	uiCombobox *combobox = uiNewCombobox();
	uiControl *controls[7];
	size_t i;

	uiComboboxAppend(combobox, "Item");
	controls[0] = uiControl(uiNewButton("Button"));
	controls[1] = uiControl(uiNewCheckbox("Checkbox"));
	controls[2] = uiControl(combobox);
	controls[3] = uiControl(uiNewSlider(0, 100));
	controls[4] = uiControl(uiNewDateTimePicker());
	controls[5] = uiControl(uiNewFontButton());
	controls[6] = uiControl(uiNewColorButton());
	for (i = 0; i < 7; i++)
		uiBoxAppend(s->box, controls[i], 0);
	show(s);
	for (i = 0; i < 7; i++) {
		uiControlSetFocus(controls[i]);
		if (!uiControlFocused(controls[i]))
			fail_msg("could not focus input control %u", (unsigned int) i);
		assert_false(uiControlFocused(uiControl(s->entry)));
		uiControlSetFocus(uiControl(s->entry));
	}
}

static void draw(uiAreaHandler *h, uiArea *a, uiAreaDrawParams *p) {}
static void mouse(uiAreaHandler *h, uiArea *a, uiAreaMouseEvent *e) {}
static void crossed(uiAreaHandler *h, uiArea *a, int left) {}
static void broken(uiAreaHandler *h, uiArea *a) {}
static int key(uiAreaHandler *h, uiArea *a, uiAreaKeyEvent *e) { return 0; }

static void areasFocusTheirDrawingSurface(void **state)
{
	focusState *s = *state;
	static uiAreaHandler handler = {draw, mouse, crossed, broken, key, NULL, NULL};
	uiArea *areas[2];
	size_t i;

	areas[0] = uiNewArea(&handler);
	areas[1] = uiNewScrollingArea(&handler, 600, 600);
	for (i = 0; i < 2; i++)
		uiBoxAppend(s->box, uiControl(areas[i]), 1);
	show(s);
	for (i = 0; i < 2; i++) {
		uiControlSetFocus(uiControl(areas[i]));
		assert_true(uiControlFocused(uiControl(areas[i])));
		assert_false(uiControlFocused(uiControl(s->entry)));
#if !defined(_WIN32) && !defined(__APPLE__)
		{
			GtkWidget *window = GTK_WIDGET(uiControlHandle(uiControl(s->window)));
			assert_true(GTK_IS_DRAWING_AREA(gtk_window_get_focus(GTK_WINDOW(window))));
		}
#endif
		uiControlSetFocus(uiControl(s->entry));
	}
}

int focusRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(entriesTransferFocus, setup, teardown),
		cmocka_unit_test_setup_teardown(unsupportedControlsDoNotStealFocus, setup, teardown),
		cmocka_unit_test_setup_teardown(unavailableControlsDoNotStealFocus, setup, teardown),
		cmocka_unit_test_setup_teardown(unavailableAncestorsPreventFocus, setup, teardown),
		cmocka_unit_test_setup_teardown(hiddenTabDoesNotReceiveFocus, setup, teardown),
		cmocka_unit_test_setup_teardown(compositeControlsOwnInternalFocus, setup, teardown),
		cmocka_unit_test_setup_teardown(radioButtonsPreserveSelection, setup, teardown),
		cmocka_unit_test_setup_teardown(nativeInputControlsReceiveFocus, setup, teardown),
		cmocka_unit_test_setup_teardown(areasFocusTheirDrawingSurface, setup, teardown),
	};

	return cmocka_run_group_tests_name("uiControl focus", tests, NULL, NULL);
}
