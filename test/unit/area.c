#include "area_test.h"

int dragBrokenCalls;

int scrollCalls;
int scrollHandled;
uiAreaScrollEvent lastScroll;

int areaScroll(uiAreaHandler *ah, uiArea *area,
	uiAreaScrollEvent *event)
{
	assert_ptr_equal(ah, &handler);
	assert_ptr_equal(((struct state *) ah->UserData)->c, area);
	scrollCalls++;
	lastScroll = *event;
	return scrollHandled;
}

static void areaDraw(uiAreaHandler *handler, uiArea *area,
	uiAreaDrawParams *params)
{
	assert_non_null(handler->UserData);
}

static void areaMouseEvent(uiAreaHandler *handler, uiArea *area,
	uiAreaMouseEvent *event)
{
}

static void areaMouseCrossed(uiAreaHandler *handler, uiArea *area, int left)
{
}

static void areaDragBroken(uiAreaHandler *handler, uiArea *area)
{
	dragBrokenCalls++;
}

static int areaKeyEvent(uiAreaHandler *handler, uiArea *area,
	uiAreaKeyEvent *event)
{
	return 0;
}

uiAreaHandler handler = {
	.Draw = areaDraw,
	.MouseEvent = areaMouseEvent,
	.MouseCrossed = areaMouseCrossed,
	.DragBroken = areaDragBroken,
	.KeyEvent = areaKeyEvent,
	.ScrollEvent = areaScroll,
};

static int areaSetup(void **state)
{
	struct state *s;

	if (unitTestSetup(state) != 0)
		return -1;
	dragBrokenCalls = 0;
	s = (struct state *) *state;
	handler.UserData = s;
	s->c = uiControl(uiNewArea(&handler));
	return 0;
}

static int scrollingAreaSetup(void **state)
{
	struct state *s;

	if (unitTestSetup(state) != 0)
		return -1;
	s = (struct state *) *state;
	handler.UserData = s;
	s->c = uiControl(uiNewScrollingArea(&handler, 640, 480));
	return 0;
}

static void areaNew(void **state)
{
	assert_non_null(uiAreaFromState(state));
}

static void areaQueueRedraw(void **state)
{
	uiAreaQueueRedrawAll(uiAreaFromState(state));
}

static void scrollingAreaSetSizeAndScroll(void **state)
{
	uiArea *area = uiAreaFromState(state);

	uiAreaSetSize(area, 800, 600);
	uiAreaScrollTo(area, 10, 20, 100, 80);
	uiAreaScrollTo(area, 110, 100, -100, -80);
}

int areaRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		UNIT_TEST_NAMED("areaNew", areaNew,
			areaSetup, unitTestTeardown),
		UNIT_TEST_NAMED("scrollingAreaNew", areaNew,
			scrollingAreaSetup, unitTestTeardown),
		UNIT_TEST_NAMED("areaQueueRedraw", areaQueueRedraw,
			areaSetup, unitTestTeardown),
		UNIT_TEST_NAMED("scrollingAreaQueueRedraw", areaQueueRedraw,
			scrollingAreaSetup, unitTestTeardown),
		UNIT_TEST_NAMED("scrollingAreaSetSizeAndScroll",
			scrollingAreaSetSizeAndScroll,
			scrollingAreaSetup, unitTestTeardown),
#if !defined(_WIN32) && !defined(__APPLE__)
		UNIT_TEST_NAMED("areaScrollEvents", areaScrollEvents,
			areaSetup, unitTestTeardown),
		UNIT_TEST_NAMED("scrollingAreaScrollEvents", scrollingAreaScrollEvents,
			scrollingAreaSetup, unitTestTeardown),
		UNIT_TEST_NAMED("scrollingAreaScrollAfterSetSize",
			scrollingAreaScrollAfterSetSize,
			scrollingAreaSetup, unitTestTeardownHidden),
		UNIT_TEST_NAMED("areaDragBrokenForImplicitPointerGrab",
			areaDragBrokenForImplicitPointerGrab,
			areaSetup, unitTestTeardown),
#endif
	};

	return cmocka_run_group_tests_name("uiArea", tests,
		unitTestsSetup, unitTestsTeardown);
}
