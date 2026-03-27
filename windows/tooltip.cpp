#include "uipriv_windows.hpp"

struct tooltipState {
	WCHAR *text;
	std::vector<HWND> windows;
};

static HWND createTooltipForControl(HWND tool, const WCHAR *text)
{
	HWND tooltip;
	TTTOOLINFOW ti = { 0 };

	tooltip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, NULL,
		WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		tool, NULL, hInstance, NULL);
	if (tooltip == NULL) {
		logLastError(L"Failed to create tooltip window.");
		return NULL;
	}

	SetWindowPos(tooltip, HWND_TOPMOST, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

	ti.cbSize = sizeof (ti);
	ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	ti.hwnd = tool;
	ti.uId = (UINT_PTR) tool;
	ti.hinst = hInstance;
	ti.lpszText = (LPWSTR) text;
	if (!SendMessageW(tooltip, TTM_ADDTOOLW, 0, (LPARAM) &ti)) {
		logLastError(L"Failed to register tooltip tool.");
		uiWindowsEnsureDestroyWindow(tooltip);
		return NULL;
	}

	// Enable multiline tooltips.
	SendMessageW(tooltip, TTM_SETMAXTIPWIDTH, 0, 0);
	return tooltip;
}

static void addTooltipToControl(HWND tool, tooltipState *state)
{
	HWND tooltip;

	if (tool == NULL)
		return;
	tooltip = createTooltipForControl(tool, state->text);
	if (tooltip != NULL)
		state->windows.push_back(tooltip);
}

static HWND addTooltipToChild(HWND parent, HWND childAfter,
	const WCHAR *className, tooltipState *state)
{
	HWND child;

	child = FindWindowExW(parent, childAfter, className, NULL);
	if (child != NULL)
		addTooltipToControl(child, state);
	return child;
}

static void addTooltipsToChildren(HWND parent, const WCHAR *className,
	tooltipState *state)
{
	HWND child = NULL;

	do {
		child = addTooltipToChild(parent, child, className, state);
	} while (child != NULL);
}

void uiControlSetTooltip(uiControl *c, const char *tooltip)
{
	HWND parent;
	tooltipState *state;
	int slider;

	slider = c->TypeSignature == uiSliderSignature;
	if (slider && tooltip != NULL)
		uiprivSliderSetControlTooltip(uiSlider(c), 1);

	uiprivDestroyTooltip(c);
	if (tooltip == NULL) {
		if (slider)
			uiprivSliderSetControlTooltip(uiSlider(c), 0);
		return;
	}

	state = new tooltipState;
	state->text = toUTF16(tooltip);
	uiWindowsControl(c)->tooltips = state;

	parent = (HWND) uiControlHandle(c);
	addTooltipToControl(parent, state);
	switch (c->TypeSignature) {
	case uiEditableComboboxSignature:
		addTooltipsToChildren(parent, L"edit", state);
		break;
	case uiSpinboxSignature:
		addTooltipsToChildren(parent, L"edit", state);
		addTooltipsToChildren(parent, UPDOWN_CLASSW, state);
		break;
	case uiRadioButtonsSignature:
		addTooltipsToChildren(parent, L"button", state);
		break;
	default:
		break;
	}
}

void uiprivAddTooltipToHWND(uiControl *c, HWND hwnd)
{
	tooltipState *state;

	state = (tooltipState *) uiWindowsControl(c)->tooltips;
	if (state != NULL)
		addTooltipToControl(hwnd, state);
}

void uiprivDestroyTooltip(uiControl *c)
{
	tooltipState *state;

	state = (tooltipState *) uiWindowsControl(c)->tooltips;
	if (state == NULL)
		return;
	uiWindowsControl(c)->tooltips = NULL;

	for (HWND tooltip : state->windows)
		uiWindowsEnsureDestroyWindow(tooltip);
	uiprivFree(state->text);
	delete state;
}
