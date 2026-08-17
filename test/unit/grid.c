#include "unit.h"

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

static void gridCoordinatesAndSpans(void **state)
{
	uiGrid *grid = uiGridFromState(state);

	uiGridAppend(grid, uiControl(uiNewButton("negative")),
		-2, -1, 2, 1, 1, uiAlignFill, 0, uiAlignCenter);
	uiGridAppend(grid, uiControl(uiNewButton("span")),
		0, 0, 2, 2, 0, uiAlignEnd, 1, uiAlignFill);
	uiGridAppend(grid, uiControl(uiNewButton("empty range")),
		4, 3, 1, 1, 0, uiAlignStart, 0, uiAlignStart);
}

static void gridInsertAtAllDirections(void **state)
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

static void gridVisibilityAndNesting(void **state)
{
	uiGrid *grid = uiGridFromState(state);
	uiGrid *nested;
	uiControl *child;

	child = uiControl(uiNewButton("toggle"));
	uiGridAppend(grid, child, 0, 0, 2, 1,
		1, uiAlignFill, 1, uiAlignFill);
	uiControlHide(child);
	uiControlShow(child);

	nested = uiNewGrid();
	uiGridAppend(nested, uiControl(uiNewButton("nested")), 0, 0, 1, 1,
		1, uiAlignCenter, 1, uiAlignCenter);
	uiGridAppend(grid, uiControl(nested), 0, 1, 1, 1,
		1, uiAlignFill, 1, uiAlignFill);
}

#define gridUnitTest(f) cmocka_unit_test_setup_teardown((f), \
	gridSetup, unitTestTeardown)

int gridRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		gridUnitTest(gridNew),
		gridUnitTest(gridSetPadded),
		gridUnitTest(gridCoordinatesAndSpans),
		gridUnitTest(gridInsertAtAllDirections),
		gridUnitTest(gridVisibilityAndNesting),
	};

	return cmocka_run_group_tests_name("uiGrid", tests,
		unitTestsSetup, unitTestsTeardown);
}
