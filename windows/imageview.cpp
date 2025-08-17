// uiImageView — Windows (D2D1) implementation (MVP, copy-owned)
#define NOMINMAX
#include "uipriv_windows.hpp"
#include "ui.h"
#include <algorithm>

// Backward compatibility for constants defined in Windows 8 and later
#ifndef D2D1_BITMAP_INTERPOLATION_MODE_CUBIC
#define D2D1_BITMAP_INTERPOLATION_MODE_CUBIC ((D2D1_BITMAP_INTERPOLATION_MODE)3)
#endif

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

#define uiImageViewSignature 0x49566965


struct uiImageView {
	uiWindowsControl c;
	HWND hwnd;
	uiImageViewContentMode mode;

	// D2D resources
	ID2D1HwndRenderTarget *rt;
	ID2D1Bitmap *bmp;

	// Cached D2D 1.1 capability for interpolation mode selection
	bool allowCubic;

	// Current image for DPI change regeneration
	const uiImage *currentImage;
};

uiWindowsControlAllDefaultsExceptDestroy(uiImageView)

static void uiImageViewDestroy(uiControl *c)
{
	uiImageView *v = uiImageView(c);
	if (v->bmp) { v->bmp->Release(); v->bmp = NULL; }
	if (v->rt)  { v->rt->Release();  v->rt  = NULL; }
	uiFreeControl(uiControl(v));
}

static void uiImageViewMinimumSize(uiWindowsControl *c, int *width, int *height)
{
	*width = 16;
	*height = 16;
}

static void compute_target_rect(float vw, float vh, float iw, float ih, uiImageViewContentMode mode, D2D1_RECT_F *dst)
{
	if (iw <= 0 || ih <= 0 || vw <= 0 || vh <= 0) { *dst = { 0, 0, 0, 0 }; return; }
	float sx = vw / iw, sy = vh / ih;
	float s;
	switch (mode) {
	case uiImageViewContentCenter: s = 1.0f; break;
	case uiImageViewContentFit:    s = (std::min)(sx, sy); break;
	case uiImageViewContentFill:   s = (std::max)(sx, sy); break;
	default: s = (std::min)(sx, sy); break;
	}
	float dw = iw * s, dh = ih * s;
	float dx = (vw - dw) * 0.5f, dy = (vh - dh) * 0.5f;
	*dst = { dx, dy, dx + dw, dy + dh };
}

static LRESULT CALLBACK uiImageViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	uiImageView *v = (uiImageView *) GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	if (v == NULL) {
		if (uMsg == WM_CREATE) {
			CREATESTRUCTW *cs = (CREATESTRUCTW *) lParam;
			v = (uiImageView *) (cs->lpCreateParams);
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR) v);
		}
		return DefWindowProcW(hwnd, uMsg, wParam, lParam);
	}
	
	switch (uMsg) {
	case WM_PAINT: {
		PAINTSTRUCT ps;
		BeginPaint(v->hwnd, &ps);

		if (v->rt == NULL) {
			v->rt = makeHWNDRenderTarget(v->hwnd);
			v->allowCubic = uiprivD2DSupportsFactory1(v->rt);
		}

		if (v->rt) {
			v->rt->BeginDraw();
			v->rt->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

			// Regenerate bitmap if needed after DPI change
			if (v->bmp == NULL && v->currentImage != NULL) {
				v->bmp = uiprivImageToD2DBitmap((uiImage *)v->currentImage, v->rt);
			}

			if (v->bmp) {
				D2D1_SIZE_F rtSize = realGetSize(v->rt);
				D2D1_SIZE_F sz = v->bmp->GetSize();
				D2D1_RECT_F dst;
				compute_target_rect(rtSize.width, rtSize.height, sz.width, sz.height, v->mode, &dst);

				// Use cubic interpolation if D2D 1.1 is available, otherwise linear
				D2D1_BITMAP_INTERPOLATION_MODE interp =
					v->allowCubic ? D2D1_BITMAP_INTERPOLATION_MODE_CUBIC
					              : D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;

				v->rt->DrawBitmap(v->bmp, dst, 1.0f, interp);
			}

			HRESULT hr = v->rt->EndDraw();
			if (hr == D2DERR_RECREATE_TARGET) {
				if (v->bmp) { v->bmp->Release(); v->bmp = NULL; }
				v->rt->Release(); v->rt = NULL;
			}
		}

		EndPaint(v->hwnd, &ps);
		return 0;
	}
	case WM_SIZE:
		if (v->rt) {
			RECT rc; uiWindowsEnsureGetClientRect(v->hwnd, &rc);
			v->rt->Resize(D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top));
		}
		break;
	case WM_DPICHANGED:
		if (v->bmp) { v->bmp->Release(); v->bmp = NULL; }
		InvalidateRect(v->hwnd, NULL, TRUE);
		break;
	}
	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

uiImageView *uiNewImageView(void)
{
	uiImageView *v;
	
	uiWindowsNewControl(uiImageView, v);
	
	v->mode = uiImageViewContentFit;
	v->rt = NULL;
	v->bmp = NULL;
	v->allowCubic = false;
	v->currentImage = NULL;

	DWORD exstyle = 0;
	DWORD style = WS_CHILD | WS_VISIBLE;
	v->hwnd = uiWindowsEnsureCreateControlHWND(exstyle, L"STATIC", L"", style, GetModuleHandle(NULL), v, TRUE);

	SetWindowLongPtrW(v->hwnd, GWLP_WNDPROC, (LONG_PTR) uiImageViewWndProc);
	
	return v;
}

void uiImageViewSetContentMode(uiImageView *v, uiImageViewContentMode mode)
{
	v->mode = mode;
	InvalidateRect(v->hwnd, NULL, TRUE);
}


void uiImageViewSetImage(uiImageView *v, const uiImage *image)
{
	if (v->bmp) { v->bmp->Release(); v->bmp = NULL; }

	// Store current image for DPI change regeneration
	v->currentImage = image;

	if (image == NULL) {
		InvalidateRect(v->hwnd, NULL, TRUE);
		return;
	}

	if (v->rt == NULL) {
		v->rt = makeHWNDRenderTarget(v->hwnd);
		v->allowCubic = uiprivD2DSupportsFactory1(v->rt);
	}

	if (v->rt != NULL) {
		v->bmp = uiprivImageToD2DBitmap((uiImage *)image, v->rt);
	}
	InvalidateRect(v->hwnd, NULL, TRUE);
}
