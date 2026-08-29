#include "unit.h"

struct imageTestState {
	uiImage *image;
};

static int imageSetup(void **state)
{
	struct imageTestState *s;
	uiInitOptions options = {0};

	assert_null(uiInit(&options));
	s = calloc(1, sizeof (struct imageTestState));
	assert_non_null(s);
	s->image = uiNewImage(16, 16);
	assert_non_null(s->image);
	*state = s;
	return 0;
}

static int imageTeardown(void **state)
{
	struct imageTestState *s = *state;

	uiFreeImage(s->image);
	free(s);
	uiUninit();
	return 0;
}

static void imageNew(void **state)
{
	struct imageTestState *s = *state;

	assert_non_null(s->image);
}

static void imageAppendRepresentations(void **state)
{
	struct imageTestState *s = *state;
	unsigned char pixels1x[16 * 16 * 4] = {0};
	unsigned char pixels2x[32 * 32 * 4] = {0};

	pixels1x[0] = 0x20;
	pixels1x[1] = 0x40;
	pixels1x[2] = 0x60;
	pixels1x[3] = 0x80;
	uiImageAppend(s->image, pixels1x, 16, 16, 16 * 4);
	uiImageAppend(s->image, pixels2x, 32, 32, 32 * 4);
}

static void imageAppendPaddedStride(void **state)
{
	struct imageTestState *s = *state;
	unsigned char pixels[2 * 12] = {0};

	uiImageAppend(s->image, pixels, 2, 2, 12);
}

int imageRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(imageNew,
			imageSetup, imageTeardown),
		cmocka_unit_test_setup_teardown(imageAppendRepresentations,
			imageSetup, imageTeardown),
		cmocka_unit_test_setup_teardown(imageAppendPaddedStride,
			imageSetup, imageTeardown),
	};

	return cmocka_run_group_tests_name("uiImage", tests, NULL, NULL);
}
