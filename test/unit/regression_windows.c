#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <windows.h>
#include <commctrl.h>
#include <objbase.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "../../ui.h"

struct dialogButtonState {
	uiWindow *window;
	int destroy;
	int destroyRequested;
	int changed;
	int closed;
	int timedOut;
};

static struct dialogButtonState *dialogButtonWatch;

static BOOL CALLBACK acceptButtonDialog(HWND hwnd, LPARAM data)
{
	struct dialogButtonState *s = (struct dialogButtonState *) data;
	WCHAR name[32];

	if (GetClassNameW(hwnd, name, 32) && wcscmp(name, L"#32770") == 0) {
		s->closed++;
		PostMessageW(hwnd, WM_COMMAND, IDOK, 0);
	}
	return TRUE;
}

static VOID CALLBACK dialogButtonTimeout(HWND hwnd, UINT message, UINT_PTR id,
	DWORD time)
{
	if (dialogButtonWatch == NULL)
		return;
	dialogButtonWatch->timedOut = 1;
	EnumThreadWindows(GetCurrentThreadId(), acceptButtonDialog,
		(LPARAM) dialogButtonWatch);
	if (dialogButtonWatch->closed == 0)
		PostQuitMessage(1);
}

static void finishButtonDialog(void *data)
{
	struct dialogButtonState *s = data;

	if (s->destroy) {
		uiControlDestroy(uiControl(s->window));
		s->destroyRequested = 1;
	}
	EnumThreadWindows(GetCurrentThreadId(), acceptButtonDialog, (LPARAM) s);
}

static void colorDialogChanged(uiColorButton *button, void *data)
{
	((struct dialogButtonState *) data)->changed++;
}

static void fontDialogChanged(uiFontButton *button, void *data)
{
	((struct dialogButtonState *) data)->changed++;
}

static void dialogButtonsIgnorePendingDestruction(void **state)
{
	uiInitOptions options = {0};
	struct dialogButtonState s;
	uiControl *button;
	int font, destroy, changed, closed, timedOut;
	UINT_PTR timer;

	for (font = 0; font < 2; font++)
		for (destroy = 0; destroy < 2; destroy++) {
			memset(&s, 0, sizeof s);
			s.destroy = destroy;
			assert_null(uiInit(&options));
			s.window = uiNewWindow("dialog pending destruction", 200, 100, 0);
			if (font) {
				uiFontButton *b = uiNewFontButton();
				uiFontButtonOnChanged(b, fontDialogChanged, &s);
				button = uiControl(b);
			} else {
				uiColorButton *b = uiNewColorButton();
				uiColorButtonOnChanged(b, colorDialogChanged, &s);
				button = uiControl(b);
			}
			uiWindowSetChild(s.window, button);
			uiQueueMain(finishButtonDialog, &s);
			dialogButtonWatch = &s;
			timer = SetTimer(NULL, 0, 5000, dialogButtonTimeout);
			assert_true(timer != 0);
			SendMessageW((HWND) uiControlHandle(button), BM_CLICK, 0, 0);
			KillTimer(NULL, timer);
			dialogButtonWatch = NULL;
			changed = s.changed;
			closed = s.closed;
			timedOut = s.timedOut;
			if (!s.destroyRequested)
				uiControlDestroy(uiControl(s.window));
			uiUninit();
			assert_false(timedOut);
			assert_int_equal(s.destroyRequested, destroy);
			assert_int_equal(closed, 1);
			assert_int_equal(changed, destroy ? 0 : 1);
		}
}

static void endingPathTwiceIsSafe(void **state)
{
	uiInitOptions options = {0};
	uiDrawPath *path;

	assert_null(uiInit(&options));
	path = uiDrawNewPath(uiDrawFillModeWinding);
	assert_non_null(path);
	uiDrawPathNewFigure(path, 0, 0);
	uiDrawPathLineTo(path, 1, 1);
	uiDrawPathEnd(path);
	assert_true(uiDrawPathEnded(path));
	uiDrawPathEnd(path);
	assert_true(uiDrawPathEnded(path));
	uiDrawFreePath(path);
	uiUninit();
}

static void initCanRetryAfterCOMModeConflict(void **state)
{
	uiInitOptions options = {0};
	const char *error;
	HRESULT hr;

	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	assert_true(hr == S_OK || hr == S_FALSE);
	error = uiInit(&options);
	assert_non_null(error);
	uiFreeInitError(error);
	CoUninitialize();
	error = uiInit(&options);
	if (error != NULL) {
		print_error("uiInit retry failed: %s\n", error);
		uiFreeInitError(error);
		fail();
	}
	uiUninit();
}
#ifdef _UI_STATIC
#include "../../common/uipriv.h"

extern float auditPathLength(uiDrawPath *path);

