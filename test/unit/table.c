#include "table_test.h"

static void tableAllColumnTypes(void **data)
{
	struct tableTestState *state = *data;
	uiTableParams params = {0};
	uiTableTextColumnOptionalParams textParams = {0};

	state->rows[0] = "First";
	state->rows[1] = "Second";
	state->numRows = 2;
	params.Model = state->model;
	params.RowBackgroundColorModelColumn = 4;
	state->table = uiNewTable(&params);
	textParams.ColorModelColumn = 4;

	uiTableAppendTextColumn(state->table, "Text", 0, 2, &textParams);
	uiTableAppendImageColumn(state->table, "Image", 1);
	uiTableAppendImageTextColumn(state->table, "Image/Text", 1, 0, 2,
		&textParams);
	uiTableAppendCheckboxColumn(state->table, "Checkbox", 2, 2);
	uiTableAppendCheckboxTextColumn(state->table, "Checkbox/Text",
		2, 2, 0, 2, &textParams);
	uiTableAppendProgressBarColumn(state->table, "Progress", 3);
	uiTableAppendButtonColumn(state->table, "Button", 0, 2);
	attachAndShowTable(state);
}

static void tableHeaderAndSortIndicator(void **data)
{
	struct tableTestState *state = *data;

	state->numRows = 0;
	makeBareTable(state);
	uiTableAppendTextColumn(state->table, "Text", 0,
		uiTableModelColumnNeverEditable, NULL);
	uiWindowSetChild(state->window, uiControl(state->table));
	assert_int_equal(uiTableHeaderVisible(state->table), 1);
	uiTableHeaderSetVisible(state->table, 0);
	assert_int_equal(uiTableHeaderVisible(state->table), 0);
	uiTableHeaderSetVisible(state->table, 1);
	assert_int_equal(uiTableHeaderVisible(state->table), 1);

	assert_int_equal(uiTableHeaderSortIndicator(state->table, 0),
		uiSortIndicatorNone);
	uiTableHeaderSetSortIndicator(state->table, 0,
		uiSortIndicatorAscending);
	assert_int_equal(uiTableHeaderSortIndicator(state->table, 0),
		uiSortIndicatorAscending);
	uiTableHeaderSetSortIndicator(state->table, 0,
		uiSortIndicatorDescending);
	assert_int_equal(uiTableHeaderSortIndicator(state->table, 0),
		uiSortIndicatorDescending);
	uiTableHeaderSetSortIndicator(state->table, 0, uiSortIndicatorNone);
	assert_int_equal(uiTableHeaderSortIndicator(state->table, 0),
		uiSortIndicatorNone);
}

static void tableSelectionModesAndValues(void **data)
{
	struct tableTestState *state = *data;
	uiTableSelection selection;
	uiTableSelection *actual;
	int rows[] = { 0, 1 };

	state->rows[0] = "First";
	state->rows[1] = "Second";
	state->numRows = 2;
	makeBareTable(state);
	uiTableAppendTextColumn(state->table, "Text", 0,
		uiTableModelColumnNeverEditable, NULL);
	attachAndShowTable(state);

	assert_int_equal(uiTableGetSelectionMode(state->table),
		uiTableSelectionModeZeroOrOne);
	uiTableSetSelectionMode(state->table, uiTableSelectionModeNone);
	assert_int_equal(uiTableGetSelectionMode(state->table),
		uiTableSelectionModeNone);
	uiTableSetSelectionMode(state->table, uiTableSelectionModeOne);
	assert_int_equal(uiTableGetSelectionMode(state->table),
		uiTableSelectionModeOne);
	uiTableSetSelectionMode(state->table, uiTableSelectionModeZeroOrMany);
	assert_int_equal(uiTableGetSelectionMode(state->table),
		uiTableSelectionModeZeroOrMany);

	selection.NumRows = 2;
	selection.Rows = rows;
	uiTableSetSelection(state->table, &selection);
	actual = uiTableGetSelection(state->table);
	assert_int_equal(actual->NumRows, 2);
	assert_int_equal(actual->Rows[0], 0);
	assert_int_equal(actual->Rows[1], 1);
	uiFreeTableSelection(actual);

	uiTableSetSelectionMode(state->table, uiTableSelectionModeZeroOrOne);
	actual = uiTableGetSelection(state->table);
	assert_int_equal(actual->NumRows, 0);
	uiFreeTableSelection(actual);

	selection.NumRows = 1;
	selection.Rows = rows + 1;
	uiTableSetSelection(state->table, &selection);
	actual = uiTableGetSelection(state->table);
	assert_int_equal(actual->NumRows, 1);
	assert_int_equal(actual->Rows[0], 1);
	uiFreeTableSelection(actual);

	selection.NumRows = 0;
	selection.Rows = NULL;
	uiTableSetSelection(state->table, &selection);
	actual = uiTableGetSelection(state->table);
	assert_int_equal(actual->NumRows, 0);
	assert_null(actual->Rows);
	uiFreeTableSelection(actual);
}

