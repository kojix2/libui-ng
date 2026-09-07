#include "unit.h"

static void controlFontFreedByCommonFunction(void **state)
{
	uiFontDescriptor font;

	uiLoadControlFont(&font);
	assert_non_null(font.Family);
	assert_true(font.Size > 0);
	uiFreeFontDescriptor(&font);
}

#define fontDescriptorUnitTest(f) cmocka_unit_test_setup_teardown((f), \
	unitTestSetup, unitTestTeardown)

int fontDescriptorRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		fontDescriptorUnitTest(controlFontFreedByCommonFunction),
	};

	return cmocka_run_group_tests_name("uiFontDescriptor", tests,
		unitTestsSetup, unitTestsTeardown);
}
