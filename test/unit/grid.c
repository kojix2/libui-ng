#include "unit.h"

#ifdef _WIN32
#include <windows.h>
#endif

#define uiGridFromState(s) ((uiGrid *) (((struct state *) *(s))->c))

static int gridSetup(void **state)
{
	struct state *s;

	unitTestSetup(state);
	s = (struct state *) *state;
	s->c = uiControl(uiNewGrid());
	return 0;
}

static void gridNew(void **state)
{
	uiGrid *grid = uiGridFromState(state);

	assert_non_null(grid);
	assert_int_equal(uiGridPadded(grid), 0);
}

static void gridSetPadded(void **state)
{
	uiGrid *grid = uiGridFromState(state);

	uiGridSetPadded(grid, 1);
	assert_int_equal(uiGridPadded(grid), 1);
	uiGridSetPadded(grid, 0);
	assert_int_equal(uiGridPadded(grid), 0);
}

static void gridCoordinatesAndSpansDoNotCrash(void **state)
{
	uiGrid *grid = uiGridFromState(state);

	uiGridAppend(grid, uiControl(uiNewButton("negative")),
		-2, -1, 2, 1, 1, uiAlignFill, 0, uiAlignCenter);
	uiGridAppend(grid, uiControl(uiNewButton("span")),
		0, 0, 2, 2, 0, uiAlignEnd, 1, uiAlignFill);
	uiGridAppend(grid, uiControl(uiNewButton("empty range")),
		4, 3, 1, 1, 0, uiAlignStart, 0, uiAlignStart);
}

