#include "unit.h"
#define uiImageViewPtrFromState(s) uiControlPtrFromState(uiImageView, s)

static uiImage *newTestImage(void)
{
	uint8_t rep1x[] = {
		0xFF, 0x00, 0x00, 0xFF,
	};
	uint8_t rep2x[] = {
		0x00, 0x80, 0xFF, 0xFF, 0x00, 0x40, 0x80, 0x80,
		0x00, 0x40, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00,
	};
	uiImage *image;

	image = uiNewImage(1, 1);
	uiImageAppend(image, rep1x, 1, 1, 4);
	uiImageAppend(image, rep2x, 2, 2, 8);
	return image;
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
		imageViewUnitTest(imageViewNew),
		imageViewUnitTest(imageViewOwnsSetImage),
		imageViewUnitTest(imageViewReplacesOwnedImage),
		imageViewUnitTest(imageViewClearsOwnedImage),
	};

	return cmocka_run_group_tests_name("uiImageView", tests,
		unitTestsSetup, unitTestsTeardown);
}
