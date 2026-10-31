#include "table_test.h"

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

int tableTestSetup(void **data)
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

int tableTestTeardown(void **data)
{
	struct tableTestState *state = *data;

	uiControlDestroy(uiControl(state->window));
	uiFreeTableModel(state->model);
	uiFreeImage(state->image);
	uiUninit();
	free(state);
	return 0;
}

void settleTableLayout(void)
{
	int i;

	// A single nonblocking main-loop iteration does not guarantee that GTK has
	// completed map and allocation. Keep this bounded so a backend bug cannot
	// hang the unit test.
	for (i = 0; i < 100; i++)
		uiMainStep(0);
}

void attachAndShowTable(struct tableTestState *state)
{
	uiWindowSetChild(state->window, uiControl(state->table));
	uiControlShow(uiControl(state->window));
	uiMainSteps();
	settleTableLayout();
}

void makeBareTable(struct tableTestState *state)
{
	uiTableParams params = {0};

	params.Model = state->model;
	params.RowBackgroundColorModelColumn = -1;
	state->table = uiNewTable(&params);
}
