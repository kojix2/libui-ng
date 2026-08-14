#include "unit.h"
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

int imageViewRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(imageViewCenterUsesLogicalSize),
		cmocka_unit_test(imageViewCenterAllowsClipping),
		cmocka_unit_test(imageViewFitPreservesAspectRatio),
		cmocka_unit_test(imageViewInvalidSizeProducesEmptyRect),
	};

	return cmocka_run_group_tests_name("uiImageView", tests, NULL, NULL);
}
