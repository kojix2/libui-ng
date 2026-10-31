#include "table_test.h"

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

void autoWidthUsesCurrentContentOnce(void **data)
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

void autoWidthUsesEditableContent(void **data)
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

void autoWidthIncludesHeaderForEmptyModel(void **data)
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

void autoWidthIncludesSortIndicator(void **data)
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
void autoWidthDoesNotCollapseFixedContentDarwin(void **data)
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

void outOfRangeColumnIsIgnored(void **data)
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
void autoWidthDoesNotExpandLastWindowsColumn(void **data)
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
