// 18 december 2015
#include "uipriv_windows.hpp"
#include "area.hpp"

void loadAreaSize(uiArea *a, ID2D1RenderTarget *rt, double *width, double *height)
{
	D2D1_SIZE_F size;
	RECT r;

	*width = 0;
	*height = 0;
	if (!a->scrolling) {
		if (rt != NULL) {
			size = realGetSize(rt);
			*width = size.width;
			*height = size.height;
		} else if (a->rt != NULL) {
			size = realGetSize(a->rt);
			*width = size.width;
			*height = size.height;
		} else {
			uiWindowsEnsureGetClientRect(a->hwnd, &r);
			*width = r.right - r.left;
			*height = r.bottom - r.top;
			pixelsToDIP(a, width, height);
		}
	}
}

static void hwndDPI(HWND hwnd, FLOAT *dpix, FLOAT *dpiy)
{
	HDC dc;

	*dpix = 96;
	*dpiy = 96;
	dc = GetDC(hwnd);
	if (dc == NULL) {
		logLastError(L"error getting DC to find DPI");
		return;
	}
	*dpix = GetDeviceCaps(dc, LOGPIXELSX);
	*dpiy = GetDeviceCaps(dc, LOGPIXELSY);
	if (ReleaseDC(hwnd, dc) == 0)
		logLastError(L"error releasing DC for finding DPI");
}

void pixelsToDIPWithRT(ID2D1RenderTarget *rt, double *x, double *y)
{
	FLOAT dpix, dpiy;

	rt->GetDpi(&dpix, &dpiy);
	// see https://msdn.microsoft.com/en-us/library/windows/desktop/dd756649%28v=vs.85%29.aspx (and others; search "direct2d mouse")
	if (x != NULL)
		*x = (*x * 96) / dpix;
	if (y != NULL)
		*y = (*y * 96) / dpiy;
}

void pixelsToDIP(uiArea *a, double *x, double *y)
{
	FLOAT dpix, dpiy;

	if (a->rt != NULL)
		a->rt->GetDpi(&dpix, &dpiy);
	else
		hwndDPI(a->hwnd, &dpix, &dpiy);
	if (x != NULL)
		*x = (*x * 96) / dpix;
	if (y != NULL)
		*y = (*y * 96) / dpiy;
}

void dipToPixels(uiArea *a, double *x, double *y)
{
	FLOAT dpix, dpiy;

	if (a->rt != NULL)
		a->rt->GetDpi(&dpix, &dpiy);
	else
		hwndDPI(a->hwnd, &dpix, &dpiy);
	if (x != NULL)
		*x = (*x * dpix) / 96;
	if (y != NULL)
		*y = (*y * dpiy) / 96;
}
