#include "unit.h"

struct tableTestState {
	uiTableModelHandler mh;
	uiTableModel *model;
	uiWindow *window;
	uiTable *table;
	uiImage *image;
	const char *rows[2];
	int numRows;
};

static int tableNumColumns(uiTableModelHandler *mh, uiTableModel *m)
{
	return 5;
}

static uiTableValueType tableColumnType(uiTableModelHandler *mh, uiTableModel *m, int column)
{
	if (column == 1)
		return uiTableValueTypeImage;
	if (column == 2 || column == 3)
		return uiTableValueTypeInt;
	if (column == 4)
		return uiTableValueTypeColor;
	return uiTableValueTypeString;
}

static int tableNumRows(uiTableModelHandler *mh, uiTableModel *m)
{
	struct tableTestState *state = (struct tableTestState *) mh;

	return state->numRows;
}

static uiTableValue *tableCellValue(uiTableModelHandler *mh, uiTableModel *m, int row, int column)
{
	struct tableTestState *state = (struct tableTestState *) mh;

	if (column == 1)
		return uiNewTableValueImage(state->image);
	if (column == 2)
		return uiNewTableValueInt(1);
	if (column == 3)
		return uiNewTableValueInt(50);
	if (column == 4)
		return uiNewTableValueColor(0.2, 0.4, 0.6, 0.8);
	return uiNewTableValueString(state->rows[row]);
}

static void tableSetCellValue(uiTableModelHandler *mh, uiTableModel *m, int row, int column, const uiTableValue *value)
{
	// Test columns are read-only.
}

static int tableTestSetup(void **data)
{
	unsigned char pixels[16 * 16 * 4] = {0};
	struct tableTestState *state;
	uiInitOptions options = {0};

	assert_null(uiInit(&options));
	state = calloc(1, sizeof (struct tableTestState));
	assert_non_null(state);
	state->mh.NumColumns = tableNumColumns;
	state->mh.ColumnType = tableColumnType;
	state->mh.NumRows = tableNumRows;
	state->mh.CellValue = tableCellValue;
	state->mh.SetCellValue = tableSetCellValue;
	state->image = uiNewImage(16, 16);
	uiImageAppend(state->image, pixels, 16, 16, 16 * 4);
	state->model = uiNewTableModel(&state->mh);
	state->window = uiNewWindow("Table Width Test", 360, 200, 0);
	uiWindowOnClosing(state->window, unitWindowOnClosingQuit, NULL);
	*data = state;
	return 0;
}

static int tableTestTeardown(void **data)
{
	struct tableTestState *state = *data;

	uiControlDestroy(uiControl(state->window));
	uiFreeTableModel(state->model);
	uiFreeImage(state->image);
	uiUninit();
	free(state);
	return 0;
}

static void settleTableLayout(void)
{
	int i;

	// A single nonblocking main-loop iteration does not guarantee that GTK has
	// completed map and allocation. Keep this bounded so a backend bug cannot
	// hang the unit test.
	for (i = 0; i < 100; i++)
		uiMainStep(0);
}

static void makeTextTable(struct tableTestState *state, const char *header, int editable)
{
	uiTableParams params = {0};

	params.Model = state->model;
	params.RowBackgroundColorModelColumn = -1;
	state->table = uiNewTable(&params);
	uiTableAppendTextColumn(state->table, header, 0,
		editable, NULL);
	// Keep the measured column away from the trailing-column expansion rules.
	uiTableAppendTextColumn(state->table, "Filler", 0,
		uiTableModelColumnNeverEditable, NULL);
	uiWindowSetChild(state->window, uiControl(state->table));
	uiControlShow(uiControl(state->window));
	uiMainSteps();
	settleTableLayout();
}

static void makeTable(struct tableTestState *state, const char *header)
{
	makeTextTable(state, header, uiTableModelColumnNeverEditable);
}

static void attachAndShowTable(struct tableTestState *state)
{
	uiWindowSetChild(state->window, uiControl(state->table));
	uiControlShow(uiControl(state->window));
	uiMainSteps();
	settleTableLayout();
}

