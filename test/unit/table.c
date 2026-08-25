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
	return 4;
}

static uiTableValueType tableColumnType(uiTableModelHandler *mh, uiTableModel *m, int column)
{
	if (column == 1)
		return uiTableValueTypeImage;
	if (column == 2 || column == 3)
		return uiTableValueTypeInt;
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

static void makeTable(struct tableTestState *state, const char *header)
{
	uiTableParams params = {0};

	params.Model = state->model;
	params.RowBackgroundColorModelColumn = -1;
	state->table = uiNewTable(&params);
	uiTableAppendTextColumn(state->table, header, 0,
		uiTableModelColumnNeverEditable, NULL);
	// Keep the measured column away from the trailing-column expansion rules.
	uiTableAppendTextColumn(state->table, "Filler", 0,
		uiTableModelColumnNeverEditable, NULL);
	uiWindowSetChild(state->window, uiControl(state->table));
	uiControlShow(uiControl(state->window));
	uiMainSteps();
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

	state->numRows = 0;
	makeTable(state, "H");
	// Exercise the final-column path too; Windows cannot use
	// LVSCW_AUTOSIZE_USEHEADER here because it would fill the remaining view.
	uiTableAppendTextColumn(state->table, "H", 0,
		uiTableModelColumnNeverEditable, NULL);
	uiTableColumnSetWidth(state->table, 2, -1);
	settleTableLayout();
	plainWidth = uiTableColumnWidth(state->table, 2);

	uiTableHeaderSetSortIndicator(state->table, 2, uiSortIndicatorAscending);
	uiTableColumnSetWidth(state->table, 2, -1);
	settleTableLayout();
	sortedWidth = uiTableColumnWidth(state->table, 2);
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
		cmocka_unit_test_setup_teardown(autoWidthUsesCurrentContentOnce,
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

	return cmocka_run_group_tests_name("uiTable column width", tests, NULL, NULL);
}
