#include "unit.h"

static int horizontalSeparatorSetup(void **state)
{
	struct state *s;

	if (unitTestSetup(state) != 0)
		return -1;
	s = (struct state *) *state;
	s->c = uiControl(uiNewHorizontalSeparator());
	return 0;
}

static int verticalSeparatorSetup(void **state)
{
	struct state *s;

	if (unitTestSetup(state) != 0)
		return -1;
	s = (struct state *) *state;
	s->c = uiControl(uiNewVerticalSeparator());
	return 0;
}

static void separatorNew(void **state)
{
	struct state *s = *state;

	assert_non_null(s->c);
}

int separatorRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		UNIT_TEST_NAMED("horizontal", separatorNew,
			horizontalSeparatorSetup, unitTestTeardown),
		UNIT_TEST_NAMED("vertical", separatorNew,
			verticalSeparatorSetup, unitTestTeardown),
	};

	return cmocka_run_group_tests_name("uiSeparator", tests,
		unitTestsSetup, unitTestsTeardown);
}