static void arcSweepDirectionMatchesEndpoint(void **state)
{
	uiInitOptions options = {0};
	const double pi = 3.14159265358979323846;
	const struct {
		double sweep;
		int negative;
		double length;
	} cases[] = {
		{ -0.5, 0, 1.5 }, { 0.5, 0, 0.5 },
		{ -0.5, 1, 0.5 }, { 0.5, 1, 1.5 },
		{ -1.5, 0, 0.5 }, { 1.5, 0, 1.5 },
		{ -1.5, 1, 1.5 }, { 1.5, 1, 0.5 },
		{ -3, 0, 1 }, { -2, 0, 0 }, { -4.5, 0, 1.5 },
		{ -3, 1, 2 }, { 3, 0, 2 },
	};
	uiDrawPath *path;
	float lengths[sizeof cases / sizeof cases[0]];
	size_t i;

	assert_null(uiInit(&options));
	for (i = 0; i < sizeof cases / sizeof cases[0]; i++) {
		path = uiDrawNewPath(uiDrawFillModeWinding);
		assert_non_null(path);
		uiDrawPathNewFigureWithArc(path, 0, 0, 100, 0,
			cases[i].sweep * pi, cases[i].negative);
		uiDrawPathEnd(path);
		lengths[i] = auditPathLength(path);
		uiDrawFreePath(path);
	}
	uiUninit();
	for (i = 0; i < sizeof cases / sizeof cases[0]; i++)
		assert_float_equal(lengths[i], cases[i].length * 100 * pi, 1);
}

extern void uiprivWindowsFailNextDestroyPostForTests(void);
extern void uiprivWindowsDeliverDestroyRetryTimerForTests(void);
extern HRESULT uiprivWindowsLogLastErrorForTests(DWORD error, DWORD *after);
extern HRESULT uiprivWindowsLogHRESULTForTests(HRESULT error, DWORD *after);
extern void uiprivWindowsBeginModalForTests(uiWindow *owner);
extern void uiprivWindowsEndModalForTests(void);

struct modalReentry {
	int called;
	int stayedDisabled;
};

static LRESULT CALLBACK modalReentrySubclass(HWND hwnd, UINT message, WPARAM wp,
	LPARAM lp, UINT_PTR id, DWORD_PTR data)
{
	struct modalReentry *r = (struct modalReentry *) data;

	if (message == WM_ENABLE && !wp && !r->called) {
		r->called = 1;
		uiprivWindowsBeginModalForTests(NULL);
		uiprivWindowsEndModalForTests();
		r->stayedDisabled = !IsWindowEnabled(hwnd);
	}
	return DefSubclassProc(hwnd, message, wp, lp);
}

static void nestedModalPreservesWindowState(void **state)
{
	uiInitOptions options = {0};
	uiWindow *owner, *other, *initiallyDisabled, *created;
	HWND ownerHWND, otherHWND, disabledHWND;
	struct modalReentry reentry = {0};

	assert_null(uiInit(&options));
	owner = uiNewWindow("modal owner", 100, 100, 0);
	other = uiNewWindow("modal other", 100, 100, 0);
	initiallyDisabled = uiNewWindow("modal disabled", 100, 100, 0);
	ownerHWND = (HWND) uiControlHandle(uiControl(owner));
	otherHWND = (HWND) uiControlHandle(uiControl(other));
	disabledHWND = (HWND) uiControlHandle(uiControl(initiallyDisabled));
	uiControlDisable(uiControl(initiallyDisabled));
	assert_true(SetWindowSubclass(otherHWND, modalReentrySubclass, 1, (DWORD_PTR) &reentry));
	uiprivWindowsBeginModalForTests(owner);
	assert_true(reentry.called);
	assert_true(reentry.stayedDisabled);
	assert_true(IsWindowEnabled(ownerHWND));
	assert_false(IsWindowEnabled(otherHWND));
	uiControlEnable(uiControl(initiallyDisabled));
	assert_false(IsWindowEnabled(disabledHWND));
	created = uiNewWindow("created while modal", 100, 100, 0);
	assert_false(IsWindowEnabled((HWND) uiControlHandle(uiControl(created))));
	uiControlDestroy(uiControl(created));
	uiprivWindowsBeginModalForTests(NULL);
	uiprivWindowsEndModalForTests();
	assert_false(IsWindowEnabled(otherHWND));
	assert_false(IsWindowEnabled(disabledHWND));
	uiprivWindowsEndModalForTests();
	assert_true(IsWindowEnabled(otherHWND));
	assert_true(IsWindowEnabled(disabledHWND));
	EnableWindow(ownerHWND, FALSE);
	uiprivWindowsBeginModalForTests(NULL);
	uiprivWindowsEndModalForTests();
	assert_false(IsWindowEnabled(ownerHWND));
	RemoveWindowSubclass(otherHWND, modalReentrySubclass, 1);
	uiControlDestroy(uiControl(initiallyDisabled));
	uiControlDestroy(uiControl(other));
	uiControlDestroy(uiControl(owner));
	uiUninit();
}

