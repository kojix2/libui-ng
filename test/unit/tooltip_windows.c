#include "../../ui.h"
#include <windows.h>
#include <commctrl.h>

struct tooltipSearch {
	HWND tool;
	int matches;
};

static BOOL CALLBACK findTooltip(HWND hwnd, LPARAM data)
{
	struct tooltipSearch *search = (struct tooltipSearch *) data;
	WCHAR className[64];
	TTTOOLINFOW info = { 0 };

	if (!GetClassNameW(hwnd, className, 64) ||
		lstrcmpW(className, TOOLTIPS_CLASSW) != 0)
		return TRUE;
	if (search->tool == NULL) {
		search->matches++;
		return TRUE;
	}
	info.cbSize = sizeof info;
	info.hwnd = search->tool;
	info.uId = (UINT_PTR) search->tool;
	if (SendMessageW(hwnd, TTM_GETTOOLINFOW, 0, (LPARAM) &info))
		search->matches++;
	return TRUE;
}

static int tooltipsFor(HWND tool)
{
	struct tooltipSearch search = { 0 };

	search.tool = tool;
	EnumThreadWindows(GetCurrentThreadId(), findTooltip, (LPARAM) &search);
	return search.matches;
}

static int spinboxTooltipSurvivesRelayout(void)
{
	uiSpinbox *spinbox;
	HWND container, oldUpdown, newUpdown;
	int ok, count, i;

	spinbox = uiNewSpinbox(0, 10);
	container = (HWND) uiControlHandle(uiControl(spinbox));
	oldUpdown = FindWindowExW(container, NULL, UPDOWN_CLASSW, NULL);
	uiControlSetTooltip(uiControl(spinbox), "spinbox tooltip");
	ok = oldUpdown != NULL && tooltipsFor(oldUpdown) == 1;
	count = tooltipsFor(NULL);
	for (i = 0; i < 3; i++) {
		MoveWindow(container, 0, 0, 240 + i, 40, TRUE);
		newUpdown = FindWindowExW(container, NULL, UPDOWN_CLASSW, NULL);
		ok = ok && newUpdown != NULL && tooltipsFor(newUpdown) == 1 &&
			tooltipsFor(NULL) == count;
		if (oldUpdown != newUpdown)
			ok = ok && tooltipsFor(oldUpdown) == 0;
		oldUpdown = newUpdown;
	}
	uiControlSetTooltip(uiControl(spinbox), NULL);
	uiControlDestroy(uiControl(spinbox));
	return ok;
}

int main(void)
{
	uiInitOptions options = { 0 };
	uiImageView *imageView;
	uiLabel *label;
	const char *error;
	HINSTANCE imageViewInstance;
	WNDCLASSW imageViewClass;
	int ok;

	error = uiInit(&options);
	if (error != NULL) {
		uiFreeInitError(error);
		return 1;
	}
	label = uiNewLabel("status");
	uiControlSetTooltip(uiControl(label), "label tooltip");
	uiControlDestroy(uiControl(label));
	imageView = uiNewImageView();
	imageViewInstance = (HINSTANCE) GetWindowLongPtrW(
		(HWND) uiControlHandle(uiControl(imageView)), GWLP_HINSTANCE);
	uiControlSetTooltip(uiControl(imageView), "image view tooltip");
	uiControlDestroy(uiControl(imageView));
	ok = spinboxTooltipSurvivesRelayout();
	uiUninit();
	if (GetClassInfoW(imageViewInstance, L"libui_uiImageViewClass", &imageViewClass))
		ok = 0;
	// A later initialization must register and release the class again.
	error = uiInit(&options);
	if (error != NULL) {
		uiFreeInitError(error);
		return 1;
	}
	imageView = uiNewImageView();
	uiControlDestroy(uiControl(imageView));
	uiUninit();
	if (GetClassInfoW(imageViewInstance, L"libui_uiImageViewClass", &imageViewClass))
		ok = 0;
	return ok ? 0 : 1;
}
