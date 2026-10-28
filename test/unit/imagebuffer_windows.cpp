// Compare oversized-buffer crops with identical small images using real D2D/WIC pixels.
#include "../../windows/uipriv_windows.hpp"
#include "../../windows/draw.hpp"
extern "C" {
#include "unit.h"
}
#include <limits.h>

struct Factories {
	ID2D1Factory *d2d = NULL;
	IWICImagingFactory *wic = NULL;
	Factories() {
		assert_true(SUCCEEDED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
			IID_ID2D1Factory, NULL, (void **) &d2d)));
		assert_true(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, NULL,
			CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (void **) &wic)));
	}
	~Factories() { wic->Release(); d2d->Release(); }
};

class Canvas {
	IWICBitmap *bitmap = NULL;
	ID2D1RenderTarget *target = NULL;
public:
	uiDrawContext context = {};
	unsigned char pixels[16 * 8 * 4] = {};

	Canvas(ID2D1Factory *factory, IWICImagingFactory *wic, float dpi) {
		assert_true(SUCCEEDED(wic->CreateBitmap(16, 8, GUID_WICPixelFormat32bppPBGRA,
			WICBitmapCacheOnLoad, &bitmap)));
		D2D1_RENDER_TARGET_PROPERTIES p = {};
		p.type = D2D1_RENDER_TARGET_TYPE_SOFTWARE;
		p.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
		p.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
		p.dpiX = p.dpiY = dpi;
		assert_true(SUCCEEDED(factory->CreateWicBitmapRenderTarget(bitmap, p, &target)));
		context.rt = target;
		target->BeginDraw();
		D2D1_COLOR_F clear = {0, 0, 0, 0};
		target->Clear(clear);
	}
	unsigned maximum() const { return target->GetMaximumBitmapSize(); }
	void begin() { target->BeginDraw(); }
	uint32_t pixel(int x, int y) const {
		uint32_t value;
		memcpy(&value, pixels + (y * 16 + x) * 4, 4);
		return value;
	}
	void finish() {
		assert_true(SUCCEEDED(target->EndDraw()));
		assert_true(SUCCEEDED(bitmap->CopyPixels(NULL, 16 * 4, sizeof pixels, pixels)));
	}
	~Canvas() { target->Release(); bitmap->Release(); }
};

static void oversizedCrops(int vertical, uiImageBufferInterpolation interpolation, float dpi)
{
	Factories f;
	{
		Canvas actual(f.d2d, f.wic, dpi), expected(f.d2d, f.wic, dpi);
		assert_true(actual.maximum() < INT_MAX - 3);
		int length = (int) actual.maximum() + 3;
		int patchWidth = vertical ? 2 : 3, patchHeight = vertical ? 3 : 2;
		const unsigned char patch[] = {
			255, 0, 0, 255, 0, 255, 0, 128, 0, 0, 255, 255,
			255, 255, 0, 255, 255, 0, 255, 128, 0, 255, 255, 255,
		};
		uiImageBuffer *largeBuffer = uiNewImageBuffer(vertical ? 2 : length,
			vertical ? length : 2, uiImageBufferRGBA8Straight);
		uiImageBuffer *smallBuffer = uiNewImageBuffer(patchWidth, patchHeight, uiImageBufferRGBA8Straight);
		assert_non_null(largeBuffer);
		assert_non_null(smallBuffer);
		assert_true(uiImageBufferUpdate(smallBuffer, patch, sizeof patch, patchWidth * 4));
		// Exercise both image boundaries and a crop with neighbors on both sides.
		for (int edge = 0; edge < 3; edge++) {
			int origin = edge == 0 ? 0 : length - 3;
			int selected = edge == 0 ? 0 : edge == 1 ? 1 : 2;
			assert_true(uiImageBufferUpdateRect(largeBuffer, vertical ? 0 : origin,
				vertical ? origin : 0, patchWidth, patchHeight, patch, sizeof patch, patchWidth * 4));
			uiDrawImageBufferParams p = {sizeof p,
				vertical ? 0 : selected, vertical ? selected : 0,
				vertical ? 2 : 1, vertical ? 1 : 2,
				edge * 3.0, 0, 2, 4, interpolation};
			uiDrawImageBufferWithParams(&expected.context, smallBuffer, &p);
			p.SourceX += vertical ? 0 : origin;
			p.SourceY += vertical ? origin : 0;
			uiDrawImageBufferWithParams(&actual.context, largeBuffer, &p);
		}
		// Update an already-cached crop whose backing origin is nonzero. The
		// dirty upload must include that origin, including for Smooth's halo.
		const unsigned char blue[] = {0, 0, 255, 255};
		assert_true(uiImageBufferUpdateRect(largeBuffer, vertical ? 0 : length - 2,
			vertical ? length - 2 : 0, 1, 1, blue, sizeof blue, 4));
		assert_true(uiImageBufferUpdateRect(smallBuffer, vertical ? 0 : 1,
			vertical ? 1 : 0, 1, 1, blue, sizeof blue, 4));
		uiDrawImageBufferParams updated = {sizeof updated, vertical ? 0 : 1, vertical ? 1 : 0,
			vertical ? 2 : 1, vertical ? 1 : 2, 9, 0, 2, 4, interpolation};
		uiDrawImageBufferWithParams(&expected.context, smallBuffer, &updated);
		updated.SourceX += vertical ? 0 : length - 3;
		updated.SourceY += vertical ? length - 3 : 0;
		uiDrawImageBufferWithParams(&actual.context, largeBuffer, &updated);
		// Submitted crops must own their pixels even after updates and Free.
		assert_true(uiImageBufferUpdateRect(largeBuffer, 0, 0, 1, 1, blue, sizeof blue, 4));
		uiFreeImageBuffer(largeBuffer);
		uiFreeImageBuffer(smallBuffer);
		actual.finish(); expected.finish();
		assert_true(expected.pixels[3] != 0);
		for (size_t i = 0; i < sizeof actual.pixels; i++) {
			int difference = (int) actual.pixels[i] - expected.pixels[i];
			assert_true(difference >= -1 && difference <= 1);
		}
	}
}