static void makeBareTable(struct tableTestState *state)
{
	uiTableParams params = {0};

	params.Model = state->model;
	params.RowBackgroundColorModelColumn = -1;
	state->table = uiNewTable(&params);
}

static void tableValueRoundTrips(void **data)
{
	struct tableTestState *state = *data;
	uiTableValue *value;
	double r, g, b, a;

	value = uiNewTableValueString("value");
	assert_int_equal(uiTableValueGetType(value), uiTableValueTypeString);
	assert_string_equal(uiTableValueString(value), "value");
	uiFreeTableValue(value);

	value = uiNewTableValueImage(state->image);
	assert_int_equal(uiTableValueGetType(value), uiTableValueTypeImage);
	assert_ptr_equal(uiTableValueImage(value), state->image);
	uiFreeTableValue(value);

	value = uiNewTableValueInt(42);
	assert_int_equal(uiTableValueGetType(value), uiTableValueTypeInt);
	assert_int_equal(uiTableValueInt(value), 42);
	uiFreeTableValue(value);

	value = uiNewTableValueColor(0.1, 0.2, 0.3, 0.4);
	assert_int_equal(uiTableValueGetType(value), uiTableValueTypeColor);
	uiTableValueColor(value, &r, &g, &b, &a);
	assert_true(r == 0.1);
	assert_true(g == 0.2);
	assert_true(b == 0.3);
	assert_true(a == 0.4);
	uiFreeTableValue(value);
}

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

static void autoWidthUsesCurrentContentOnce(void **data)
{
	struct tableTestState *state = *data;
	int fixedWidth;
	int longWidth;
	int unchangedWidth;
	int shortWidth;

	state->rows[0] = "x";
	state->rows[1] = "yy";
	state->numRows = 2;
	makeTable(state, "H");

	uiTableColumnSetWidth(state->table, 0, -1);
	settleTableLayout();
	shortWidth = uiTableColumnWidth(state->table, 0);
	assert_true(shortWidth > 0);

	state->rows[1] = "This is a much longer cell value used to test automatic table column sizing";
	uiTableModelRowChanged(state->model, 1);
	settleTableLayout();
	unchangedWidth = uiTableColumnWidth(state->table, 0);
	assert_int_equal(unchangedWidth, shortWidth);

	uiTableColumnSetWidth(state->table, 0, -1);
	settleTableLayout();
	longWidth = uiTableColumnWidth(state->table, 0);
	assert_true(longWidth > shortWidth);

	uiTableColumnSetWidth(state->table, 0, shortWidth);
	settleTableLayout();
	fixedWidth = uiTableColumnWidth(state->table, 0);
	assert_true(fixedWidth < longWidth);
}

static void autoWidthUsesEditableContent(void **data)
{
	struct tableTestState *state = *data;
	int longWidth;
	int shortWidth;

	state->rows[0] = "x";
	state->rows[1] = "yy";
	state->numRows = 2;
	makeTextTable(state, "H", uiTableModelColumnAlwaysEditable);

	uiTableColumnSetWidth(state->table, 0, -1);
	settleTableLayout();
	shortWidth = uiTableColumnWidth(state->table, 0);

	state->rows[1] = "This is a much longer editable cell value used to test automatic table column sizing";
	uiTableModelRowChanged(state->model, 1);
	uiTableColumnSetWidth(state->table, 0, -1);
	settleTableLayout();
	longWidth = uiTableColumnWidth(state->table, 0);

	assert_true(longWidth > shortWidth);
}

static void autoWidthIncludesHeaderForEmptyModel(void **data)
{
	struct tableTestState *state = *data;
	int longHeaderWidth;
	int shortHeaderWidth;

	state->numRows = 0;
	makeTable(state, "A deliberately long table column header");
	uiTableAppendTextColumn(state->table, "X", 0,
		uiTableModelColumnNeverEditable, NULL);
	uiTableAppendTextColumn(state->table, "Trailing filler", 0,
		uiTableModelColumnNeverEditable, NULL);

	uiTableColumnSetWidth(state->table, 0, -1);
	uiTableColumnSetWidth(state->table, 2, -1);
	settleTableLayout();
	longHeaderWidth = uiTableColumnWidth(state->table, 0);
	shortHeaderWidth = uiTableColumnWidth(state->table, 2);
	assert_true(longHeaderWidth > shortHeaderWidth);
}

