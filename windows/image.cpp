#include "uipriv_windows.hpp"
#include <limits.h>

// TODO:
// - is the alpha channel ignored when drawing images in tables?

IWICImagingFactory *uiprivWICFactory = NULL;

HRESULT uiprivInitImage(void)
{
	return CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
		IID_IWICImagingFactory, (void **) (&uiprivWICFactory));
}

void uiprivUninitImage(void)
{
	uiprivWICFactory->Release();
	uiprivWICFactory = NULL;
}

struct uiImage {
	double width;
	double height;
	std::vector<IWICBitmap *> *bitmaps;
};

uiImage *uiNewImage(double width, double height)
{
	uiImage *i;

	if (!(width > 0) || width > INT_MAX ||
		!(height > 0) || height > INT_MAX) {
		uiprivUserBug("uiNewImage() dimensions must be finite, positive, and no greater than INT_MAX.");
		return NULL;
	}
	i = uiprivNew(uiImage);
	i->width = width;
	i->height = height;
	i->bitmaps = new std::vector<IWICBitmap *>;
	return i;
}

static void freeImage(void *p)
{
	uiImage *i = (uiImage *) p;

	for (IWICBitmap *b : *(i->bitmaps))
		b->Release();
	delete i->bitmaps;
	uiprivFree(i);
}

void uiFreeImage(uiImage *i)
{
	if (uiprivUserCallbackDeferFree(i, freeImage))
		return;
	freeImage(i);
}

// Store images as premultiplied BGRA, the format expected by Direct2D and by
// AlphaBlend() on little-endian Windows.
#define formatForGDI GUID_WICPixelFormat32bppPBGRA

void uiImageAppend(uiImage *i, void *pixels, int pixelWidth, int pixelHeight, int byteStride)
{
	IWICBitmap *b = NULL;
	WICRect r;
	IWICBitmapLock *l = NULL;
	uint8_t *pix, *data;
	// MinGW-w64 does not declare the WICInProcPointer alias used by
	// IWICBitmapLock::GetDataPointer(); BYTE * is the equivalent type.
	BYTE *dipp;
	UINT size;
	UINT realStride;
	int x, y;
	HRESULT hr;

	if (i == NULL)
		uiprivUserBug("You cannot append a uiImage representation to NULL.");
	if (pixels == NULL)
		uiprivUserBug("You cannot append a NULL pixel buffer to a uiImage.");
	if (pixelWidth <= 0)
		uiprivUserBug("You cannot append a uiImage representation with pixel width %d.", pixelWidth);
	if (pixelHeight <= 0)
		uiprivUserBug("You cannot append a uiImage representation with pixel height %d.", pixelHeight);
	if (pixelWidth > INT_MAX / 4)
		uiprivUserBug("You cannot append a uiImage representation with pixel width %d.", pixelWidth);
	if (byteStride < pixelWidth * 4)
		uiprivUserBug("You cannot append a uiImage representation with byte stride %d and pixel width %d.", byteStride, pixelWidth);

	hr = uiprivWICFactory->CreateBitmap(pixelWidth, pixelHeight,
		formatForGDI, WICBitmapCacheOnDemand,
		&b);
	if (hr != S_OK) {
		logHRESULT(L"error calling CreateBitmap() in uiImageAppend()", hr);
		return;
	}
	r.X = 0;
	r.Y = 0;
	r.Width = pixelWidth;
	r.Height = pixelHeight;
	hr = b->Lock(&r, WICBitmapLockWrite, &l);
	if (hr != S_OK) {
		logHRESULT(L"error calling Lock() in uiImageAppend()", hr);
		b->Release();
		return;
	}

	pix = (uint8_t *) pixels;
	hr = l->GetDataPointer(&size, &dipp);
	if (hr != S_OK) {
		logHRESULT(L"error calling GetDataPointer() in uiImageAppend()", hr);
		l->Release();
		b->Release();
		return;
	}
	data = (uint8_t *) dipp;
	hr = l->GetStride(&realStride);
	if (hr != S_OK) {
		logHRESULT(L"error calling GetStride() in uiImageAppend()", hr);
		l->Release();
		b->Release();
		return;
	}
	for (y = 0; y < pixelHeight; y++) {
		for (x = 0; x < pixelWidth * 4; x += 4) {
			union {
				uint32_t v32;
				uint8_t v8[4];
			} v;

			v.v32 = ((uint32_t) (pix[x + 3])) << 24;
			v.v32 |= ((uint32_t) (pix[x])) << 16;
			v.v32 |= ((uint32_t) (pix[x + 1])) << 8;
			v.v32 |= ((uint32_t) (pix[x + 2]));
			data[x] = v.v8[0];
			data[x + 1] = v.v8[1];
			data[x + 2] = v.v8[2];
			data[x + 3] = v.v8[3];
		}
		pix += byteStride;
		data += realStride;
	}

	l->Release();
	i->bitmaps->push_back(b);
}

