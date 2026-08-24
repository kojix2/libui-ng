#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "../../ui.h"
#include "../../common/uipriv.h"

struct imageSize {
	int width;
	int height;
};

static struct imageSize chooseRep(int targetWidth, int targetHeight,
	const struct imageSize *reps, size_t n)
{
	uiprivImageRepMatcher matcher;
	struct imageSize best = { 0, 0 };
	size_t i;

	uiprivImageRepMatcherInit(&matcher, targetWidth, targetHeight);
	for (i = 0; i < n; i++)
		if (uiprivImageRepMatcherAdd(&matcher,
			reps[i].width, reps[i].height))
			best = reps[i];
	return best;
}

static void assertImageSize(struct imageSize actual, int width, int height)
{
	assert_int_equal(actual.width, width);
	assert_int_equal(actual.height, height);
}

static void imageRepPrefersClosestLargeEnoughRepresentation(void **state)
{
	const struct imageSize reps[] = {
		{ 16, 16 }, { 64, 64 }, { 128, 128 },
	};

	(void) state;
	assertImageSize(chooseRep(48, 48, reps, 3), 64, 64);
}

static void imageRepFallsBackToClosestSmallerRepresentation(void **state)
{
	const struct imageSize reps[] = {
		{ 16, 16 }, { 64, 64 }, { 128, 128 },
	};

	(void) state;
	assertImageSize(chooseRep(256, 256, reps, 3), 128, 128);
}

static void imageRepSelectionIsIndependentOfAppendOrder(void **state)
{
	const struct imageSize firstOrder[] = {
		{ 100, 300 }, { 120, 360 },
	};
	const struct imageSize reverseOrder[] = {
		{ 120, 360 }, { 100, 300 },
	};

	(void) state;
	// The representations have the same aspect ratio, while the target does
	// not. The closest representation must not depend on append order.
	assertImageSize(chooseRep(100, 400, firstOrder, 2), 120, 360);
	assertImageSize(chooseRep(100, 400, reverseOrder, 2), 120, 360);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(imageRepPrefersClosestLargeEnoughRepresentation),
		cmocka_unit_test(imageRepFallsBackToClosestSmallerRepresentation),
		cmocka_unit_test(imageRepSelectionIsIndependentOfAppendOrder),
	};

	return cmocka_run_group_tests_name("Image representation", tests,
		NULL, NULL);
}
