#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "../../ui.h"
#include "../../common/uipriv.h"

#define EPSILON 0.000001

static void assertRect(double x, double y, double width, double height,
	double expectedX, double expectedY,
	double expectedWidth, double expectedHeight)
{
	assert_float_equal(x, expectedX, EPSILON);
	assert_float_equal(y, expectedY, EPSILON);
	assert_float_equal(width, expectedWidth, EPSILON);
	assert_float_equal(height, expectedHeight, EPSILON);
}

static void imageViewCenterUsesLogicalSize(void **state)
{
	double x, y, width, height;

	(void) state;
	uiprivImageViewComputeRect(300, 200, 100, 50,
		uiImageViewContentCenter, &x, &y, &width, &height);
	assertRect(x, y, width, height, 100, 75, 100, 50);
}

static void imageViewCenterAllowsClipping(void **state)
{
	double x, y, width, height;

	(void) state;
	uiprivImageViewComputeRect(50, 25, 100, 50,
		uiImageViewContentCenter, &x, &y, &width, &height);
	assertRect(x, y, width, height, -25, -12.5, 100, 50);
}

static void imageViewFitPreservesAspectRatio(void **state)
{
	double x, y, width, height;

	(void) state;
	uiprivImageViewComputeRect(300, 200, 100, 50,
		uiImageViewContentFit, &x, &y, &width, &height);
	assertRect(x, y, width, height, 0, 25, 300, 150);
}

static void imageViewInvalidSizeProducesEmptyRect(void **state)
{
	double x, y, width, height;

	(void) state;
	uiprivImageViewComputeRect(300, 200, 0, 50,
		uiImageViewContentFit, &x, &y, &width, &height);
	assertRect(x, y, width, height, 0, 0, 0, 0);
}

static void imageViewNonFiniteSizeProducesEmptyRect(void **state)
{
	double x, y, width, height;

	(void) state;
	uiprivImageViewComputeRect(300, 200, NAN, 50,
		uiImageViewContentFit, &x, &y, &width, &height);
	assertRect(x, y, width, height, 0, 0, 0, 0);
	uiprivImageViewComputeRect(INFINITY, 200, 100, 50,
		uiImageViewContentFit, &x, &y, &width, &height);
	assertRect(x, y, width, height, 0, 0, 0, 0);
}

static void imageViewFitAvoidsIntermediateOverflow(void **state)
{
	double x, y, width, height;

	(void) state;
	uiprivImageViewComputeRect(100, 100, DBL_MIN, DBL_MIN,
		uiImageViewContentFit, &x, &y, &width, &height);
	assertRect(x, y, width, height, 0, 0, 100, 100);
	uiprivImageViewComputeRect(100, 100, DBL_MAX, DBL_MAX,
		uiImageViewContentFit, &x, &y, &width, &height);
	assertRect(x, y, width, height, 0, 0, 100, 100);
}

static void imageViewRejectsInvalidContentMode(void **state)
{
	double x, y, width, height;

	(void) state;
	assert_true(uiprivImageViewContentModeValid(uiImageViewContentCenter));
	assert_true(uiprivImageViewContentModeValid(uiImageViewContentFit));
	assert_false(uiprivImageViewContentModeValid(
		(uiImageViewContentMode) -1));
	assert_false(uiprivImageViewContentModeValid(
		(uiImageViewContentMode) 2));
	uiprivImageViewComputeRect(100, 100, 50, 50,
		(uiImageViewContentMode) 2, &x, &y, &width, &height);
	assertRect(x, y, width, height, 0, 0, 0, 0);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(imageViewCenterUsesLogicalSize),
		cmocka_unit_test(imageViewCenterAllowsClipping),
		cmocka_unit_test(imageViewFitPreservesAspectRatio),
		cmocka_unit_test(imageViewInvalidSizeProducesEmptyRect),
		cmocka_unit_test(imageViewNonFiniteSizeProducesEmptyRect),
		cmocka_unit_test(imageViewFitAvoidsIntermediateOverflow),
		cmocka_unit_test(imageViewRejectsInvalidContentMode),
	};

	return cmocka_run_group_tests_name("Image view rectangle", tests,
		NULL, NULL);
}
