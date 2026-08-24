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

uiImage *uiprivImageCopy(const uiImage *i)
{
	uiImage *copy;

	if (i == NULL)
		return NULL;

	copy = uiNewImage(i->width, i->height);
	for (IWICBitmap *b : *(i->bitmaps)) {
		b->AddRef();
		copy->bitmaps->push_back(b);
	}
	return copy;
}

void uiprivImageSize(const uiImage *i, double *width, double *height)
{
	if (width != NULL)
		*width = i->width;
	if (height != NULL)
		*height = i->height;
}

// Store images as premultiplied BGRA, the format expected by Direct2D and by
// AlphaBlend() on little-endian Windows.
#define formatForGDI GUID_WICPixelFormat32bppPBGRA

void uiImageAppend(uiImage *i, const void *pixels, int pixelWidth, int pixelHeight, int byteStride)
{
	IWICBitmap *b = NULL;
	WICRect r;
	IWICBitmapLock *l = NULL;
	const uint8_t *pix;
	uint8_t *data;
	// MinGW-w64 does not declare the WICInProcPointer alias used by
	// IWICBitmapLock::GetDataPointer(); BYTE * is the equivalent type.
	BYTE *dipp;
	UINT size;
	UINT realStride;
	uint64_t destinationSize;
	int x, y;
	HRESULT hr;

	if (i == NULL) {
		uiprivUserBug("You cannot append a uiImage representation to NULL.");
		return;
	}
	if (pixels == NULL) {
		uiprivUserBug("You cannot append a NULL pixel buffer to a uiImage.");
		return;
	}
	if (pixelWidth <= 0) {
		uiprivUserBug("You cannot append a uiImage representation with pixel width %d.", pixelWidth);
		return;
	}
	if (pixelHeight <= 0) {
		uiprivUserBug("You cannot append a uiImage representation with pixel height %d.", pixelHeight);
		return;
	}
	if (pixelWidth > INT_MAX / 4) {
		uiprivUserBug("You cannot append a uiImage representation with pixel width %d.", pixelWidth);
		return;
	}
	if (byteStride < pixelWidth * 4) {
		uiprivUserBug("You cannot append a uiImage representation with byte stride %d and pixel width %d.", byteStride, pixelWidth);
		return;
	}
	if (!uiprivImagePixelBufferSpan(pixelWidth, pixelHeight,
		byteStride, NULL)) {
		uiprivUserBug("The uiImage representation pixel buffer is too large to address on this platform.");
		return;
	}

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

	pix = (const uint8_t *) pixels;
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
	destinationSize = (uint64_t) (pixelHeight - 1) * realStride +
		(uint64_t) pixelWidth * 4;
	if (realStride < (UINT) pixelWidth * 4 || destinationSize > size) {
		logHRESULT(L"WIC returned an invalid buffer layout in uiImageAppend()", E_FAIL);
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
		if (y + 1 < pixelHeight) {
			pix += byteStride;
			data += realStride;
		}
	}

	l->Release();
	i->bitmaps->push_back(b);
}

IWICBitmap *uiprivImageAppropriateForDC(const uiImage *i, HDC dc)
{
	int targetWidth, targetHeight;

	if (i == NULL)
		return NULL;
	// uiImage dimensions are in points at 96 DPI; select a representation
	// using the corresponding pixel dimensions for this device context.
	targetWidth = uiprivImageTargetPixelSize(i->width *
		GetDeviceCaps(dc, LOGPIXELSX) / 96.0);
	targetHeight = uiprivImageTargetPixelSize(i->height *
		GetDeviceCaps(dc, LOGPIXELSY) / 96.0);

	return uiprivImageAppropriateForSize(i,
		targetWidth, targetHeight);
}

IWICBitmap *uiprivImageAppropriateForSize(const uiImage *i,
	int pixelWidth, int pixelHeight)
{
	uiprivImageRepMatcher matcher;
	IWICBitmap *best;

	if (i == NULL)
		return NULL;
	uiprivImageRepMatcherInit(&matcher, pixelWidth, pixelHeight);
	best = NULL;
	for (IWICBitmap *bitmap : *(i->bitmaps)) {
		UINT width, height;
		HRESULT hr;

		hr = bitmap->GetSize(&width, &height);
		if (hr != S_OK) {
			logHRESULT(L"error calling GetSize() in uiprivImageAppropriateForSize()", hr);
			continue;
		}
		if (uiprivImageRepMatcherAdd(&matcher, (int) width, (int) height))
			best = bitmap;
	}
	return best;
}

HRESULT uiprivWICToGDI(IWICBitmap *b, HDC dc, int width, int height, HBITMAP *hb)
{
	UINT ux, uy;
	int x, y;
	int drawX, drawY, drawWidth, drawHeight;
	IWICBitmapSource *src = NULL;
	BITMAPINFO bmi;
	VOID *bits;
	BITMAP bmp;
	uint64_t bitmapHeight, bufferSize, destinationOffset;
	HRESULT hr;

	if (hb == NULL)
		return E_POINTER;
	*hb = NULL;
	if (b == NULL)
		return E_INVALIDARG;
	hr = b->GetSize(&ux, &uy);
	if (hr != S_OK)
		return hr;
	x = ux;
	y = uy;
	if (width == 0)
		width = x;
	if (height == 0)
		height = y;
	uiprivImageFitRect(x, y, width, height,
		&drawX, &drawY, &drawWidth, &drawHeight);
	if (drawWidth == 0 || drawHeight == 0)
		return E_INVALIDARG;

	// special case: don't invoke a scaler if the size is the same
	if (drawWidth == x && drawHeight == y) {
		b->AddRef();		// for the Release() later
		src = b;
	} else {
		IWICBitmapScaler *scaler = NULL;
		WICPixelFormatGUID guid;
		IWICFormatConverter *conv = NULL;

		hr = uiprivWICFactory->CreateBitmapScaler(&scaler);
		if (hr != S_OK)
			return hr;
		hr = scaler->Initialize(b, drawWidth, drawHeight,
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
	if (bmp.bmWidthBytes <= 0 || bmp.bmHeight == 0) {
		hr = E_FAIL;
		goto fail;
	}
	bitmapHeight = bmp.bmHeight < 0 ?
		(uint64_t) (-(int64_t) bmp.bmHeight) : (uint64_t) bmp.bmHeight;
	bufferSize = (uint64_t) bmp.bmWidthBytes * bitmapHeight;
	destinationOffset = (uint64_t) drawY * bmp.bmWidthBytes +
		(uint64_t) drawX * 4;
	if (bufferSize > UINT_MAX || destinationOffset >= bufferSize) {
		hr = HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);
		goto fail;
	}
	// Preserve transparent padding around the proportionally scaled image.
	ZeroMemory(bits, (SIZE_T) bufferSize);
	hr = src->CopyPixels(NULL, bmp.bmWidthBytes,
		(UINT) (bufferSize - destinationOffset),
		((BYTE *) bits) + destinationOffset);

fail:
	if (*hb != NULL && hr != S_OK) {
		// don't bother with the error returned here
		DeleteObject(*hb);
		*hb = NULL;
	}
	src->Release();
	return hr;
}
