#include "unit.h"

static uiImage *drawImage;
static int didDrawImage;
static int drawImageTimedOut;

static int drawImageTimeout(void *data)
{
	(void) data;
	drawImageTimedOut = 1;
	return 0;
}

static void drawImageDraw(uiAreaHandler *handler, uiArea *area,
	uiAreaDrawParams *params)
{
	(void) handler;
	(void) area;
	uiDrawImage(params->Context, drawImage, 0, 0, 1, 1);
	uiDrawImage(params->Context, drawImage, 8, 0, 4, 4);
	didDrawImage = 1;
}

static void drawImageMouseEvent(uiAreaHandler *handler, uiArea *area,
	uiAreaMouseEvent *event)
{
	(void) handler;
	(void) area;
	(void) event;
}

static void drawImageMouseCrossed(uiAreaHandler *handler, uiArea *area,
	int left)
{
	(void) handler;
	(void) area;
	(void) left;
}

static void drawImageDragBroken(uiAreaHandler *handler, uiArea *area)
{
	(void) handler;
	(void) area;
}

static int drawImageKeyEvent(uiAreaHandler *handler, uiArea *area,
	uiAreaKeyEvent *event)
{
	(void) handler;
	(void) area;
	(void) event;
	return 0;
}

static uiAreaHandler drawImageHandler = {
	drawImageDraw,
	drawImageMouseEvent,
	drawImageMouseCrossed,
	drawImageDragBroken,
	drawImageKeyEvent,
};

static uiImage *newDrawImage(void)
{
	uint8_t rep1x[] = {
		0xFF, 0x00, 0x00, 0xFF,
	};
	uint8_t rep2x[] = {
		0x00, 0x80, 0xFF, 0xFF, 0x00, 0x60, 0xC0, 0xC0,
		0x00, 0x40, 0x80, 0x80, 0x00, 0x20, 0x40, 0x40,
	};
	uiImage *image;

	image = uiNewImage(1, 1);
	uiImageAppend(image, rep1x, 1, 1, 4);
	uiImageAppend(image, rep2x, 2, 2, 8);
	return image;
}

static void drawImageFromArea(void **state)
{
	uiInitOptions options = { 0 };
	uiWindow *window;
	uiArea *area;
	int drawn;

	(void) state;
	assert_null(uiInit(&options));
	drawImage = newDrawImage();
	didDrawImage = 0;
	drawImageTimedOut = 0;
	area = uiNewArea(&drawImageHandler);
	window = uiNewWindow("uiDrawImage Unit Test", 100, 100, 0);
	uiWindowSetChild(window, uiControl(area));
	uiControlShow(uiControl(window));

	uiMainSteps();
	uiTimer(1000, drawImageTimeout, NULL);
	while (!didDrawImage && !drawImageTimedOut)
		uiMainStep(1);
	drawn = didDrawImage;

	uiControlDestroy(uiControl(window));
	uiFreeImage(drawImage);
	drawImage = NULL;
	uiUninit();
	assert_true(drawn);
}

int drawImageRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(drawImageFromArea),
	};

	return cmocka_run_group_tests_name("uiDrawImage", tests, NULL, NULL);
}
