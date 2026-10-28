#include "unit.h"
#include <string.h>

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
	NULL,
	NULL,
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

static uiImageBuffer *shared;
static uiArea *areas[2];
static int drawn[2];
static int timedOut;

static void drawBuffer(uiAreaHandler *handler, uiArea *area, uiAreaDrawParams *p)
{
	unsigned char red[] = {255, 0, 0, 255};
	unsigned char blue[] = {0, 0, 255, 255};
	uiImageBuffer *temporary = uiNewImageBuffer(3, 2, uiImageBufferRGBA8Straight);
	uiDrawImageBufferParams crop = {sizeof crop, 2, 1, 1, 1, 0, 20, 16, 16,
		uiImageBufferInterpolationNearest};
	uiDrawMatrix matrix;
	uiDrawPath *clip;
	(void) handler;
	assert_non_null(temporary);
	assert_true(uiImageBufferUpdate(shared, red, 4, 4));
	uiDrawImageBuffer(p->Context, shared, 0, 0, 16, 16, uiImageBufferInterpolationNearest);
	assert_true(uiImageBufferUpdate(shared, blue, 4, 4));
	uiDrawImageBuffer(p->Context, shared, 20, 0, 16, 16, uiImageBufferInterpolationSmooth);
	uiDrawSave(p->Context);
	uiDrawMatrixSetIdentity(&matrix);
	uiDrawMatrixTranslate(&matrix, 3, 7);
	uiDrawTransform(p->Context, &matrix);
	clip = uiDrawNewPath(uiDrawFillModeWinding);
	uiDrawPathAddRectangle(clip, 0, 0, 30, 30);
	uiDrawPathEnd(clip);
	uiDrawClip(p->Context, clip);
	uiDrawFreePath(clip);
	assert_true(uiImageBufferUpdateRect(temporary, 2, 1, 1, 1, red, 4, 4));
	uiDrawImageBufferWithParams(p->Context, temporary, &crop);
	uiFreeImageBuffer(temporary);
	uiDrawRestore(p->Context);
	drawn[area == areas[1]]++;
}
static void mouse(uiAreaHandler *h, uiArea *a, uiAreaMouseEvent *e)
{ (void) h; (void) a; (void) e; }
static void crossed(uiAreaHandler *h, uiArea *a, int left)
{ (void) h; (void) a; (void) left; }
static void broken(uiAreaHandler *h, uiArea *a)
{ (void) h; (void) a; }
static int key(uiAreaHandler *h, uiArea *a, uiAreaKeyEvent *e)
{ (void) h; (void) a; (void) e; return 0; }
static uiAreaHandler handler = {drawBuffer, mouse, crossed, broken, key, NULL, NULL};
static int timeout(void *data)
{ (void) data; timedOut = 1; return 0; }

static void areaDraws(void **state)
{
	uiInitOptions options = {0};
	uiWindow *window;
	uiBox *box;
	int width, height;
	(void) state;
	assert_null(uiInit(&options));
	shared = uiNewImageBuffer(1, 1, uiImageBufferRGBA8Straight);
	assert_non_null(shared);
	uiImageBufferGetSize(shared, &width, &height);
	assert_int_equal(width, 1);
	assert_int_equal(height, 1);
	memset(drawn, 0, sizeof drawn);
	timedOut = 0;
	window = uiNewWindow("ImageBuffer test", 240, 160, 0);
	box = uiNewHorizontalBox();
	areas[0] = uiNewArea(&handler);
	areas[1] = uiNewScrollingArea(&handler, 200, 200);
	uiBoxAppend(box, uiControl(areas[0]), 1);
	uiBoxAppend(box, uiControl(areas[1]), 1);
	uiWindowSetChild(window, uiControl(box));
	uiControlShow(uiControl(window));
	uiMainSteps();
	uiTimer(2000, timeout, NULL);
	while ((!drawn[0] || !drawn[1]) && !timedOut)
		uiMainStep(1);
	uiControlDestroy(uiControl(window));
	uiFreeImageBuffer(shared);
	shared = NULL;
	uiUninit();
	assert_true(drawn[0] > 0);
	assert_true(drawn[1] > 0);
}


int drawImageRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(drawImageFromArea),
		cmocka_unit_test(areaDraws),
	};

	return cmocka_run_group_tests_name("uiDrawImage", tests, NULL, NULL);
}
