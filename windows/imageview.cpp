// uiImageView — Windows implementation (copy-owned, Direct2D)
#include "uipriv_windows.hpp"
#include "draw.hpp"

#define uiImageViewSignature 0x49566965
#ifndef WM_DPICHANGED_AFTERPARENT
#define WM_DPICHANGED_AFTERPARENT 0x02E3
#endif

struct uiImageView {
	uiWindowsControl c;
	HWND hwnd;
	uiImageViewContentMode mode;
	uiImage *image;  // owned copy for drawing (may be NULL)
	ID2D1HwndRenderTarget *rt;
	ID2D1Bitmap *d2dBitmap;
	IWICBitmap *d2dBitmapSource;  // borrowed from image
};

static LRESULT CALLBACK imageViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static ATOM imageViewClass = 0;

static void releaseImageViewBitmap(uiImageView *iv)
{
	if (iv->d2dBitmap != NULL)
		iv->d2dBitmap->Release();
	iv->d2dBitmap = NULL;
	iv->d2dBitmapSource = NULL;
}

static void releaseImageViewDeviceResources(uiImageView *iv)
{
	releaseImageViewBitmap(iv);
	if (iv->rt != NULL)
		iv->rt->Release();
	iv->rt = NULL;
}

static HBRUSH imageViewBackground(uiImageView *iv, HDC hdc,
	D2D1_COLOR_F *d2dColor)
{
	HBRUSH brush;
	LOGBRUSH logicalBrush;
	COLORREF color;

	brush = (HBRUSH) SendMessageW(GetParent(iv->hwnd), WM_CTLCOLORSTATIC,
		(WPARAM) hdc, (LPARAM) iv->hwnd);
	if (brush == NULL) {
		brush = GetSysColorBrush(COLOR_WINDOW);
		color = GetSysColor(COLOR_WINDOW);
	} else if (brush == GetStockObject(DC_BRUSH)) {
		color = GetDCBrushColor(hdc);
	} else if (GetObject(brush, sizeof (LOGBRUSH), &logicalBrush) != 0 &&
		logicalBrush.lbStyle == BS_SOLID) {
		color = logicalBrush.lbColor;
	} else {
		// libui containers only support solid backgrounds. Keep a sensible
		// fallback for custom parents that return another brush style.
		color = GetBkColor(hdc);
	}
	if (color == CLR_INVALID)
		color = GetSysColor(COLOR_WINDOW);
	d2dColor->r = ((FLOAT) GetRValue(color)) / 255.0f;
	d2dColor->g = ((FLOAT) GetGValue(color)) / 255.0f;
	d2dColor->b = ((FLOAT) GetBValue(color)) / 255.0f;
	d2dColor->a = 1.0f;
	return brush;
}

static void initImageViewClass(void)
{
	WNDCLASSW wc;

	if (imageViewClass != 0)
		return;
	ZeroMemory(&wc, sizeof (WNDCLASSW));
	wc.lpszClassName = L"libui_uiImageViewClass";
	wc.lpfnWndProc = imageViewWndProc;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;  // we handle WM_ERASEBKGND
	wc.lpszMenuName = NULL;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = sizeof (uiImageView *);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	imageViewClass = RegisterClassW(&wc);
	if (imageViewClass == 0)
		logLastError(L"error registering uiImageView window class");
}

