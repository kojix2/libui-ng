// 23 june 2018
#include "../ui.h"
#include "uipriv.h"
#include "table.h"

int uiprivTableModelNumColumns(uiTableModel *m)
{
	uiTableModelHandler *mh;
	int n;

	mh = uiprivTableModelHandler(m);
	uiprivUserCallbackEnter(NULL);
	n = (*(mh->NumColumns))(mh, m);
	uiprivUserCallbackLeave();
	return n;
}

uiTableValueType uiprivTableModelColumnType(uiTableModel *m, int column)
{
	uiTableModelHandler *mh;
	uiTableValueType type;

	mh = uiprivTableModelHandler(m);
	uiprivUserCallbackEnter(NULL);
	type = (*(mh->ColumnType))(mh, m, column);
	uiprivUserCallbackLeave();
	return type;
}

int uiprivTableModelNumRows(uiTableModel *m)
{
	uiTableModelHandler *mh;
	int n;

	mh = uiprivTableModelHandler(m);
	uiprivUserCallbackEnter(NULL);
	n = (*(mh->NumRows))(mh, m);
	uiprivUserCallbackLeave();
	return n;
}

uiTableValue *uiprivTableModelCellValue(uiTableModel *m, int row, int column)
{
	uiTableModelHandler *mh;
	uiTableValue *value;

	mh = uiprivTableModelHandler(m);
	uiprivUserCallbackEnter(NULL);
	value = (*(mh->CellValue))(mh, m, row, column);
	uiprivUserCallbackLeave();
	return value;
}

void uiprivTableModelSetCellValue(uiTableModel *m, int row, int column, const uiTableValue *value)
{
	uiTableModelHandler *mh;

	mh = uiprivTableModelHandler(m);
	uiprivUserCallbackEnter(NULL);
	(*(mh->SetCellValue))(mh, m, row, column, value);

	uiTableModelRowChanged(m, row);
	uiprivUserCallbackLeave();
}

const uiTableTextColumnOptionalParams uiprivDefaultTextColumnOptionalParams = {
	.ColorModelColumn = -1,
};

int uiprivTableModelCellEditable(uiTableModel *m, int row, int column)
{
	uiTableValue *value;
	int editable;

	switch (column) {
	case uiTableModelColumnNeverEditable:
		return 0;
	case uiTableModelColumnAlwaysEditable:
		return 1;
	}
	value = uiprivTableModelCellValue(m, row, column);
	editable = uiTableValueInt(value);
	uiFreeTableValue(value);
	return editable;
}

int uiprivTableModelColorIfProvided(uiTableModel *m, int row, int column, double *r, double *g, double *b, double *a)
{
	uiTableValue *value;

	if (column == -1)
		return 0;
	value = uiprivTableModelCellValue(m, row, column);
	if (value == NULL)
		return 0;
	uiTableValueColor(value, r, g, b, a);
	uiFreeTableValue(value);
	return 1;
}
