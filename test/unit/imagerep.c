#include <float.h>
#include <limits.h>
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

static void imageTargetPixelSizeRoundsUpAndClamps(void **state)
{
	(void) state;
	assert_int_equal(uiprivImageTargetPixelSize(0), 0);
	assert_int_equal(uiprivImageTargetPixelSize(-1), 0);
	assert_int_equal(uiprivImageTargetPixelSize(0.25), 1);
	assert_int_equal(uiprivImageTargetPixelSize(1), 1);
	assert_int_equal(uiprivImageTargetPixelSize(1.01), 2);
	assert_int_equal(uiprivImageTargetPixelSize(16.5), 17);
	assert_int_equal(uiprivImageTargetPixelSize(INT_MAX), INT_MAX);
	assert_int_equal(uiprivImageTargetPixelSize(DBL_MAX), INT_MAX);
}

static void imagePixelBufferSpanValidatesDimensionsAndOverflow(void **state)
{
	size_t span;

	(void) state;
	assert_true(uiprivImagePixelBufferSpan(2, 3, 12, &span));
	assert_int_equal(span, 36);
	assert_false(uiprivImagePixelBufferSpan(0, 3, 12, &span));
	assert_false(uiprivImagePixelBufferSpan(2, 0, 12, &span));
	assert_false(uiprivImagePixelBufferSpan(2, 3, 7, &span));
#if SIZE_MAX <= UINT32_MAX
	assert_false(uiprivImagePixelBufferSpan(INT_MAX / 4, 3,
		INT_MAX - 3, &span));
#endif
}

static void imageRepPrefersClosestLargeEnoughRepresentation(void **state)
{
	const struct imageSize reps[] = {
		{ 16, 16 }, { 64, 64 }, { 128, 128 },
	};

	(void) state;
	assertImageSize(chooseRep(48, 48, reps, 3), 64, 64);
}

static void imageRepHandlesNoRepresentations(void **state)
{
	(void) state;
	assertImageSize(chooseRep(16, 16, NULL, 0), 0, 0);
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

static void assertFitRect(int imageWidth, int imageHeight,
	int boundsWidth, int boundsHeight,
	int expectedX, int expectedY, int expectedWidth, int expectedHeight)
{
	int x, y, width, height;

	uiprivImageFitRect(imageWidth, imageHeight, boundsWidth, boundsHeight,
		&x, &y, &width, &height);
	assert_int_equal(x, expectedX);
	assert_int_equal(y, expectedY);
	assert_int_equal(width, expectedWidth);
	assert_int_equal(height, expectedHeight);
}

static void imageFitRectPreservesAspectRatioAndCenters(void **state)
{
	(void) state;
	assertFitRect(32, 16, 16, 16, 0, 4, 16, 8);
	assertFitRect(16, 32, 16, 16, 4, 0, 8, 16);
	assertFitRect(16, 16, 16, 16, 0, 0, 16, 16);
	assertFitRect(4, 2, 5, 5, 0, 1, 5, 2);
}

static void imageFitRectHandlesInvalidAndExtremeSizes(void **state)
{
	(void) state;
	assertFitRect(0, 16, 16, 16, 0, 0, 0, 0);
	assertFitRect(INT_MAX, 1, INT_MAX, INT_MAX,
		0, (INT_MAX - 1) / 2, INT_MAX, 1);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(imageTargetPixelSizeRoundsUpAndClamps),
		cmocka_unit_test(imagePixelBufferSpanValidatesDimensionsAndOverflow),
		cmocka_unit_test(imageRepHandlesNoRepresentations),
		cmocka_unit_test(imageRepPrefersClosestLargeEnoughRepresentation),
		cmocka_unit_test(imageRepFallsBackToClosestSmallerRepresentation),
		cmocka_unit_test(imageRepSelectionIsIndependentOfAppendOrder),
		cmocka_unit_test(imageFitRectPreservesAspectRatioAndCenters),
		cmocka_unit_test(imageFitRectHandlesInvalidAndExtremeSizes),
	};

	return cmocka_run_group_tests_name("Image representation", tests,
		NULL, NULL);
}