static HRESULT drawImageView(uiImageView *iv, ID2D1RenderTarget *rt,
	D2D1_COLOR_F *backgroundColor, ID2D1Bitmap **cachedBitmap,
	IWICBitmap **cachedSource)
{
	RECT clientRect;
	IWICBitmap *bitmap;
	ID2D1Bitmap *d2dBitmap;
	float dpiX, dpiY;
	double viewW, viewH, imgW, imgH;
	double dx, dy, dw, dh;
	int clientW, clientH;
	int targetW, targetH;
	HRESULT bitmapHR, hr;

	GetClientRect(iv->hwnd, &clientRect);
	clientW = clientRect.right - clientRect.left;
	clientH = clientRect.bottom - clientRect.top;
	rt->GetDpi(&dpiX, &dpiY);
	bitmap = NULL;
	if (iv->image != NULL && clientW > 0 && clientH > 0 &&
		uiprivImagePositiveFinite(dpiX) &&
		uiprivImagePositiveFinite(dpiY)) {
		viewW = ((double) clientW) * 96.0 / dpiX;
		viewH = ((double) clientH) * 96.0 / dpiY;
		uiprivImageSize(iv->image, &imgW, &imgH);
		uiprivImageViewComputeRect(viewW, viewH, imgW, imgH, iv->mode,
			&dx, &dy, &dw, &dh);
		targetW = uiprivImageTargetPixelSize(dw * dpiX / 96.0);
		targetH = uiprivImageTargetPixelSize(dh * dpiY / 96.0);
		if (targetW != 0 && targetH != 0)
			bitmap = uiprivImageAppropriateForSize(iv->image,
				targetW, targetH);
	}
	d2dBitmap = NULL;
	bitmapHR = S_OK;
	if (bitmap != NULL && cachedBitmap != NULL && cachedSource != NULL) {
		if (*cachedSource != bitmap) {
			if (*cachedBitmap != NULL)
				(*cachedBitmap)->Release();
			*cachedBitmap = NULL;
			*cachedSource = NULL;
		}
		if (*cachedBitmap == NULL) {
			bitmapHR = rt->CreateBitmapFromWicBitmap(bitmap, NULL,
				cachedBitmap);
			if (bitmapHR == S_OK)
				*cachedSource = bitmap;
		}
		d2dBitmap = *cachedBitmap;
	} else if (bitmap != NULL) {
		bitmapHR = rt->CreateBitmapFromWicBitmap(bitmap, NULL, &d2dBitmap);
	}
	if (bitmap == NULL && cachedBitmap != NULL && cachedSource != NULL) {
		if (*cachedBitmap != NULL)
			(*cachedBitmap)->Release();
		*cachedBitmap = NULL;
		*cachedSource = NULL;
	}

	rt->BeginDraw();
	rt->Clear(backgroundColor);
	if (d2dBitmap != NULL) {
		D2D1_RECT_F destRect;

		destRect = D2D1::RectF((FLOAT) dx, (FLOAT) dy,
			(FLOAT) (dx + dw), (FLOAT) (dy + dh));
		rt->DrawBitmap(d2dBitmap, destRect, 1.0f,
			D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
	}
	hr = rt->EndDraw();
	if (cachedBitmap == NULL && d2dBitmap != NULL)
		d2dBitmap->Release();
	if (hr == S_OK && bitmapHR != S_OK)
		return bitmapHR;
	return hr;
}

static void paintImageView(uiImageView *iv, HDC hdc)
{
	RECT clientRect;
	D2D1_COLOR_F backgroundColor;
	HBRUSH backgroundBrush;
	float dpiX, dpiY;
	float rtDPIX, rtDPIY;
	HRESULT hr;

	GetClientRect(iv->hwnd, &clientRect);
	if (clientRect.right <= clientRect.left ||
		clientRect.bottom <= clientRect.top)
		return;
	backgroundBrush = imageViewBackground(iv, hdc, &backgroundColor);
	if (iv->rt == NULL)
		iv->rt = makeHWNDRenderTarget(iv->hwnd);
	if (iv->rt == NULL) {
		FillRect(hdc, &clientRect, backgroundBrush);
		return;
	}
	dpiX = (float) GetDeviceCaps(hdc, LOGPIXELSX);
	dpiY = (float) GetDeviceCaps(hdc, LOGPIXELSY);
	iv->rt->GetDpi(&rtDPIX, &rtDPIY);
	if (uiprivImagePositiveFinite(dpiX) &&
		uiprivImagePositiveFinite(dpiY) &&
		(dpiX != rtDPIX || dpiY != rtDPIY)) {
		iv->rt->SetDpi(dpiX, dpiY);
		releaseImageViewBitmap(iv);
	}

	hr = drawImageView(iv, iv->rt, &backgroundColor,
		&iv->d2dBitmap, &iv->d2dBitmapSource);
	if (hr == S_OK)
		return;
	if (hr != (HRESULT) D2DERR_RECREATE_TARGET)
		logHRESULT(L"error drawing uiImageView", hr);
	releaseImageViewDeviceResources(iv);
	FillRect(hdc, &clientRect, backgroundBrush);
	if (hr == (HRESULT) D2DERR_RECREATE_TARGET)
		InvalidateRect(iv->hwnd, NULL, FALSE);
}

static void printImageView(uiImageView *iv, HDC hdc)
{
	RECT clientRect;
	D2D1_COLOR_F backgroundColor;
	HBRUSH backgroundBrush;
	ID2D1DCRenderTarget *rt;
	HRESULT hr;

	GetClientRect(iv->hwnd, &clientRect);
	backgroundBrush = imageViewBackground(iv, hdc, &backgroundColor);
	rt = makeHDCRenderTarget(hdc, &clientRect);
	if (rt == NULL) {
		FillRect(hdc, &clientRect, backgroundBrush);
		return;
	}
	hr = drawImageView(iv, rt, &backgroundColor, NULL, NULL);
	if (hr != S_OK) {
		logHRESULT(L"error printing uiImageView", hr);
		FillRect(hdc, &clientRect, backgroundBrush);
	}
	rt->Release();
}

static LRESULT CALLBACK imageViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	uiImageView *iv;
	CREATESTRUCTW *cs = (CREATESTRUCTW *) lParam;
	PAINTSTRUCT ps;
	HDC hdc;

	iv = (uiImageView *) GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	if (iv == NULL) {
		if (uMsg == WM_CREATE) {
			iv = (uiImageView *) (cs->lpCreateParams);
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR) iv);
			return 0; // WM_CREATE success
		}
		return DefWindowProcW(hwnd, uMsg, wParam, lParam);
	}

	switch (uMsg) {
	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		paintImageView(iv, hdc);
		EndPaint(hwnd, &ps);
		return 0;
	case WM_SIZE:
		if (iv->rt != NULL) {
			RECT clientRect;
			D2D1_SIZE_U size;

			GetClientRect(hwnd, &clientRect);
			size.width = clientRect.right - clientRect.left;
			size.height = clientRect.bottom - clientRect.top;
			if (iv->rt->Resize(&size) != S_OK)
				releaseImageViewDeviceResources(iv);
		}
		InvalidateRect(hwnd, NULL, FALSE);
		break;
	case WM_DPICHANGED:
	case WM_DPICHANGED_AFTERPARENT:
	case WM_THEMECHANGED:
	case WM_SYSCOLORCHANGE:
	case WM_SETTINGCHANGE:
		InvalidateRect(hwnd, NULL, FALSE);
		break;
	case WM_DISPLAYCHANGE:
		releaseImageViewDeviceResources(iv);
		InvalidateRect(hwnd, NULL, FALSE);
		break;
	case WM_ERASEBKGND:
		// we handle background drawing in WM_PAINT
		return 1;
	case WM_PRINTCLIENT:
		printImageView(iv, (HDC) wParam);
		return 0;
	}
	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