static void drawPixel(Canvas &canvas, uiImageBuffer *buffer, int x)
{
	uiDrawImageBuffer(&canvas.context, buffer, x, 0, 1, 1, uiImageBufferInterpolationNearest);
}

static void submittedPixelsAcrossTargets(void **state)
{
	Factories f;
	Canvas a(f.d2d, f.wic, 96), b(f.d2d, f.wic, 96);
	uiImageBuffer *buffer = uiNewImageBuffer(1, 1, uiImageBufferRGBA8Straight);
	const unsigned char red[] = {255, 0, 0, 255}, blue[] = {0, 0, 255, 255};
	assert_non_null(buffer);
	assert_true(uiImageBufferUpdate(buffer, red, 4, 4));
	drawPixel(a, buffer, 0); drawPixel(b, buffer, 0);
	assert_true(uiImageBufferUpdate(buffer, blue, 4, 4));
	drawPixel(a, buffer, 1); drawPixel(b, buffer, 1);
	assert_true(uiImageBufferUpdate(buffer, red, 4, 4));
	drawPixel(a, buffer, 2); // B must keep its dirty update until its next draw.
	a.finish(); b.finish();
	assert_int_equal(a.pixel(0, 0), UINT32_C(0xFFFF0000));
	assert_int_equal(a.pixel(1, 0), UINT32_C(0xFF0000FF));
	assert_int_equal(a.pixel(2, 0), UINT32_C(0xFFFF0000));
	assert_int_equal(b.pixel(0, 0), UINT32_C(0xFFFF0000));
	assert_int_equal(b.pixel(1, 0), UINT32_C(0xFF0000FF));
	b.begin(); drawPixel(b, buffer, 2);
	uiFreeImageBuffer(buffer); // A submitted bitmap outlives the CPU buffer.
	b.finish();
	assert_int_equal(b.pixel(2, 0), UINT32_C(0xFFFF0000));
}

static void partialUpdatesAcrossFrames(void **state)
{
	Factories f;
	Canvas c(f.d2d, f.wic, 96);
	uiImageBuffer *buffer = uiNewImageBuffer(4, 3, uiImageBufferRGBA8Straight);
	assert_non_null(buffer);
	uiDrawImageBuffer(&c.context, buffer, 0, 0, 4, 3, uiImageBufferInterpolationNearest);
	c.finish();
	const unsigned char patch[] = {255, 0, 0, 255, 9, 9, 9, 9, 0, 0, 255, 255};
	const unsigned char green[] = {0, 255, 0, 255};
	assert_true(uiImageBufferUpdateRect(buffer, 2, 1, 1, 2, patch, sizeof patch, 8));
	assert_true(uiImageBufferUpdateRect(buffer, 0, 0, 1, 1, green, 4, 4));
	assert_false(uiImageBufferUpdateRect(buffer, 3, 2, 2, 1, green, 4, 4));
	c.begin();
	uiDrawImageBuffer(&c.context, buffer, 0, 0, 4, 3, uiImageBufferInterpolationNearest);
	c.finish();
	assert_int_equal(c.pixel(0, 0), UINT32_C(0xFF00FF00));
	assert_int_equal(c.pixel(2, 1), UINT32_C(0xFFFF0000));
	assert_int_equal(c.pixel(2, 2), UINT32_C(0xFF0000FF));
	assert_int_equal(c.pixel(1, 1), 0);
	assert_int_equal(c.pixel(3, 2), 0);
	uiFreeImageBuffer(buffer);
}

