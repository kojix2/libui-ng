#include "unit.h"
#include "../../common/uipriv.h"

#include <limits.h>

static void gridDimensionsAcceptBoundedRanges(void **state)
{
	int xcount, ycount, cellCount;

	(void) state;
	assert_true(uiprivGridDimensions(-5000, -5, 5000, 5,
		&xcount, &ycount, &cellCount));
	assert_int_equal(xcount, 10000);
	assert_int_equal(ycount, 10);
	assert_int_equal(cellCount, 100000);
}

static void gridDimensionsRejectExcessiveRanges(void **state)
{
	int xcount, ycount, cellCount;

	(void) state;
	assert_false(uiprivGridDimensions(0, 0, 10001, 1,
		&xcount, &ycount, &cellCount));
	assert_false(uiprivGridDimensions(0, 0, 1000, 101,
		&xcount, &ycount, &cellCount));
	assert_false(uiprivGridDimensions(INT_MIN, 0, 1, 1,
		&xcount, &ycount, &cellCount));
	assert_false(uiprivGridDimensions(INT_MIN, INT_MIN, INT_MAX, INT_MAX,
		&xcount, &ycount, &cellCount));
}

static void gridDimensionsRejectEmptyRanges(void **state)
{
	int xcount, ycount, cellCount;

	(void) state;
	assert_false(uiprivGridDimensions(0, 0, 0, 1,
		&xcount, &ycount, &cellCount));
	assert_false(uiprivGridDimensions(1, 0, 0, 1,
		&xcount, &ycount, &cellCount));
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(gridDimensionsAcceptBoundedRanges),
		cmocka_unit_test(gridDimensionsRejectExcessiveRanges),
		cmocka_unit_test(gridDimensionsRejectEmptyRanges),
	};

	return cmocka_run_group_tests_name("grid dimensions", tests, NULL, NULL);
}
