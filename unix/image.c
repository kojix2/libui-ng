// 27 june 2016
#include "uipriv_unix.h"
#include <limits.h>

struct uiImage {
	double width;
	double height;
	GPtrArray *images;
};

static void freeImageRep(gpointer item)
{
	cairo_surface_t *cs = (cairo_surface_t *) item;

	cairo_surface_destroy(cs);
}

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
	i->images = g_ptr_array_new_with_free_func(freeImageRep);
	return i;
}

static void freeImage(void *p)
{
	uiImage *i = p;

	g_ptr_array_free(i->images, TRUE);
	uiprivFree(i);
}

void uiFreeImage(uiImage *i)
{
	if (uiprivUserCallbackDeferFree(i, freeImage))
		return;
	freeImage(i);
}

void uiImageAppend(uiImage *i, const void *pixels, int pixelWidth, int pixelHeight, int byteStride)
{
	cairo_surface_t *cs;
	uint8_t *data;
	const uint8_t *pix;
	int64_t minStride;
	int realStride;
	int x, y;

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
	minStride = (int64_t) pixelWidth * 4;
	if (byteStride < minStride) {
		uiprivUserBug("You cannot append a uiImage representation with byte stride %d and pixel width %d.", byteStride, pixelWidth);
		return;
	}
	if (!uiprivImagePixelBufferSpan(pixelWidth, pixelHeight,
		byteStride, NULL)) {
		uiprivUserBug("The uiImage representation pixel buffer is too large to address on this platform.");
		return;
	}

	// note that this is native-endian
	cs = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
		pixelWidth, pixelHeight);
	if (cairo_surface_status(cs) != CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(cs);
		return;
	}
	cairo_surface_flush(cs);

	pix = (const uint8_t *) pixels;
	data = (uint8_t *) cairo_image_surface_get_data(cs);
	if (data == NULL) {
		cairo_surface_destroy(cs);
		return;
	}
	realStride = cairo_image_surface_get_stride(cs);
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

	cairo_surface_mark_dirty(cs);
	g_ptr_array_add(i->images, cs);
}

cairo_surface_t *uiprivImageAppropriateSurface(uiImage *i, GtkWidget *w)
{
	uiprivImageRepMatcher matcher;
	cairo_surface_t *best;
	guint n;
	int targetWidth, targetHeight;
	int scale;

	if (i == NULL)
		return NULL;
	scale = gtk_widget_get_scale_factor(w);
	if (scale <= 0) {
		targetWidth = 0;
		targetHeight = 0;
	} else {
		targetWidth = uiprivImageTargetPixelSize(i->width * scale);
		targetHeight = uiprivImageTargetPixelSize(i->height * scale);
	}

	uiprivImageRepMatcherInit(&matcher, targetWidth, targetHeight);
	best = NULL;
	for (n = 0; n < i->images->len; n++) {
		cairo_surface_t *surface;
		int width, height;

		surface = g_ptr_array_index(i->images, n);
		width = cairo_image_surface_get_width(surface);
		height = cairo_image_surface_get_height(surface);
		if (uiprivImageRepMatcherAdd(&matcher, width, height))
			best = surface;
	}
	return best;
}

cairo_surface_t *uiprivImageAppropriateSurfaceForTable(uiImage *i, GtkWidget *w)
{
	cairo_surface_t *best;
	cairo_surface_t *surface;
	int width, height;

	best = uiprivImageAppropriateSurface(i, w);
	if (best == NULL)
		return NULL;
	width = cairo_image_surface_get_width(best);
	height = cairo_image_surface_get_height(best);
	surface = cairo_surface_create_for_rectangle(best, 0, 0, width, height);
	if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(surface);
		return NULL;
	}
	// GtkCellRendererPixbuf uses the device scale to obtain the logical size.
	// Keep it on this table-only subsurface so drawing APIs can continue to use
	// the representation's unmodified pixel coordinate system.
	cairo_surface_set_device_scale(surface,
		width / i->width, height / i->height);
	return surface;
}
