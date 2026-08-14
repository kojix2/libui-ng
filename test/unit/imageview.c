#include "unit.h"
#include "../../common/uipriv.h"

#define EPSILON 0.000001
#define uiImageViewPtrFromState(s) uiControlPtrFromState(uiImageView, s)

static uiImage *newTestImage(void)
{
	uint8_t rep1x[] = {
		0xFF, 0x00, 0x00, 0xFF,
	};
	uint8_t rep2x[] = {
		0x00, 0x80, 0xFF, 0xFF, 0x00, 0x80, 0xFF, 0x80,
		0x00, 0x80, 0xFF, 0x80, 0x00, 0x80, 0xFF, 0x00,
	};
	uiImage *image;

	image = uiNewImage(1, 1);
	uiImageAppend(image, rep1x, 1, 1, 4);
	uiImageAppend(image, rep2x, 2, 2, 8);
	return image;
}

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

static void imageViewNew(void **state)
{
	uiImageView **view = uiImageViewPtrFromState(state);

	*view = uiNewImageView();
}

static void imageViewOwnsSetImage(void **state)
{
	uiImageView **view = uiImageViewPtrFromState(state);
	uiImage *image;

	// unitTestTeardown attaches and paints the view after this returns.
	*view = uiNewImageView();
	image = newTestImage();
	uiImageViewSetImage(*view, image);
	uiFreeImage(image);
}

static void imageViewReplacesOwnedImage(void **state)
{
	uiImageView **view = uiImageViewPtrFromState(state);
	uiImage *first;
	uiImage *second;

	*view = uiNewImageView();
	first = newTestImage();
	uiImageViewSetImage(*view, first);
	uiFreeImage(first);
	second = newTestImage();
	uiImageViewSetImage(*view, second);
	uiFreeImage(second);
}

static void imageViewClearsOwnedImage(void **state)
{
	uiImageView **view = uiImageViewPtrFromState(state);
	uiImage *image;

	*view = uiNewImageView();
	image = newTestImage();
	uiImageViewSetImage(*view, image);
	uiFreeImage(image);
	uiImageViewSetImage(*view, NULL);
}

#define imageViewUnitTest(f) cmocka_unit_test_setup_teardown((f), \
	unitTestSetup, unitTestTeardown)

int imageViewRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(imageViewCenterUsesLogicalSize),
		cmocka_unit_test(imageViewCenterAllowsClipping),
		cmocka_unit_test(imageViewFitPreservesAspectRatio),
		cmocka_unit_test(imageViewInvalidSizeProducesEmptyRect),
		imageViewUnitTest(imageViewNew),
		imageViewUnitTest(imageViewOwnsSetImage),
		imageViewUnitTest(imageViewReplacesOwnedImage),
		imageViewUnitTest(imageViewClearsOwnedImage),
	};

	return cmocka_run_group_tests_name("uiImageView", tests,
		unitTestsSetup, unitTestsTeardown);
}