static void gridInsertAtAllDirectionsDoesNotCrash(void **state)
{
	uiGrid *grid = uiGridFromState(state);
	uiControl *center;

	center = uiControl(uiNewButton("center"));
	uiGridAppend(grid, center, 0, 0, 1, 1,
		0, uiAlignFill, 0, uiAlignFill);
	uiGridInsertAt(grid, uiControl(uiNewButton("leading")), center,
		uiAtLeading, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
	uiGridInsertAt(grid, uiControl(uiNewButton("top")), center,
		uiAtTop, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
	uiGridInsertAt(grid, uiControl(uiNewButton("trailing")), center,
		uiAtTrailing, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
	uiGridInsertAt(grid, uiControl(uiNewButton("bottom")), center,
		uiAtBottom, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
}

static void gridChildVisibilityChangesDoNotCrash(void **state)
{
	uiGrid *grid = uiGridFromState(state);
	uiControl *child;

	child = uiControl(uiNewButton("toggle"));
	uiGridAppend(grid, child, 0, 0, 2, 1,
		1, uiAlignFill, 1, uiAlignFill);
	uiControlHide(child);
	uiControlShow(child);
}

static void gridNestedGridDoesNotCrash(void **state)
{
	uiGrid *grid = uiGridFromState(state);
	uiGrid *nested;

	nested = uiNewGrid();
	uiGridAppend(nested, uiControl(uiNewButton("nested")), 0, 0, 1, 1,
		1, uiAlignCenter, 1, uiAlignCenter);
	uiGridAppend(grid, uiControl(nested), 0, 1, 1, 1,
		1, uiAlignFill, 1, uiAlignFill);
}

static void gridDeleteDetachesAndUpdatesChildren(void **state)
{
	uiGrid *grid = uiGridFromState(state);
	uiControl *first;
	uiControl *second;
	uiControl *replacement;

	first = uiControl(uiNewButton("first"));
	second = uiControl(uiNewButton("second"));
	replacement = uiControl(uiNewButton("replacement"));
	uiGridAppend(grid, first, 0, 0, 1, 1,
		0, uiAlignFill, 0, uiAlignFill);
	uiGridAppend(grid, second, 1, 0, 1, 1,
		0, uiAlignFill, 0, uiAlignFill);
	uiGridDelete(grid, first);
	assert_null(uiControlParent(first));
	uiGridInsertAt(grid, replacement, second, uiAtLeading, 1, 1,
		0, uiAlignFill, 0, uiAlignFill);
	uiGridDelete(grid, second);
	assert_null(uiControlParent(second));
	uiControlDestroy(first);
	uiControlDestroy(second);
}

static void gridChildMinimumSizeCanChangeAfterAppend(void **state)
{
	uiGrid *grid = uiGridFromState(state);
	uiControl *child;

	child = uiControl(uiNewButton("minimum size"));
	uiGridAppend(grid, child, 0, 0, 1, 1,
		0, uiAlignFill, 0, uiAlignFill);
	uiControlSetMinimumSize(child, 40, 0);
	uiControlHide(child);
	uiControlShow(child);
	uiControlSetMinimumSize(child, -1, -1);
	uiGridDelete(grid, child);
	uiControlDestroy(child);
}

#ifdef _WIN32
static int controlWindowWidth(uiControl *c)
{
	RECT r;

	assert_true(GetWindowRect((HWND) uiControlHandle(c), &r));
	return r.right - r.left;
}

static int controlWindowHeight(uiControl *c)
{
	RECT r;

	assert_true(GetWindowRect((HWND) uiControlHandle(c), &r));
	return r.bottom - r.top;
}

static void gridNonExpandingSpanUsesItsMinimumSize(void **state)
{
	struct state *s = *state;
	uiGrid *grid = uiGridFromState(state);
	uiControl *child;

	child = uiControl(uiNewButton("span"));
	uiControlSetMinimumSize(child, 120, 90);
	uiGridAppend(grid, child, 0, 0, 2, 2,
		0, uiAlignFill, 0, uiAlignFill);
	uiWindowSetChild(s->w, uiControl(grid));

	assert_int_equal(controlWindowWidth(child), 120);
	assert_int_equal(controlWindowHeight(child), 90);
}

static void gridExpandingSpanUsesTheAvailableSize(void **state)
{
	struct state *s = *state;
	uiGrid *grid = uiGridFromState(state);
	uiControl *child;

	child = uiControl(uiNewButton("span"));
	uiControlSetMinimumSize(child, 120, 90);
	uiGridAppend(grid, child, 0, 0, 2, 2,
		1, uiAlignFill, 1, uiAlignFill);
	uiWindowSetChild(s->w, uiControl(grid));

	assert_int_equal(controlWindowWidth(child),
		controlWindowWidth(uiControl(grid)));
	assert_int_equal(controlWindowHeight(child),
		controlWindowHeight(uiControl(grid)));
}

static void gridMixedSpanUsesItsMinimumWidth(void **state)
{
	struct state *s = *state;
	uiGrid *grid = uiGridFromState(state);
	uiControl *span;
	uiControl *left;
	uiControl *right;

	span = uiControl(uiNewButton("span"));
	left = uiControl(uiNewButton("left"));
	right = uiControl(uiNewButton("right"));
	uiControlSetMinimumSize(span, 121, 30);
	uiControlSetMinimumSize(left, 20, 20);
	uiControlSetMinimumSize(right, 20, 20);
	uiGridAppend(grid, span, 0, 0, 2, 1,
		0, uiAlignFill, 0, uiAlignFill);
	uiGridAppend(grid, left, 0, 1, 1, 1,
		0, uiAlignFill, 0, uiAlignFill);
	uiGridAppend(grid, right, 1, 1, 1, 1,
		0, uiAlignFill, 0, uiAlignFill);
	uiWindowSetChild(s->w, uiControl(grid));

	assert_int_equal(controlWindowWidth(span), 121);
}

static void gridExpansionPreservesTrackMinimums(void **state)
{
	struct state *s = *state;
	uiGrid *grid = uiGridFromState(state);
	uiControl *topLeft;
	uiControl *topRight;
	uiControl *bottomLeft;
	uiControl *bottomRight;

	topLeft = uiControl(uiNewButton("top left"));
	topRight = uiControl(uiNewButton("top right"));
	bottomLeft = uiControl(uiNewButton("bottom left"));
	bottomRight = uiControl(uiNewButton("bottom right"));
	uiControlSetMinimumSize(topLeft, 240, 140);
	uiControlSetMinimumSize(topRight, 20, 140);
	uiControlSetMinimumSize(bottomLeft, 240, 20);
	uiControlSetMinimumSize(bottomRight, 20, 20);
	uiGridAppend(grid, topLeft, 0, 0, 1, 1,
		1, uiAlignFill, 1, uiAlignFill);
	uiGridAppend(grid, topRight, 1, 0, 1, 1,
		1, uiAlignFill, 1, uiAlignFill);
	uiGridAppend(grid, bottomLeft, 0, 1, 1, 1,
		1, uiAlignFill, 1, uiAlignFill);
	uiGridAppend(grid, bottomRight, 1, 1, 1, 1,
		1, uiAlignFill, 1, uiAlignFill);
	uiWindowSetChild(s->w, uiControl(grid));

	assert_true(controlWindowWidth(topLeft) >= 240);
	assert_true(controlWindowWidth(topRight) >= 20);
	assert_true(controlWindowHeight(topLeft) >= 140);
	assert_true(controlWindowHeight(bottomLeft) >= 20);
	assert_int_equal(controlWindowWidth(topLeft) +
		controlWindowWidth(topRight),
		controlWindowWidth(uiControl(grid)));
	assert_int_equal(controlWindowHeight(topLeft) +
		controlWindowHeight(bottomLeft),
		controlWindowHeight(uiControl(grid)));
}
#endif

#define gridUnitTest(f) cmocka_unit_test_setup_teardown((f), \
	gridSetup, unitTestTeardown)

int gridRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		gridUnitTest(gridNew),
		gridUnitTest(gridSetPadded),
		gridUnitTest(gridCoordinatesAndSpansDoNotCrash),
		gridUnitTest(gridInsertAtAllDirectionsDoesNotCrash),
		gridUnitTest(gridChildVisibilityChangesDoNotCrash),
		gridUnitTest(gridNestedGridDoesNotCrash),
		gridUnitTest(gridDeleteDetachesAndUpdatesChildren),
		gridUnitTest(gridChildMinimumSizeCanChangeAfterAppend),
	#ifdef _WIN32
		gridUnitTest(gridNonExpandingSpanUsesItsMinimumSize),
		gridUnitTest(gridExpandingSpanUsesTheAvailableSize),
		gridUnitTest(gridMixedSpanUsesItsMinimumWidth),
		gridUnitTest(gridExpansionPreservesTrackMinimums),
	#endif
	};

	return cmocka_run_group_tests_name("uiGrid", tests,
		unitTestsSetup, unitTestsTeardown);
}
