#ifndef LIBUI_TABLE_TEST_H
#define LIBUI_TABLE_TEST_H

#include "unit.h"

/* Private to the Table tests. The window owns its attached table. Teardown
 * destroys the window before releasing the model and image, then uninitializes
 * libui. Keep the handler first: callbacks recover the fixture from its address.
 */
struct tableTestState {
	uiTableModelHandler mh;
	uiTableModel *model;
	uiWindow *window;
	uiTable *table;
	uiImage *image;
	const char *rows[2];
	int numRows;
};

int tableTestSetup(void **data);
int tableTestTeardown(void **data);
void settleTableLayout(void);
void attachAndShowTable(struct tableTestState *state);
void makeBareTable(struct tableTestState *state);

/* Cases exported only to the existing tableRunUnitTests() registration array. */
void tableValueRoundTrips(void **data);
void autoWidthUsesCurrentContentOnce(void **data);
void autoWidthUsesEditableContent(void **data);
void autoWidthIncludesHeaderForEmptyModel(void **data);
void autoWidthIncludesSortIndicator(void **data);
#ifdef __APPLE__
void autoWidthDoesNotCollapseFixedContentDarwin(void **data);
#endif
void outOfRangeColumnIsIgnored(void **data);
#ifdef _WIN32
void autoWidthDoesNotExpandLastWindowsColumn(void **data);
#endif

#endif
