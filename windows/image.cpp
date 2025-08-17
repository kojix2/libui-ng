#include "uipriv_windows.hpp"

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
	if (uiprivWICFactory != NULL) {
		uiprivWICFactory->Release();
		uiprivWICFactory = NULL;
	}
}

struct uiImage {
	double width;
	double height;
	std::vector<IWICBitmap *> *bitmaps;
};

uiImage *uiNewImage(double width, double height)
{
	uiImage *i;

	i = uiprivNew(uiImage);
	i->width = width;
	i->height = height;
	i->bitmaps = new std::vector<IWICBitmap *>;
	return i;
}

void uiFreeImage(uiImage *i)
{
	for (IWICBitmap *b : *(i->bitmaps))
		b->Release();
	delete i->bitmaps;
	uiprivFree(i);
}

// to make things easier, we store images in WIC in the same way we store them in GDI (as system-endian ARGB) and premultiplied (as that's what AlphaBlend() expects (TODO confirm this))
// but what WIC format is system-endian ARGB? for a little-endian system, that's BGRA
// it turns out that the Windows 8 BMP encoder uses BGRA if told to (https://docs.microsoft.com/en-us/windows/desktop/wic/bmp-format-overview)
// it also turns out Direct2D requires PBGRA for drawing (https://docs.microsoft.com/en-us/windows/desktop/wic/-wic-bitmapsources-howto-drawusingd2d)
// so I guess we can assume PBGRA is correct...? (TODO)
#define formatForGDI GUID_WICPixelFormat32bppPBGRA

void uiImageAppend(uiImage *i, void *pixels, int pixelWidth, int pixelHeight, int byteStride)
{
	IWICBitmap *b;
	WICRect r;
	IWICBitmapLock *l;
	uint8_t *pix, *data;
	// TODO WICInProcPointer is not available in MinGW-w64
	BYTE *dipp;
	UINT size;
	UINT realStride;
	int x, y;
	HRESULT hr;

	hr = uiprivWICFactory->CreateBitmap(pixelWidth, pixelHeight,
		formatForGDI, WICBitmapCacheOnDemand,
		&b);
	if (hr != S_OK) {
		logHRESULT(L"error calling CreateBitmap() in uiImageAppend()", hr);
		return; // Failed to create bitmap, abort operation
	}
	r.X = 0;
	r.Y = 0;
	r.Width = pixelWidth;
	r.Height = pixelHeight;
	hr = b->Lock(&r, WICBitmapLockWrite, &l);
	if (hr != S_OK) {
		logHRESULT(L"error calling Lock() in uiImageAppend()", hr);
		b->Release();
		return; // Failed to lock bitmap, abort operation
	}

	pix = (uint8_t *) pixels;
	// TODO can size be NULL?
	hr = l->GetDataPointer(&size, &dipp);
	if (hr != S_OK)
		logHRESULT(L"error calling GetDataPointer() in uiImageAppend()", hr);
	data = (uint8_t *) dipp;
	hr = l->GetStride(&realStride);
	if (hr != S_OK)
		logHRESULT(L"error calling GetStride() in uiImageAppend()", hr);
	for (y = 0; y < pixelHeight; y++) {
		for (x = 0; x < pixelWidth * 4; x += 4) {
			data[x] = pix[x + 2];
			data[x + 1] = pix[x + 1];
			data[x + 2] = pix[x];
			data[x + 3] = pix[x + 3];
		}
		pix += byteStride;
		data += realStride;
	}

	l->Release();
	i->bitmaps->push_back(b);
}

struct matcher {
	IWICBitmap *best;
	int distX;
	int distY;
	int targetX;
	int targetY;
	bool foundLarger;
};

// TODO is this the right algorithm?
static void match(IWICBitmap *b, struct matcher *m)
{
	UINT ux, uy;
	int x, y;
	int x2, y2;
	HRESULT hr;

	hr = b->GetSize(&ux, &uy);
	if (hr != S_OK)
		logHRESULT(L"error calling GetSize() in match()", hr);
	x = ux;
	y = uy;
	if (m->best == NULL)
		goto writeMatch;

	if (x < m->targetX && y < m->targetY)
		if (m->foundLarger)
			// always prefer larger ones
			return;
	if (x >= m->targetX && y >= m->targetY && !m->foundLarger)
		// we set foundLarger below
		goto writeMatch;

// TODO
#define abs(x) ((x) < 0 ? -(x) : (x))
	x2 = abs(m->targetX - x);
	y2 = abs(m->targetY - y);
	if (x2 < m->distX && y2 < m->distY)
		goto writeMatch;

	// TODO weight one dimension? threshhold?
	return;

writeMatch:
	// must set this here too; otherwise the first image will never have ths set
	if (x >= m->targetX && y >= m->targetY && !m->foundLarger)
		m->foundLarger = true;
	m->best = b;
	m->distX = abs(m->targetX - x);
	m->distY = abs(m->targetY - y);
}

