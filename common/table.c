#include "../ui.h"
#include "uipriv.h"
#include "table.h"

void uiprivValidateTableSelection(uiTableModel *m, const uiTableSelection *s)
{
	int i;
	int numRows;

	if (s == NULL)
		uiprivUserBug("uiTableSetSelection() selection must not be NULL");
	if (s->NumRows < 0)
		uiprivUserBug("uiTableSetSelection() NumRows must be >= 0");
	if (s->NumRows > 0 && s->Rows == NULL)
		uiprivUserBug("uiTableSetSelection() Rows must not be NULL when NumRows is > 0");

	numRows = uiprivTableModelNumRows(m);
	for (i = 0; i < s->NumRows; i++)
		if (s->Rows[i] < 0 || s->Rows[i] >= numRows)
			uiprivUserBug("uiTableSetSelection() row %d is out of range", s->Rows[i]);
}

void uiFreeTableSelection(uiTableSelection *s)
{
	if (s->Rows != NULL)
		uiprivFree(s->Rows);
	uiprivFree(s);
}