struct AreaState {
	uiImageBuffer *buffer;
	ID2D1RenderTarget *target;
};
static void areaDraw(uiAreaHandler *h, uiArea *, uiAreaDrawParams *p)
{
	AreaState *s = (AreaState *) h->UserData;
	if (s->target == NULL) { s->target = p->Context->rt; s->target->AddRef(); }
	uiDrawImageBuffer(p->Context, s->buffer, 0, 0, 1, 1, uiImageBufferInterpolationNearest);
}
static void areaMouse(uiAreaHandler *, uiArea *, uiAreaMouseEvent *) {}
static void areaCrossed(uiAreaHandler *, uiArea *, int) {}
static void areaBroken(uiAreaHandler *, uiArea *) {}
static int areaKey(uiAreaHandler *, uiArea *, uiAreaKeyEvent *) { return 0; }

static void nativeTargetsRetireTheirCaches(void **state)
{
	for (int printing = 0; printing < 2; printing++) {
		AreaState s = {uiNewImageBuffer(1, 1, uiImageBufferRGBA8Straight), NULL};
		assert_non_null(s.buffer);
		uiAreaHandler h = {areaDraw, areaMouse, areaCrossed, areaBroken, areaKey, NULL, &s};
		uiWindow *window = uiNewWindow("ImageBuffer target retirement", 64, 32, 0);
		uiArea *area = uiNewArea(&h);
		uiWindowSetChild(window, uiControl(area));
		HWND hwnd = (HWND) uiControlHandle(uiControl(area));
		if (printing) {
			HDC dc = CreateCompatibleDC(NULL);
			assert_non_null(dc);
			BITMAPINFO info = {};
			info.bmiHeader.biSize = sizeof info.bmiHeader;
			info.bmiHeader.biWidth = 256; info.bmiHeader.biHeight = -256;
			info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32;
			void *pixels = NULL;
			HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &pixels, NULL, 0);
			assert_non_null(bitmap);
			HGDIOBJ previous = SelectObject(dc, bitmap);
			SendMessageW(hwnd, WM_PRINTCLIENT, (WPARAM) dc, PRF_CLIENT);
			SelectObject(dc, previous); DeleteObject(bitmap); DeleteDC(dc);
		} else {
			InvalidateRect(hwnd, NULL, FALSE);
			SendMessageW(hwnd, WM_PAINT, 0, 0);
		}
		assert_non_null(s.target);
		uiControlDestroy(uiControl(window));
		// The test's AddRef must be the only reference left, even though the
		// ImageBuffer is still alive. This also covers temporary print targets.
		ULONG remaining = s.target->Release();
		uiFreeImageBuffer(s.buffer);
		assert_int_equal(remaining, 0);
	}
}

static void nearestCrops(void **state)
{
	oversizedCrops(0, uiImageBufferInterpolationNearest, 96);
	oversizedCrops(1, uiImageBufferInterpolationNearest, 144);
}
static void smoothCrops(void **state)
{
	oversizedCrops(0, uiImageBufferInterpolationSmooth, 144);
	oversizedCrops(1, uiImageBufferInterpolationSmooth, 96);
}
static int setup(void **state)
{
	uiInitOptions o = {0};
	assert_null(uiInit(&o));
	return 0;
}
static int teardown(void **state) { uiUninit(); return 0; }

int main(void)
{
	const CMUnitTest tests[] = {
		cmocka_unit_test(nearestCrops), cmocka_unit_test(smoothCrops),
		cmocka_unit_test(submittedPixelsAcrossTargets), cmocka_unit_test(partialUpdatesAcrossFrames),
		cmocka_unit_test(nativeTargetsRetireTheirCaches),
	};
	return cmocka_run_group_tests_name("ImageBuffer Windows pixels", tests, setup, teardown);
}