uiWindowsControlAllDefaultsExceptDestroy(uiImageView)

static void uiImageViewDestroy(uiControl *c)
{
	uiImageView *iv = uiImageView(c);

	releaseImageViewDeviceResources(iv);
	if (iv->image != NULL) {
		uiFreeImage(iv->image);
		iv->image = NULL;
	}
	uiWindowsEnsureDestroyWindow(iv->hwnd);
	uiFreeControl(uiControl(iv));
}

static void uiImageViewMinimumSize(uiWindowsControl *c, int *width, int *height)
{
	// Set minimum size to 16x16 to support icon usage
	*width = 16;
	*height = 16;
}

uiImageView *uiNewImageView(void)
{
	uiImageView *iv;

	initImageViewClass();

	uiWindowsNewControl(uiImageView, iv);
	iv->mode = uiImageViewContentFit;  // default mode
	iv->image = NULL;
	iv->rt = NULL;
	iv->d2dBitmap = NULL;
	iv->d2dBitmapSource = NULL;

	iv->hwnd = uiWindowsEnsureCreateControlHWND(WS_EX_CONTROLPARENT,
		L"libui_uiImageViewClass", L"",
		0,
		hInstance, iv,
		FALSE);

	return iv;
}

void uiImageViewSetContentMode(uiImageView *iv, uiImageViewContentMode mode)
{
	iv->mode = mode;
	InvalidateRect(iv->hwnd, NULL, FALSE);
}

void uiImageViewSetImage(uiImageView *iv, const uiImage *image)
{
	releaseImageViewBitmap(iv);
	// Release old image if exists
	if (iv->image != NULL) {
		uiFreeImage(iv->image);
		iv->image = NULL;
	}

	if (image == NULL) {
		// No image, just invalidate to clear
		InvalidateRect(iv->hwnd, NULL, FALSE);
		return;
	}

	iv->image = uiprivImageCopy((uiImage *) image);

	InvalidateRect(iv->hwnd, NULL, FALSE);
}