static void autoWidthIncludesSortIndicator(void **data)
{
	struct tableTestState *state = *data;
	int plainWidth;
	int sortedWidth;
	int targetColumn;

	state->numRows = 0;
	makeTable(state, "H");
	targetColumn = 0;
#ifdef _WIN32
	// Exercise the final-column path too; Windows cannot use
	// LVSCW_AUTOSIZE_USEHEADER here because it would fill the remaining view.
	uiTableAppendTextColumn(state->table, "H", 0,
		uiTableModelColumnNeverEditable, NULL);
	targetColumn = 2;
#endif
	uiTableColumnSetWidth(state->table, targetColumn, -1);
	settleTableLayout();
	plainWidth = uiTableColumnWidth(state->table, targetColumn);

	uiTableHeaderSetSortIndicator(state->table, targetColumn, uiSortIndicatorAscending);
	uiTableColumnSetWidth(state->table, targetColumn, -1);
	settleTableLayout();
	sortedWidth = uiTableColumnWidth(state->table, targetColumn);
	assert_true(sortedWidth > plainWidth);
}

#ifdef __APPLE__
static void autoWidthDoesNotCollapseFixedContentDarwin(void **data)
{
	struct tableTestState *state = *data;
	uiTableParams params = {0};
	int checkboxWidth;
	int imageWidth;
	int progressWidth;

	state->rows[0] = "";
	state->numRows = 1;
	params.Model = state->model;
	params.RowBackgroundColorModelColumn = -1;
	state->table = uiNewTable(&params);
	uiTableAppendImageColumn(state->table, "", 1);
	uiTableAppendCheckboxColumn(state->table, "", 2,
		uiTableModelColumnNeverEditable);
	uiTableAppendProgressBarColumn(state->table, "", 3);
	uiWindowSetChild(state->window, uiControl(state->table));
	uiControlShow(uiControl(state->window));
	uiMainSteps();
	settleTableLayout();

	uiTableColumnSetWidth(state->table, 0, -1);
	uiTableColumnSetWidth(state->table, 1, -1);
	uiTableColumnSetWidth(state->table, 2, -1);
	settleTableLayout();
	imageWidth = uiTableColumnWidth(state->table, 0);
	checkboxWidth = uiTableColumnWidth(state->table, 1);
	progressWidth = uiTableColumnWidth(state->table, 2);
	assert_true(imageWidth >= 16);
	assert_true(checkboxWidth >= 16);
	assert_true(progressWidth >= 80);
}
#endif

static void outOfRangeColumnIsIgnored(void **data)
{
	struct tableTestState *state = *data;

	state->numRows = 0;
	makeTable(state, "H");
	uiTableColumnSetWidth(state->table, -1, -1);
	uiTableColumnSetWidth(state->table, 1000, -1);
	uiTableColumnSetWidth(state->table, -1, 10);
	uiTableColumnSetWidth(state->table, 1000, 10);
}

#ifdef _WIN32
static void autoWidthDoesNotExpandLastWindowsColumn(void **data)
{
	struct tableTestState *state = *data;
	uiTableParams params = {0};
	int autoWidth;
	int initialWidth;

	state->numRows = 0;
	params.Model = state->model;
	params.RowBackgroundColorModelColumn = -1;
	state->table = uiNewTable(&params);
	uiTableAppendTextColumn(state->table, "X", 0,
		uiTableModelColumnNeverEditable, NULL);
	uiWindowSetChild(state->window, uiControl(state->table));
	uiControlShow(uiControl(state->window));
	uiMainSteps();
	settleTableLayout();
	initialWidth = uiTableColumnWidth(state->table, 0);

	uiTableColumnSetWidth(state->table, 0, -1);
	settleTableLayout();
	autoWidth = uiTableColumnWidth(state->table, 0);
	assert_true(autoWidth > 0);
	assert_true(autoWidth < initialWidth);
}
#endif

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
