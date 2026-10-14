#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "../../common/allocsize.h"

static void allocationHeaderSizeChecksBoundaries(void **state)
{
	const size_t extra = sizeof(size_t) + sizeof(const char **);
	size_t total = 0;

	(void) state;
	assert_true(uiprivAllocSizeWithHeader(0, extra, &total));
	assert_int_equal(total, extra);
	assert_true(uiprivAllocSizeWithHeader(SIZE_MAX - extra - 1, extra, &total));
	assert_int_equal(total, SIZE_MAX - 1);
	assert_true(uiprivAllocSizeWithHeader(SIZE_MAX - extra, extra, &total));
	assert_int_equal(total, SIZE_MAX);
	total = 123;
	assert_false(uiprivAllocSizeWithHeader(SIZE_MAX - extra + 1, extra, &total));
	assert_int_equal(total, 123);
	assert_false(uiprivAllocSizeWithHeader(SIZE_MAX, extra, &total));
	assert_int_equal(total, 123);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(allocationHeaderSizeChecksBoundaries),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
