// uiImageView — Windows implementation (MVP, copy-owned, Direct2D)
#include "uipriv_windows.hpp"

#define uiImageViewSignature 0x49566965

struct uiImageView {
	uiWindowsControl c;
	HWND hwnd;
	uiImageViewContentMode mode;
	uiImage *image;  // owned copy for drawing (may be NULL)
};

static LRESULT CALLBACK imageViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static ATOM imageViewClass = 0;

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

static void paintImageView(uiImageView *iv, HDC hdc, RECT *rcPaint)
{
	RECT clientRect;
	ID2D1DCRenderTarget *rt;
	IWICBitmap *bitmap;
	ID2D1Bitmap *d2dBitmap;
	float dpiX, dpiY;
	double viewW, viewH, imgW, imgH;
	double dx, dy, dw, dh;
	HRESULT hr;

	GetClientRect(iv->hwnd, &clientRect);

	// Always fill background with proper theme color
	HBRUSH bk = (HBRUSH)SendMessageW(GetParent(iv->hwnd), WM_CTLCOLORSTATIC,
	                                 (WPARAM)hdc, (LPARAM)iv->hwnd);
	if (!bk) bk = (HBRUSH)(COLOR_WINDOW + 1);
	FillRect(hdc, &clientRect, bk);
	
	if (iv->image == NULL) {
		// No image to draw, background already filled
		return;
	}

	// Create Direct2D render target
	rt = makeHDCRenderTarget(hdc, &clientRect);
	if (rt == NULL)
		return;

	rt->BeginDraw();

	// Clear background
	rt->Clear(D2D1::ColorF(D2D1::ColorF::White, 0.0f)); // Transparent background

	rt->GetDpi(&dpiX, &dpiY);
	bitmap = uiprivImageAppropriateForDPI(iv->image, dpiX, dpiY);
	if (bitmap == NULL) {
		rt->EndDraw();
		rt->Release();
		return;
	}

	// Layout uses uiImage's logical size; bitmap pixels only provide samples.
	viewW = ((double) (clientRect.right - clientRect.left)) * 96.0 / dpiX;
	viewH = ((double) (clientRect.bottom - clientRect.top)) * 96.0 / dpiY;
	uiprivImageSize(iv->image, &imgW, &imgH);
	uiprivImageViewComputeRect(viewW, viewH, imgW, imgH, iv->mode,
		&dx, &dy, &dw, &dh);

	// Create D2D bitmap from WIC bitmap
	hr = rt->CreateBitmapFromWicBitmap(bitmap, NULL, &d2dBitmap);
	if (hr != S_OK) {
		rt->EndDraw();
		rt->Release();
		return;
	}

	// Draw the bitmap with high-quality linear interpolation
	D2D1_RECT_F destRect = D2D1::RectF((FLOAT) dx, (FLOAT) dy,
		(FLOAT) (dx + dw), (FLOAT) (dy + dh));
	rt->DrawBitmap(d2dBitmap, destRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);

	d2dBitmap->Release();
	rt->EndDraw();
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
		paintImageView(iv, hdc, &ps.rcPaint);
		EndPaint(hwnd, &ps);
		return 0;
	case WM_ERASEBKGND:
		// we handle background drawing in WM_PAINT
		return 1;
	case WM_PRINTCLIENT:
		paintImageView(iv, (HDC) wParam, NULL);
		return 0;
	}
	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

uiWindowsControlAllDefaultsExceptDestroy(uiImageView)

static void uiImageViewDestroy(uiControl *c)
{
	uiImageView *iv = uiImageView(c);

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

	iv->hwnd = uiWindowsEnsureCreateControlHWND(WS_EX_CONTROLPARENT,
		L"libui_uiImageViewClass", L"",
		0,
		hInstance, iv,
		FALSE);

	iv->mode = uiImageViewContentFit;  // default mode
	iv->image = NULL;

	return iv;
}

void uiImageViewSetContentMode(uiImageView *iv, uiImageViewContentMode mode)
{
	iv->mode = mode;
	InvalidateRect(iv->hwnd, NULL, TRUE);
}

void uiImageViewSetImage(uiImageView *iv, const uiImage *image)
{
	// Release old image if exists
	if (iv->image != NULL) {
		uiFreeImage(iv->image);
		iv->image = NULL;
	}

	if (image == NULL) {
		// No image, just invalidate to clear
		InvalidateRect(iv->hwnd, NULL, TRUE);
		return;
	}

	iv->image = uiprivImageCopy((uiImage *) image);

	InvalidateRect(iv->hwnd, NULL, TRUE);
}