static void tableCallbackNoCall(uiTable *table, void *data)
{
	function_called();
}

static void tableRowCallbackNoCall(uiTable *table, int row, void *data)
{
	function_called();
}

static void tableHeaderCallbackNoCall(uiTable *table, int column, void *data)
{
	function_called();
}

static void tableProgrammaticChangesDoNotCallback(void **data)
{
	struct tableTestState *state = *data;
	uiTableSelection selection = {0};

	state->rows[0] = "First";
	state->numRows = 1;
	makeBareTable(state);
	uiTableAppendTextColumn(state->table, "Text", 0,
		uiTableModelColumnNeverEditable, NULL);
	uiWindowSetChild(state->window, uiControl(state->table));
	uiTableOnRowClicked(state->table, tableRowCallbackNoCall, NULL);
	uiTableOnRowDoubleClicked(state->table, tableRowCallbackNoCall, NULL);
	uiTableHeaderOnClicked(state->table, tableHeaderCallbackNoCall, NULL);
	uiTableOnSelectionChanged(state->table, tableCallbackNoCall, NULL);
	uiTableSetSelection(state->table, &selection);
	uiTableSetSelectionMode(state->table, uiTableSelectionModeNone);
	uiTableHeaderSetSortIndicator(state->table, 0,
		uiSortIndicatorAscending);
}

static void tableModelNotifications(void **data)
{
	struct tableTestState *state = *data;

	state->rows[0] = "First";
	state->rows[1] = "Second";
	state->numRows = 1;
	makeBareTable(state);
	uiTableAppendTextColumn(state->table, "Text", 0,
		uiTableModelColumnNeverEditable, NULL);
	attachAndShowTable(state);

	state->numRows = 2;
	uiTableModelRowInserted(state->model, 1);
	uiTableModelRowChanged(state->model, 0);
	state->numRows = 1;
	uiTableModelRowDeleted(state->model, 1);
	settleTableLayout();
}

int tableRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(tableValueRoundTrips,
			tableTestSetup, tableTestTeardown),
		cmocka_unit_test_setup_teardown(tableAllColumnTypes,
			tableTestSetup, tableTestTeardown),
		cmocka_unit_test_setup_teardown(tableHeaderAndSortIndicator,
			tableTestSetup, tableTestTeardown),
		cmocka_unit_test_setup_teardown(tableSelectionModesAndValues,
			tableTestSetup, tableTestTeardown),
		cmocka_unit_test_setup_teardown(tableProgrammaticChangesDoNotCallback,
			tableTestSetup, tableTestTeardown),
		cmocka_unit_test_setup_teardown(tableModelNotifications,
			tableTestSetup, tableTestTeardown),
		cmocka_unit_test_setup_teardown(autoWidthUsesCurrentContentOnce,
			tableTestSetup, tableTestTeardown),
		cmocka_unit_test_setup_teardown(autoWidthUsesEditableContent,
			tableTestSetup, tableTestTeardown),
		cmocka_unit_test_setup_teardown(autoWidthIncludesHeaderForEmptyModel,
			tableTestSetup, tableTestTeardown),
		cmocka_unit_test_setup_teardown(autoWidthIncludesSortIndicator,
			tableTestSetup, tableTestTeardown),
		cmocka_unit_test_setup_teardown(outOfRangeColumnIsIgnored,
			tableTestSetup, tableTestTeardown),
#ifdef __APPLE__
		cmocka_unit_test_setup_teardown(autoWidthDoesNotCollapseFixedContentDarwin,
			tableTestSetup, tableTestTeardown),
#endif
#ifdef _WIN32
		cmocka_unit_test_setup_teardown(autoWidthDoesNotExpandLastWindowsColumn,
			tableTestSetup, tableTestTeardown),
#endif
	};

	return cmocka_run_group_tests_name("uiTable", tests, NULL, NULL);
}
