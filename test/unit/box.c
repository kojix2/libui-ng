#include "unit.h"

#define uiBoxFromState(s) ((uiBox *) (((struct state *) *(s))->c))

static int horizontalBoxSetup(void **state)
{
	struct state *s;

	if (unitTestSetup(state) != 0)
		return -1;
	s = (struct state *) *state;
	s->c = uiControl(uiNewHorizontalBox());
	return 0;
}

static int verticalBoxSetup(void **state)
{
	struct state *s;

	if (unitTestSetup(state) != 0)
		return -1;
	s = (struct state *) *state;
	s->c = uiControl(uiNewVerticalBox());
	return 0;
}

static void boxNew(void **state)
{
	uiBox *box = uiBoxFromState(state);

	assert_non_null(box);
	assert_int_equal(uiBoxNumChildren(box), 0);
	assert_int_equal(uiBoxPadded(box), 0);
}

static void boxSetPadded(void **state)
{
	uiBox *box = uiBoxFromState(state);

	uiBoxSetPadded(box, 1);
	assert_int_equal(uiBoxPadded(box), 1);
	uiBoxSetPadded(box, 0);
	assert_int_equal(uiBoxPadded(box), 0);
}

static void boxAppendAndDelete(void **state)
{
	uiBox *box = uiBoxFromState(state);
	uiControl *first;
	uiControl *second;

	first = uiControl(uiNewButton("first"));
	second = uiControl(uiNewButton("second"));
	uiBoxAppend(box, first, 0);
	uiBoxAppend(box, second, 1);
	assert_int_equal(uiBoxNumChildren(box), 2);
	assert_ptr_equal(uiControlParent(first), uiControl(box));
	assert_ptr_equal(uiControlParent(second), uiControl(box));

	uiBoxDelete(box, 0);
	assert_int_equal(uiBoxNumChildren(box), 1);
	assert_null(uiControlParent(first));
	uiControlDestroy(first);
}

static void boxChildMinimumSizeCanChangeAfterAppend(void **state)
{
	uiBox *box = uiBoxFromState(state);
	uiControl *child;

	child = uiControl(uiNewButton("minimum size"));
	uiBoxAppend(box, child, 0);
	uiControlSetMinimumSize(child, 40, -1);
	uiControlHide(child);
	uiControlShow(child);
	uiControlSetMinimumSize(child, 0, 0);
	uiControlSetMinimumSize(child, -1, -1);
	uiBoxDelete(box, 0);
	uiControlDestroy(child);
}

#define horizontalBoxUnitTest(f) UNIT_TEST_NAMED("horizontal/" #f, (f), \
		horizontalBoxSetup, unitTestTeardown)
#define verticalBoxUnitTest(f) UNIT_TEST_NAMED("vertical/" #f, (f), \
		verticalBoxSetup, unitTestTeardown)

int boxRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		horizontalBoxUnitTest(boxNew),
		verticalBoxUnitTest(boxNew),
		horizontalBoxUnitTest(boxSetPadded),
		verticalBoxUnitTest(boxSetPadded),
		horizontalBoxUnitTest(boxAppendAndDelete),
		verticalBoxUnitTest(boxAppendAndDelete),
		horizontalBoxUnitTest(boxChildMinimumSizeCanChangeAfterAppend),
		verticalBoxUnitTest(boxChildMinimumSizeCanChangeAfterAppend),
	};

	return cmocka_run_group_tests_name("uiBox", tests,
		unitTestsSetup, unitTestsTeardown);
}