static void errorLogsReturnWithoutBreaking(void **state)
{
	DWORD after;

	assert_int_equal(uiprivWindowsLogLastErrorForTests(ERROR_ACCESS_DENIED, &after),
		HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
	assert_int_equal(after, ERROR_ACCESS_DENIED);
	assert_int_equal(uiprivWindowsLogLastErrorForTests(ERROR_SUCCESS, &after), E_FAIL);
	assert_int_equal(after, ERROR_SUCCESS);
	assert_int_equal(uiprivWindowsLogHRESULTForTests(E_INVALIDARG, &after), E_INVALIDARG);
	assert_int_equal(after, ERROR_ACCESS_DENIED);
}

static void countDestroyedControl(uiControl *c, void *data)
{
	(*(int *) data)++;
}

static void deferredDestroySurvivesFailedPost(void **state)
{
	uiInitOptions options = {0};
	uiButton *button;
	int destroyed = 0;

	assert_null(uiInit(&options));
	button = uiNewButton("boundary retry");
	uiControlOnDestroyed(uiControl(button), countDestroyedControl, &destroyed);
	uiprivWindowsFailNextDestroyPostForTests();
	uiprivUserCallbackEnter(NULL);
	uiControlDestroy(uiControl(button));
	uiprivUserCallbackLeave();
	assert_int_equal(destroyed, 0);
	uiMainStep(0);
	assert_int_equal(destroyed, 1);
	uiUninit();
}

static void deferredDestroyTimerWaitsForCallback(void **state)
{
	uiInitOptions options = {0};
	uiButton *button;
	int destroyed = 0;
	int i;

	assert_null(uiInit(&options));
	button = uiNewButton("timer retry");
	uiControlOnDestroyed(uiControl(button), countDestroyedControl, &destroyed);
	uiprivWindowsFailNextDestroyPostForTests();
	uiprivUserCallbackEnter(NULL);
	uiControlDestroy(uiControl(button));
	uiprivUserCallbackLeave();
	uiprivUserCallbackEnter(NULL);
	uiprivWindowsDeliverDestroyRetryTimerForTests();
	assert_int_equal(destroyed, 0);
	uiprivUserCallbackLeave();
	for (i = 0; i < 100 && destroyed == 0; i++)
		uiMainStep(0);
	assert_int_equal(destroyed, 1);
	uiUninit();
}

static void deferredDestroyTimerRunsInNativeLoop(void **state)
{
	uiInitOptions options = {0};
	uiButton *button;
	int destroyed = 0;
	ULONGLONG deadline;
	MSG msg;

	assert_null(uiInit(&options));
	button = uiNewButton("native timer retry");
	uiControlOnDestroyed(uiControl(button), countDestroyedControl, &destroyed);
	uiprivWindowsFailNextDestroyPostForTests();
	uiprivUserCallbackEnter(NULL);
	uiControlDestroy(uiControl(button));
	uiprivUserCallbackLeave();
	deadline = GetTickCount64() + 3000;
	while (destroyed == 0 && GetTickCount64() < deadline) {
		if (!PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
			Sleep(1);
			continue;
		}
		if (msg.message == WM_QUIT)
			fail_msg("%s", "unexpected WM_QUIT while awaiting retry timer");
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	assert_int_equal(destroyed, 1);
	uiUninit();
}
#endif

struct tableState {
	uiTableModelHandler mh;
	uiTableModel *model;
	uiWindow *window;
	uiTable *table;
	char rows[4][64];
	int checked[4];
	int numRows;
	int setterCalls;
	int setterRow;
	int setterColumn;
	int resizeInSetter;
	int destroyWindowInSetter;
	int headerCalls;
};

static int tableNumColumns(uiTableModelHandler *mh, uiTableModel *m)
{
	return 2;
}

static uiTableValueType tableColumnType(uiTableModelHandler *mh, uiTableModel *m, int column)
{
	return column == 0 ? uiTableValueTypeString : uiTableValueTypeInt;
}

static int tableNumRows(uiTableModelHandler *mh, uiTableModel *m)
{
	return ((struct tableState *) mh)->numRows;
}

static uiTableValue *tableCellValue(uiTableModelHandler *mh, uiTableModel *m, int row, int column)
{
	struct tableState *s = (struct tableState *) mh;
	if (column == 0)
		return uiNewTableValueString(s->rows[row]);
	return uiNewTableValueInt(s->checked[row]);
}

static void tableSetCellValue(uiTableModelHandler *mh, uiTableModel *m,
	int row, int column, const uiTableValue *value)
{
	struct tableState *s = (struct tableState *) mh;
	s->setterCalls++;
	s->setterRow = row;
	s->setterColumn = column;
	if (column == 0) {
		strncpy(s->rows[row], uiTableValueString(value), sizeof s->rows[row] - 1);
		s->rows[row][sizeof s->rows[row] - 1] = '\0';
	} else
		s->checked[row] = uiTableValueInt(value);
	if (s->resizeInSetter)
		uiTableColumnSetWidth(s->table, 0, 170);
	if (s->destroyWindowInSetter)
		uiControlDestroy(uiControl(s->window));
}

static int tableSetup(void **data)
{
	uiInitOptions options = {0};
	struct tableState *s;
	uiTableParams params = {0};
	int i;

	assert_null(uiInit(&options));
	s = calloc(1, sizeof *s);
	assert_non_null(s);
	s->mh.NumColumns = tableNumColumns;
	s->mh.ColumnType = tableColumnType;
	s->mh.NumRows = tableNumRows;
	s->mh.CellValue = tableCellValue;
	s->mh.SetCellValue = tableSetCellValue;
	s->numRows = 3;
	for (i = 0; i < s->numRows; i++)
		s->rows[i][0] = 'A' + i;
	s->model = uiNewTableModel(&s->mh);
	s->window = uiNewWindow("Windows audit table", 400, 220, 0);
	params.Model = s->model;
	params.RowBackgroundColorModelColumn = -1;
	s->table = uiNewTable(&params);
	uiTableAppendTextColumn(s->table, "Text", 0,
		uiTableModelColumnAlwaysEditable, NULL);
	uiWindowSetChild(s->window, uiControl(s->table));
	uiControlShow(uiControl(s->window));
	for (i = 0; i < 20; i++)
		uiMainStep(0);
	*data = s;
	return 0;
}

static int tableTeardown(void **data)
{
	struct tableState *s = *data;
	uiControlDestroy(uiControl(s->window));
	uiFreeTableModel(s->model);
	uiUninit();
	free(s);
	return 0;
}

static HWND tableHWND(struct tableState *s)
{
	return (HWND) uiControlHandle(uiControl(s->table));
}

static HWND openTableEditor(struct tableState *s, int row)
{
	NMITEMACTIVATE nm = {0};
	RECT r = {0};
	HWND table = tableHWND(s);
	assert_true(ListView_GetItemRect(table, row, &r, LVIR_BOUNDS));
	nm.hdr.hwndFrom = table;
	nm.hdr.idFrom = GetDlgCtrlID(table);
	nm.hdr.code = NM_CLICK;
	nm.ptAction.x = r.left + 45;
	nm.ptAction.y = (r.top + r.bottom) / 2;
	SendMessageW((HWND) uiControlHandle(uiControl(s->window)),
		WM_NOTIFY, nm.hdr.idFrom, (LPARAM) &nm);
	return FindWindowExW(table, NULL, L"Edit", NULL);
}

static void countTableHeaderClick(uiTable *t, int column, void *data)
{
	struct tableState *s = data;

	s->headerCalls++;
}

static void headerClickStopsAfterSetterDestroysWindow(void **data)
{
	struct tableState *s = *data;
	HWND table = tableHWND(s);
	NMLISTVIEW nm = {0};

	assert_non_null(openTableEditor(s, 0));
	uiTableHeaderOnClicked(s->table, countTableHeaderClick, s);
	s->destroyWindowInSetter = 1;
	nm.hdr.hwndFrom = table;
	nm.hdr.idFrom = GetDlgCtrlID(table);
	nm.hdr.code = LVN_COLUMNCLICK;
	nm.iSubItem = 0;
	SendMessageW((HWND) uiControlHandle(uiControl(s->window)),
		WM_NOTIFY, nm.hdr.idFrom, (LPARAM) &nm);
	assert_int_equal(s->setterCalls, 1);
	assert_int_equal(s->headerCalls, 0);
}

static void editFollowsInsertedRow(void **data)
{
	struct tableState *s = *data;
	HWND edit = openTableEditor(s, 1);
	assert_non_null(edit);
	strcpy(s->rows[3], s->rows[2]);
	strcpy(s->rows[2], s->rows[1]);
	strcpy(s->rows[1], s->rows[0]);
	strcpy(s->rows[0], "X");
	s->numRows++;
	uiTableModelRowInserted(s->model, 0);
	assert_true(IsWindow(edit));
	SetWindowTextW(edit, L"edited B");
	SendMessageW(edit, WM_KEYDOWN, VK_RETURN, 0);
	assert_int_equal(s->setterCalls, 1);
	assert_int_equal(s->setterRow, 2);
	assert_string_equal(s->rows[2], "edited B");
	assert_string_equal(s->rows[1], "A");
}

static void editSetterRunsOnceUnderResizeReentry(void **data)
{
	struct tableState *s = *data;
	HWND edit = openTableEditor(s, 1);
	assert_non_null(edit);
	s->resizeInSetter = 1;
	SetWindowTextW(edit, L"edited B");
	SendMessageW(edit, WM_KEYDOWN, VK_RETURN, 0);
	assert_int_equal(s->setterCalls, 1);
	assert_int_equal(s->setterRow, 1);
	assert_string_equal(s->rows[1], "edited B");
}

static void editFollowsDeletedEarlierRow(void **data)
{
	struct tableState *s = *data;
	HWND edit = openTableEditor(s, 1);
	assert_non_null(edit);
	strcpy(s->rows[0], s->rows[1]);
	strcpy(s->rows[1], s->rows[2]);
	s->numRows--;
	uiTableModelRowDeleted(s->model, 0);
	if (IsWindow(edit)) {
		SetWindowTextW(edit, L"edited B");
		SendMessageW(edit, WM_KEYDOWN, VK_RETURN, 0);
		assert_int_equal(s->setterCalls, 1);
		assert_int_equal(s->setterRow, 0);
		assert_string_equal(s->rows[0], "edited B");
	} else {
		// Native ListView may cancel editing while deleting the earlier row.
		assert_int_equal(s->setterCalls, 0);
		assert_string_equal(s->rows[0], "B");
	}
	assert_string_equal(s->rows[1], "C");
}

static void deletingEditedRowCancelsEdit(void **data)
{
	struct tableState *s = *data;
	HWND edit = openTableEditor(s, 1);
	assert_non_null(edit);
	strcpy(s->rows[1], s->rows[2]);
	s->numRows--;
	uiTableModelRowDeleted(s->model, 1);
	assert_false(IsWindow(edit));
	assert_int_equal(s->setterCalls, 0);
	assert_string_equal(s->rows[1], "C");
}

static void checkboxTextIconChangesCheckbox(void **data)
{
	struct tableState *s = *data;
	NMITEMACTIVATE nm = {0};
	RECT icon = {0};
	HWND table = tableHWND(s);
	uiTableAppendCheckboxTextColumn(s->table, "Combined", 1,
		uiTableModelColumnAlwaysEditable, 0,
		uiTableModelColumnAlwaysEditable, NULL);
	uiTableColumnSetWidth(s->table, 1, 160);
	assert_true(ListView_GetSubItemRect(table, 0, 1, LVIR_ICON, &icon));
	nm.hdr.hwndFrom = table;
	nm.hdr.idFrom = GetDlgCtrlID(table);
	nm.hdr.code = NM_CLICK;
	nm.ptAction.x = (icon.left + icon.right) / 2;
	nm.ptAction.y = (icon.top + icon.bottom) / 2;
	SendMessageW((HWND) uiControlHandle(uiControl(s->window)),
		WM_NOTIFY, nm.hdr.idFrom, (LPARAM) &nm);
	assert_int_equal(s->setterCalls, 1);
	assert_int_equal(s->setterColumn, 1);
	assert_int_equal(s->checked[0], 1);
}

static void checkboxTextLabelEditsText(void **data)
{
	struct tableState *s = *data;
	NMITEMACTIVATE nm = {0};
	RECT label = {0};
	HWND table = tableHWND(s);
	HWND edit;
	uiTableAppendCheckboxTextColumn(s->table, "Combined", 1,
		uiTableModelColumnAlwaysEditable, 0,
		uiTableModelColumnAlwaysEditable, NULL);
	uiTableColumnSetWidth(s->table, 1, 160);
	assert_true(ListView_GetSubItemRect(table, 0, 1, LVIR_LABEL, &label));
	nm.hdr.hwndFrom = table;
	nm.hdr.idFrom = GetDlgCtrlID(table);
	nm.hdr.code = NM_CLICK;
	nm.ptAction.x = label.right - 30;
	nm.ptAction.y = (label.top + label.bottom) / 2;
	SendMessageW((HWND) uiControlHandle(uiControl(s->window)),
		WM_NOTIFY, nm.hdr.idFrom, (LPARAM) &nm);
	edit = FindWindowExW(table, NULL, L"Edit", NULL);
	assert_non_null(edit);
	SetWindowTextW(edit, L"edited A");
	SendMessageW(edit, WM_KEYDOWN, VK_RETURN, 0);
	assert_int_equal(s->setterCalls, 1);
	assert_int_equal(s->setterColumn, 0);
	assert_string_equal(s->rows[0], "edited A");
	assert_int_equal(s->checked[0], 0);
}

static void tableSharesImageListOwnership(void **data)
{
	struct tableState *s = *data;
	assert_true((GetWindowLongPtrW(tableHWND(s), GWL_STYLE) &
		LVS_SHAREIMAGELISTS) != 0);
}

struct timerState {
	int calls;
};

static int reentrantTimer(void *data)
{
	struct timerState *s = data;
	int i;
	s->calls++;
	Sleep(20);
	for (i = 0; i < 20; i++)
		uiMainStep(0);
	uiQuit();
	return 0;
}

static void timerStopsOnceUnderNestedMessageLoop(void **data)
{
	uiInitOptions options = {0};
	struct timerState s = {0};
	assert_null(uiInit(&options));
	uiTimer(1, reentrantTimer, &s);
	uiMain();
	assert_int_equal(s.calls, 1);
	uiUninit();
}

static void longMultilineInput(void **data)
{
	uiInitOptions options = {0};
	uiMultilineEntry *e;
	uiWindow *w;
	WCHAR *input;
	char *actual;
	int i;
	assert_null(uiInit(&options));
	w = uiNewWindow("Long input", 200, 120, 0);
	e = uiNewMultilineEntry();
	uiWindowSetChild(w, uiControl(e));
	uiControlShow(uiControl(w));
	input = malloc(40001 * sizeof *input);
	assert_non_null(input);
	for (i = 0; i < 40000; i++)
		input[i] = L'x';
	input[40000] = 0;
	SendMessageW((HWND) uiControlHandle(uiControl(e)), EM_REPLACESEL,
		TRUE, (LPARAM) input);
	actual = uiMultilineEntryText(e);
	assert_int_equal(strlen(actual), 40000);
	uiFreeText(actual);
	free(input);
	uiControlDestroy(uiControl(w));
	uiUninit();
}

static void longEditableComboInput(void **data)
{
	uiInitOptions options = {0};
	uiEditableCombobox *combo;
	uiWindow *w;
	COMBOBOXINFO info = {0};
	WCHAR input[257];
	char *actual;
	int i;
	assert_null(uiInit(&options));
	w = uiNewWindow("Long combo input", 200, 120, 0);
	combo = uiNewEditableCombobox();
	uiWindowSetChild(w, uiControl(combo));
	uiControlShow(uiControl(w));
	info.cbSize = sizeof info;
	assert_true(GetComboBoxInfo((HWND) uiControlHandle(uiControl(combo)), &info));
	for (i = 0; i < 256; i++)
		input[i] = L'x';
	input[256] = 0;
	SendMessageW(info.hwndItem, EM_REPLACESEL, TRUE, (LPARAM) input);
	actual = uiEditableComboboxText(combo);
	assert_int_equal(strlen(actual), 256);
	uiFreeText(actual);
	uiControlDestroy(uiControl(w));
	uiUninit();
}

static void enabledStateAfterReparent(void **data)
{
	uiInitOptions options = {0};
	uiWindow *left, *right;
	uiBox *disabledParent, *enabledParent;
	uiButton *button;
	HWND buttonHWND;

	assert_null(uiInit(&options));
	left = uiNewWindow("Disabled parent", 200, 120, 0);
	right = uiNewWindow("Enabled parent", 200, 120, 0);
	disabledParent = uiNewHorizontalBox();
	enabledParent = uiNewHorizontalBox();
	button = uiNewButton("Move me");
	uiWindowSetChild(left, uiControl(disabledParent));
	uiWindowSetChild(right, uiControl(enabledParent));
	uiBoxAppend(disabledParent, uiControl(button), 0);
	buttonHWND = (HWND) uiControlHandle(uiControl(button));
	uiControlDisable(uiControl(disabledParent));
	assert_false(IsWindowEnabled(buttonHWND));
	uiBoxDelete(disabledParent, 0);
	uiBoxAppend(enabledParent, uiControl(button), 0);
	uiControlShow(uiControl(left));
	uiControlShow(uiControl(right));
	assert_true(uiControlEnabledToUser(uiControl(button)));
	assert_true(IsWindowEnabled(buttonHWND));
	uiControlDisable(uiControl(button));
	uiBoxDelete(enabledParent, 0);
	uiBoxAppend(disabledParent, uiControl(button), 0);
	uiBoxDelete(disabledParent, 0);
	uiBoxAppend(enabledParent, uiControl(button), 0);
	assert_false(uiControlEnabledToUser(uiControl(button)));
	assert_false(IsWindowEnabled(buttonHWND));
	uiControlDestroy(uiControl(left));
	uiControlDestroy(uiControl(right));
	uiUninit();
}

struct areaState {
	uiAreaHandler handler;
	uiWindow *window;
	uiArea *area;
	int drawCalls;
	int queueInDraw;
	int leaveCalls;
	int keyCalls;
	int destroyCallbackRan;
	int clipTest;
	uiDrawTextAlign textAlign;
	int textTest;
};

static void areaDraw(uiAreaHandler *handler, uiArea *area, uiAreaDrawParams *params)
{
	struct areaState *s = (struct areaState *) handler;
	s->drawCalls++;
	if (s->queueInDraw && s->drawCalls == 1)
		uiAreaQueueRedrawAll(area);
	if (s->clipTest) {
		uiDrawPath *path;
		uiDrawMatrix matrix;
		uiDrawBrush brush = {0};
		path = uiDrawNewPath(uiDrawFillModeWinding);
		uiDrawPathAddRectangle(path, 0, 0, 100, 80);
		uiDrawPathEnd(path);
		uiDrawClip(params->Context, path);
		uiDrawFreePath(path);
		uiDrawMatrixSetIdentity(&matrix);
		uiDrawMatrixTranslate(&matrix, 200, 0);
		uiDrawTransform(params->Context, &matrix);
		if (s->clipTest == 2) {
			path = uiDrawNewPath(uiDrawFillModeWinding);
			uiDrawPathAddRectangle(path, 0, 0, 100, 80);
			uiDrawPathEnd(path);
			uiDrawClip(params->Context, path);
			uiDrawFreePath(path);
		}
		path = uiDrawNewPath(uiDrawFillModeWinding);
		uiDrawPathAddRectangle(path, 0, 0, 100, 80);
		uiDrawPathEnd(path);
		brush.Type = uiDrawBrushTypeSolid;
		brush.R = 1;
		brush.A = 1;
		uiDrawFill(params->Context, path, &brush);
		uiDrawFreePath(path);
	}
	if (s->textTest) {
		uiFontDescriptor font;
		uiAttributedString *string;
		uiDrawTextLayoutParams layoutParams;
		uiDrawTextLayout *layout;
		uiLoadControlFont(&font);
		string = uiNewAttributedString("Visible text");
		layoutParams.String = string;
		layoutParams.DefaultFont = &font;
		layoutParams.Width = -1;
		layoutParams.Align = s->textAlign;
		layout = uiDrawNewTextLayout(&layoutParams);
		if (layout != NULL) {
			uiDrawText(params->Context, layout, 20, 20);
			uiDrawFreeTextLayout(layout);
		}
		uiFreeAttributedString(string);
		uiFreeFontDescriptor(&font);
	}
}

static void areaMouse(uiAreaHandler *handler, uiArea *area, uiAreaMouseEvent *event) {}
static void areaCrossed(uiAreaHandler *handler, uiArea *area, int left)
{
	if (left)
		((struct areaState *) handler)->leaveCalls++;
}
static void areaDragBroken(uiAreaHandler *handler, uiArea *area) {}
static int areaKey(uiAreaHandler *handler, uiArea *area, uiAreaKeyEvent *event)
{
	((struct areaState *) handler)->keyCalls++;
	return 0;
}

static int areaSetup(void **data)
{
	uiInitOptions options = {0};
	struct areaState *s;
	int i;
	assert_null(uiInit(&options));
	s = calloc(1, sizeof *s);
	assert_non_null(s);
	s->handler.Draw = areaDraw;
	s->handler.MouseEvent = areaMouse;
	s->handler.MouseCrossed = areaCrossed;
	s->handler.DragBroken = areaDragBroken;
	s->handler.KeyEvent = areaKey;
	s->window = uiNewWindow("Windows audit area", 420, 220, 0);
	s->area = uiNewScrollingArea(&s->handler, 300, 300);
	uiWindowSetChild(s->window, uiControl(s->area));
	uiControlShow(uiControl(s->window));
	for (i = 0; i < 20; i++)
		uiMainStep(0);
	*data = s;
	return 0;
}

static int areaTeardown(void **data)
{
	struct areaState *s = *data;
	uiControlDestroy(uiControl(s->window));
	uiUninit();
	free(s);
	return 0;
}

static HWND areaHWND(struct areaState *s)
{
	return (HWND) uiControlHandle(uiControl(s->area));
}

extern BOOL auditAreaInside(const uiArea *a);
extern BOOL auditAreaCapturing(const uiArea *a);
extern BOOL auditAreaTracking(const uiArea *a);

static void mouseTrackingResumesAfterClick(void **data)
{
	struct areaState *s = *data;
	HWND hwnd = areaHWND(s);
	LPARAM point = MAKELPARAM(20, 20);
	// Synthetic button messages do not control the desktop's mouse state.
	// Check Area's tracking transition instead of querying native TME state.
	SendMessageW(hwnd, WM_MOUSEMOVE, 0, point);
	assert_true(auditAreaInside(s->area));
	assert_true(auditAreaTracking(s->area));
	SendMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, point);
	assert_true(auditAreaCapturing(s->area));
	assert_false(auditAreaTracking(s->area));
	SendMessageW(hwnd, WM_LBUTTONUP, 0, point);
	assert_false(auditAreaCapturing(s->area));
	assert_true(auditAreaInside(s->area));
	assert_true(auditAreaTracking(s->area));
}

static void horizontalWheelMovesRight(void **data)
{
	struct areaState *s = *data;
	SCROLLINFO si = {0};
	uiAreaSetSize(s->area, 1000, 300);
	SendMessageW(areaHWND(s), WM_MOUSEHWHEEL, MAKEWPARAM(0, 120), 0);
	si.cbSize = sizeof si;
	si.fMask = SIF_POS;
	assert_true(GetScrollInfo(areaHWND(s), SB_HORZ, &si));
	assert_true(si.nPos > 0);
}

static void redrawQueuedInsideDrawSurvives(void **data)
{
	struct areaState *s = *data;
	RECT update;
	s->drawCalls = 0;
	s->queueInDraw = 1;
	uiAreaQueueRedrawAll(s->area);
	UpdateWindow(areaHWND(s));
	assert_int_equal(s->drawCalls, 1);
	assert_true(GetUpdateRect(areaHWND(s), &update, FALSE));
	UpdateWindow(areaHWND(s));
	assert_int_equal(s->drawCalls, 2);
}

static void largeScrollCoordinatesClamp(void **data)
{
	struct areaState *s = *data;
	SCROLLINFO si = {0};
	uiAreaSetSize(s->area, 1000, 300);
	uiAreaScrollTo(s->area, 1e12, 0, 1, 1);
	si.cbSize = sizeof si;
	si.fMask = SIF_POS | SIF_PAGE | SIF_RANGE;
	assert_true(GetScrollInfo(areaHWND(s), SB_HORZ, &si));
	assert_int_equal(si.nPos, si.nMax - (int) si.nPage + 1);
}

static void destroyAreaAndSendKey(void *data)
{
	struct areaState *s = data;
	HWND hwnd = areaHWND(s);
	uiWindowSetChild(s->window, NULL);
	uiControlDestroy(uiControl(s->area));
	// This is the internal key message sent by the Windows message filter.
	SendMessageW(hwnd, WM_USER + 0x40, 'A', 0);
	s->area = NULL;
	s->destroyCallbackRan = 1;
}

static void pendingDestroySuppressesKey(void **data)
{
	struct areaState *s = *data;
	int i;
	uiQueueMain(destroyAreaAndSendKey, s);
	for (i = 0; i < 100 && !s->destroyCallbackRan; i++)
		uiMainStep(0);
	assert_true(s->destroyCallbackRan);
	assert_int_equal(s->keyCalls, 0);
}

static void scrollbarVisibilityIsHistoryIndependent(void **data)
{
	struct areaState *s = *data;
	HWND hwnd = areaHWND(s);
	LONG_PTR fromSmall, fromLarge;
	SetWindowPos(hwnd, NULL, 0, 0, 250, 250, SWP_NOMOVE | SWP_NOZORDER);
	SetWindowPos(hwnd, NULL, 0, 0, 300, 300, SWP_NOMOVE | SWP_NOZORDER);
	fromSmall = GetWindowLongPtrW(hwnd, GWL_STYLE) & (WS_HSCROLL | WS_VSCROLL);
	SetWindowPos(hwnd, NULL, 0, 0, 340, 340, SWP_NOMOVE | SWP_NOZORDER);
	SetWindowPos(hwnd, NULL, 0, 0, 300, 300, SWP_NOMOVE | SWP_NOZORDER);
	fromLarge = GetWindowLongPtrW(hwnd, GWL_STYLE) & (WS_HSCROLL | WS_VSCROLL);
	assert_int_equal(fromSmall, fromLarge);
	assert_int_equal(fromLarge, 0);
}

static void clipDoesNotFollowLaterTransform(void **data)
{
	struct areaState *s = *data;
	HDC screen, dc;
	HBITMAP bitmap, oldBitmap;
	COLORREF covered, uncovered;
	SetWindowPos(areaHWND(s), NULL, 0, 0, 400, 100,
		SWP_NOMOVE | SWP_NOZORDER);
	s->clipTest = 1;
	screen = GetDC(areaHWND(s));
	dc = CreateCompatibleDC(screen);
	bitmap = CreateCompatibleBitmap(screen, 400, 100);
	oldBitmap = SelectObject(dc, bitmap);
	SendMessageW(areaHWND(s), WM_PRINTCLIENT, (WPARAM) dc, 0);
	covered = GetPixel(dc, 250, 40);
	uncovered = GetPixel(dc, 350, 40);
	assert_int_equal(covered, uncovered);
	SelectObject(dc, oldBitmap);
	DeleteObject(bitmap);
	DeleteDC(dc);
	ReleaseDC(areaHWND(s), screen);
}

static void clipIntersectionUsesSetTimeTransforms(void **data)
{
	struct areaState *s = *data;
	HDC screen, dc;
	HBITMAP bitmap, oldBitmap;
	COLORREF covered, uncovered;
	SetWindowPos(areaHWND(s), NULL, 0, 0, 400, 100,
		SWP_NOMOVE | SWP_NOZORDER);
	s->clipTest = 2;
	screen = GetDC(areaHWND(s));
	dc = CreateCompatibleDC(screen);
	bitmap = CreateCompatibleBitmap(screen, 400, 100);
	oldBitmap = SelectObject(dc, bitmap);
	SendMessageW(areaHWND(s), WM_PRINTCLIENT, (WPARAM) dc, 0);
	covered = GetPixel(dc, 250, 40);
	uncovered = GetPixel(dc, 350, 40);
	assert_int_equal(covered, uncovered);
	SelectObject(dc, oldBitmap);
	DeleteObject(bitmap);
	DeleteDC(dc);
	ReleaseDC(areaHWND(s), screen);
}

static void unwrappedAlignedTextIsVisible(void **data)
{
	struct areaState *s = *data;
	HDC screen, dc;
	HBITMAP bitmap, oldBitmap;
	int pass, x, y, darkPixels;
	COLORREF pixel;
	SetWindowPos(areaHWND(s), NULL, 0, 0, 400, 100,
		SWP_NOMOVE | SWP_NOZORDER);
	s->textTest = 1;
	screen = GetDC(areaHWND(s));
	dc = CreateCompatibleDC(screen);
	bitmap = CreateCompatibleBitmap(screen, 400, 100);
	oldBitmap = SelectObject(dc, bitmap);
	for (pass = 0; pass < 2; pass++) {
		s->textAlign = pass == 0 ? uiDrawTextAlignCenter : uiDrawTextAlignRight;
		SendMessageW(areaHWND(s), WM_PRINTCLIENT, (WPARAM) dc, 0);
		darkPixels = 0;
		for (y = 20; y < 70; y++)
			for (x = 20; x < 160; x++) {
				pixel = GetPixel(dc, x, y);
				if (GetRValue(pixel) < 100 && GetGValue(pixel) < 100 &&
					GetBValue(pixel) < 100)
					darkPixels++;
			}
		assert_true(darkPixels > 0);
	}
	SelectObject(dc, oldBitmap);
	DeleteObject(bitmap);
	DeleteDC(dc);
	ReleaseDC(areaHWND(s), screen);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(headerClickStopsAfterSetterDestroysWindow, tableSetup, tableTeardown),
		cmocka_unit_test(dialogButtonsIgnorePendingDestruction),
		cmocka_unit_test(endingPathTwiceIsSafe),
		cmocka_unit_test(initCanRetryAfterCOMModeConflict),
#ifdef _UI_STATIC
		cmocka_unit_test(arcSweepDirectionMatchesEndpoint),
		cmocka_unit_test(nestedModalPreservesWindowState),
		cmocka_unit_test(errorLogsReturnWithoutBreaking),
		cmocka_unit_test(deferredDestroySurvivesFailedPost),
		cmocka_unit_test(deferredDestroyTimerWaitsForCallback),
		cmocka_unit_test(deferredDestroyTimerRunsInNativeLoop),
#endif
		cmocka_unit_test_setup_teardown(editFollowsInsertedRow, tableSetup, tableTeardown),
		cmocka_unit_test_setup_teardown(editSetterRunsOnceUnderResizeReentry, tableSetup, tableTeardown),
		cmocka_unit_test_setup_teardown(editFollowsDeletedEarlierRow, tableSetup, tableTeardown),
		cmocka_unit_test_setup_teardown(deletingEditedRowCancelsEdit, tableSetup, tableTeardown),
		cmocka_unit_test_setup_teardown(checkboxTextIconChangesCheckbox, tableSetup, tableTeardown),
		cmocka_unit_test_setup_teardown(checkboxTextLabelEditsText, tableSetup, tableTeardown),
		cmocka_unit_test_setup_teardown(tableSharesImageListOwnership, tableSetup, tableTeardown),
		cmocka_unit_test(timerStopsOnceUnderNestedMessageLoop),
		cmocka_unit_test(longMultilineInput),
		cmocka_unit_test(longEditableComboInput),
		cmocka_unit_test(enabledStateAfterReparent),
		cmocka_unit_test_setup_teardown(mouseTrackingResumesAfterClick, areaSetup, areaTeardown),
		cmocka_unit_test_setup_teardown(horizontalWheelMovesRight, areaSetup, areaTeardown),
		cmocka_unit_test_setup_teardown(redrawQueuedInsideDrawSurvives, areaSetup, areaTeardown),
		cmocka_unit_test_setup_teardown(largeScrollCoordinatesClamp, areaSetup, areaTeardown),
		cmocka_unit_test_setup_teardown(pendingDestroySuppressesKey, areaSetup, areaTeardown),
		cmocka_unit_test_setup_teardown(scrollbarVisibilityIsHistoryIndependent, areaSetup, areaTeardown),
		cmocka_unit_test_setup_teardown(clipDoesNotFollowLaterTransform, areaSetup, areaTeardown),
		cmocka_unit_test_setup_teardown(clipIntersectionUsesSetTimeTransforms, areaSetup, areaTeardown),
		cmocka_unit_test_setup_teardown(unwrappedAlignedTextIsVisible, areaSetup, areaTeardown),
	};
	return cmocka_run_group_tests_name("windows regression", tests, NULL, NULL);
}