IWICBitmap *uiprivImageAppropriateForDC(uiImage *i, HDC dc)
{
	struct matcher m;

	m.best = NULL;
	m.distX = INT_MAX;
	m.distY = INT_MAX;
	// TODO explain this
	m.targetX = MulDiv(i->width, GetDeviceCaps(dc, LOGPIXELSX), 96);
	m.targetY = MulDiv(i->height, GetDeviceCaps(dc, LOGPIXELSY), 96);
	m.foundLarger = false;
	for (IWICBitmap *b : *(i->bitmaps))
		match(b, &m);
	return m.best;
}

// New function that takes DPI values directly instead of HDC
// This avoids the need for GDI Interop and handles NULL HDC cases safely
IWICBitmap *uiprivImageAppropriateForDPI(uiImage *i, float dpiX, float dpiY)
{
	struct matcher m;

	if (i == NULL)
		return NULL;

	m.best = NULL;
	m.distX = INT_MAX;
	m.distY = INT_MAX;
	// Use floating point calculation for better precision
	m.targetX = (int)((i->width * dpiX) / 96.0f);
	m.targetY = (int)((i->height * dpiY) / 96.0f);
	m.foundLarger = false;
	for (IWICBitmap *b : *(i->bitmaps))
		match(b, &m);
	return m.best;
}

// TODO this needs to center images if the given size is not the same aspect ratio
HRESULT uiprivWICToGDI(IWICBitmap *b, HDC dc, int width, int height, HBITMAP *hb)
{
	UINT ux, uy;
	int x, y;
	IWICBitmapSource *src;
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
		IWICBitmapScaler *scaler;
		WICPixelFormatGUID guid;
		IWICFormatConverter *conv;

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
				// TODO is the dither type correct in all cases?
				WICBitmapDitherTypeNone, NULL, 0, WICBitmapPaletteTypeMedianCut);
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

	// now we need to figure out the stride of the image data GDI gave us
	// TODO find out if CreateDIBSection() fills that in bmi for us
	// TODO fill in the error returns here too
	if (GetObject(*hb, sizeof (BITMAP), &bmp) == 0)
		logLastError(L"error calling GetObject() in uiprivWICToGDI()");
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

// Convert uiImage to ID2D1Bitmap for D2D1 rendering
// Similar to uiprivImageAppropriateForDC but for D2D1 render targets
extern "C" ID2D1Bitmap *uiprivImageToD2DBitmap(uiImage *img, ID2D1RenderTarget *rt)
{
	if (img == NULL || rt == NULL)
		return NULL;

	// Get render target DPI to select appropriate image size
	FLOAT dpiX, dpiY;
	rt->GetDpi(&dpiX, &dpiY);

	// Find best matching bitmap size (similar to uiprivImageAppropriateForDC)
	struct matcher m;
	m.best = NULL;
	m.distX = INT_MAX;
	m.distY = INT_MAX;
	// Scale target size based on DPI (similar to MulDiv logic in uiprivImageAppropriateForDC)
	m.targetX = (int)((img->width * dpiX) / 96.0);
	m.targetY = (int)((img->height * dpiY) / 96.0);
	m.foundLarger = false;

	for (IWICBitmap *b : *(img->bitmaps))
		match(b, &m);

	if (m.best == NULL)
		return NULL;

	// Create D2D1 bitmap from WIC bitmap
	ID2D1Bitmap *d2dBitmap = NULL;
	HRESULT hr = rt->CreateBitmapFromWicBitmap(m.best, NULL, &d2dBitmap);
	if (hr != S_OK) {
		logHRESULT(L"error calling CreateBitmapFromWicBitmap() in uiprivImageToD2DBitmap()", hr);
		return NULL;
	}

	return d2dBitmap;
}
