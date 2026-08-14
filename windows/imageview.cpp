// uiImageView — Windows implementation (MVP, copy-owned, Direct2D)
#include "uipriv_windows.hpp"
#include "draw.hpp"

#define uiImageViewSignature 0x49566965

struct uiImageView {
	uiWindowsControl c;
	HWND hwnd;
	uiImageViewContentMode mode;
	uiImage *image;  // owned copy for drawing (may be NULL)
};

static LRESULT CALLBACK imageViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static ATOM imageViewClass = 0;

static HBRUSH imageViewBackgroundBrush(uiImageView *iv, HDC hdc)
{
	HBRUSH brush;

	brush = (HBRUSH) SendMessageW(GetParent(iv->hwnd), WM_CTLCOLORSTATIC,
		(WPARAM) hdc, (LPARAM) iv->hwnd);
	if (brush == NULL)
		return GetSysColorBrush(COLOR_WINDOW);
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

static void paintImageView(uiImageView *iv, HDC hdc)
{
	RECT clientRect;
	D2D1_RENDER_TARGET_PROPERTIES props;
	ID2D1RenderTarget *rt;
	IWICBitmap *bitmap;
	IWICBitmap *target;
	ID2D1Bitmap *d2dBitmap;
	HBRUSH backgroundBrush;
	HBITMAP targetBitmap;
	HDC sourceDC;
	HGDIOBJ previousBitmap;
	BLENDFUNCTION blend;
	float dpiX, dpiY;
	double viewW, viewH, imgW, imgH;
	double dx, dy, dw, dh;
	int clientW, clientH;
	int targetW, targetH;
	HRESULT drawHR, hr;

	GetClientRect(iv->hwnd, &clientRect);
	backgroundBrush = imageViewBackgroundBrush(iv, hdc);
	FillRect(hdc, &clientRect, backgroundBrush);
	if (iv->image == NULL)
		return;
	clientW = clientRect.right - clientRect.left;
	clientH = clientRect.bottom - clientRect.top;
	if (clientW <= 0 || clientH <= 0)
		return;

	dpiX = (float) GetDeviceCaps(hdc, LOGPIXELSX);
	dpiY = (float) GetDeviceCaps(hdc, LOGPIXELSY);
	bitmap = NULL;
	if (uiprivImagePositiveFinite(dpiX) &&
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
	if (bitmap == NULL)
		return;

	target = NULL;
	hr = uiprivWICFactory->CreateBitmap(clientW, clientH,
		GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnLoad, &target);
	if (hr != S_OK) {
		logHRESULT(L"error creating WIC target bitmap for uiImageView", hr);
		return;
	}
	ZeroMemory(&props, sizeof (D2D1_RENDER_TARGET_PROPERTIES));
	props.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
	props.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	props.dpiX = dpiX;
	props.dpiY = dpiY;
	props.usage = D2D1_RENDER_TARGET_USAGE_NONE;
	props.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
	rt = NULL;
	hr = d2dfactory->CreateWicBitmapRenderTarget(target, &props, &rt);
	if (hr != S_OK) {
		logHRESULT(L"error creating WIC render target for uiImageView", hr);
		target->Release();
		return;
	}

	rt->BeginDraw();
	rt->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
	d2dBitmap = NULL;
	drawHR = rt->CreateBitmapFromWicBitmap(bitmap, NULL, &d2dBitmap);
	if (drawHR == S_OK) {
		D2D1_RECT_F destRect;

		destRect = D2D1::RectF((FLOAT) dx, (FLOAT) dy,
			(FLOAT) (dx + dw), (FLOAT) (dy + dh));
		rt->DrawBitmap(d2dBitmap, destRect, 1.0f,
			D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
		d2dBitmap->Release();
	} else
		logHRESULT(L"error creating Direct2D bitmap for uiImageView", drawHR);
	hr = rt->EndDraw();
	rt->Release();
	if (hr != S_OK) {
		logHRESULT(L"error ending uiImageView draw", hr);
		target->Release();
		return;
	}
	if (drawHR != S_OK) {
		target->Release();
		return;
	}

	targetBitmap = NULL;
	hr = uiprivWICToGDI(target, hdc, 0, 0, &targetBitmap);
	target->Release();
	if (hr != S_OK || targetBitmap == NULL) {
		if (hr != S_OK)
			logHRESULT(L"error converting uiImageView target to GDI", hr);
		return;
	}
	sourceDC = CreateCompatibleDC(hdc);
	if (sourceDC == NULL) {
		logLastError(L"error creating source DC for uiImageView");
		DeleteObject(targetBitmap);
		return;
	}
	previousBitmap = SelectObject(sourceDC, targetBitmap);
	if (previousBitmap == NULL || previousBitmap == HGDI_ERROR) {
		logLastError(L"error selecting uiImageView target bitmap");
		DeleteDC(sourceDC);
		DeleteObject(targetBitmap);
		return;
	}
	blend.BlendOp = AC_SRC_OVER;
	blend.BlendFlags = 0;
	blend.SourceConstantAlpha = 255;
	blend.AlphaFormat = AC_SRC_ALPHA;
	if (AlphaBlend(hdc, 0, 0, clientW, clientH,
		sourceDC, 0, 0, clientW, clientH, blend) == FALSE)
		logLastError(L"error alpha blending uiImageView");
	SelectObject(sourceDC, previousBitmap);
	DeleteDC(sourceDC);
	DeleteObject(targetBitmap);
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
	case WM_ERASEBKGND:
		// we handle background drawing in WM_PAINT
		return 1;
	case WM_PRINTCLIENT:
		paintImageView(iv, (HDC) wParam);
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