IWICBitmap *uiprivImageAppropriateForDC(uiImage *i, HDC dc)
{
	uiprivImageRepMatcher matcher;
	IWICBitmap *best;
	int targetWidth, targetHeight;
	int target;

	// uiImage dimensions are in points at 96 DPI; select a representation
	// using the corresponding pixel dimensions for this device context.
	target = MulDiv((int) i->width, GetDeviceCaps(dc, LOGPIXELSX), 96);
	targetWidth = target == -1 ? INT_MAX : target;
	target = MulDiv((int) i->height, GetDeviceCaps(dc, LOGPIXELSY), 96);
	targetHeight = target == -1 ? INT_MAX : target;

	uiprivImageRepMatcherInit(&matcher, targetWidth, targetHeight);
	best = NULL;
	for (IWICBitmap *b : *(i->bitmaps)) {
		UINT width, height;
		HRESULT hr;

		hr = b->GetSize(&width, &height);
		if (hr != S_OK) {
			logHRESULT(L"error calling GetSize() in uiprivImageAppropriateForDC()", hr);
			continue;
		}
		if (uiprivImageRepMatcherAdd(&matcher, (int) width, (int) height))
			best = b;
	}
	return best;
}

// TODO this needs to center images if the given size is not the same aspect ratio
HRESULT uiprivWICToGDI(IWICBitmap *b, HDC dc, int width, int height, HBITMAP *hb)
{
	UINT ux, uy;
	int x, y;
	IWICBitmapSource *src = NULL;
	BITMAPINFO bmi;
	VOID *bits;
	BITMAP bmp;
	HRESULT hr;

	hr = b->GetSize(&ux, &uy);
	if (hr != S_OK)
		return hr;
	x = ux;
	y = uy;
	if (width == 0)
		width = x;
	if (height == 0)
		height = y;

	// special case: don't invoke a scaler if the size is the same
	if (width == x && height == y) {
		b->AddRef();		// for the Release() later
		src = b;
	} else {
		IWICBitmapScaler *scaler = NULL;
		WICPixelFormatGUID guid;
		IWICFormatConverter *conv = NULL;

		hr = uiprivWICFactory->CreateBitmapScaler(&scaler);
		if (hr != S_OK)
			return hr;
		hr = scaler->Initialize(b, width, height,
			// according to https://stackoverflow.com/questions/4250738/is-stretchblt-halftone-bilinear-for-all-scaling, this is what StretchBlt(COLORONCOLOR) does (with COLORONCOLOR being what's supported by AlphaBlend())
			WICBitmapInterpolationModeNearestNeighbor);
		if (hr != S_OK) {
			scaler->Release();
			return hr;
		}

		// But we are not done yet! IWICBitmapScaler can use an
		// entirely different pixel format than what we gave it,
		// and by extension, what GDI wants. See also:
		// - https://stackoverflow.com/questions/28323228/iwicbitmapscaler-doesnt-work-for-96bpprgbfloat-format
		// - https://github.com/Microsoft/DirectXTex/blob/0d94e9469bc3e6080a71145f35efa559f8f2e522/DirectXTex/DirectXTexResize.cpp#L83
		hr = scaler->GetPixelFormat(&guid);
		if (hr != S_OK) {
			scaler->Release();
			return hr;
		}
		if (IsEqualGUID(guid, formatForGDI))
			src = scaler;
		else {
			hr = uiprivWICFactory->CreateFormatConverter(&conv);
			if (hr != S_OK) {
				scaler->Release();
				return hr;
			}
			hr = conv->Initialize(scaler, formatForGDI,
				// A 32-bit true-color destination needs neither dithering nor a palette.
				WICBitmapDitherTypeNone, NULL, 0, WICBitmapPaletteTypeCustom);
			scaler->Release();
			if (hr != S_OK) {
				conv->Release();
				return hr;
			}
			src = conv;
		}
	}

	ZeroMemory(&bmi, sizeof (BITMAPINFO));
	bmi.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -((int) height);
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	*hb = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS,
		&bits, NULL, 0);
	if (*hb == NULL) {
		logLastError(L"CreateDIBSection()");
		hr = E_FAIL;
		goto fail;
	}

	// BITMAPINFO is input-only for CreateDIBSection(); query the resulting
	// bitmap to obtain the actual row stride.
	if (GetObject(*hb, sizeof (BITMAP), &bmp) == 0) {
		logLastError(L"error calling GetObject() in uiprivWICToGDI()");
		hr = E_FAIL;
		goto fail;
	}
	hr = src->CopyPixels(NULL, bmp.bmWidthBytes,
		bmp.bmWidthBytes * bmp.bmHeight, (BYTE *) bits);

fail:
	if (*hb != NULL && hr != S_OK) {
		// don't bother with the error returned here
		DeleteObject(*hb);
		*hb = NULL;
	}
	src->Release();
	return hr;
}
