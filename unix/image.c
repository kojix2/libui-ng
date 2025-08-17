// 27 june 2016
#include "uipriv_unix.h"

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

	i = uiprivNew(uiImage);
	i->width = width;
	i->height = height;
	i->images = g_ptr_array_new_with_free_func(freeImageRep);
	return i;
}

void uiFreeImage(uiImage *i)
{
	g_ptr_array_free(i->images, TRUE);
	uiprivFree(i);
}

void uiImageAppend(uiImage *i, void *pixels, int pixelWidth, int pixelHeight, int byteStride)
{
	cairo_surface_t *cs;
	uint8_t *data, *pix;
	int realStride;
	int x, y;

	// note that this is native-endian
	cs = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
		pixelWidth, pixelHeight);
	if (cairo_surface_status(cs) != CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(cs);
		return; // Failed to create surface, abort operation
	}
	cairo_surface_flush(cs);

	pix = (uint8_t *) pixels;
	data = (uint8_t *) cairo_image_surface_get_data(cs);
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
		pix += byteStride;
		data += realStride;
	}

	cairo_surface_mark_dirty(cs);
	g_ptr_array_add(i->images, cs);
}

struct matcher {
	cairo_surface_t *best;
	int distX;
	int distY;
	int targetX;
	int targetY;
	gboolean foundLarger;
};

// TODO is this the right algorithm?
static void match(gpointer surface, gpointer data)
{
	cairo_surface_t *cs = (cairo_surface_t *) surface;
	struct matcher *m = (struct matcher *) data;
	int x, y;
	int x2, y2;

	x = cairo_image_surface_get_width(cs);
	y = cairo_image_surface_get_height(cs);
	if (m->best == NULL)
		goto writeMatch;

	if (x < m->targetX && y < m->targetY)
		if (m->foundLarger)
			// always prefer larger ones
			return;
	if (x >= m->targetX && y >= m->targetY && !m->foundLarger)
		// we set foundLarger below
		goto writeMatch;

	x2 = abs(m->targetX - x);
	y2 = abs(m->targetY - y);
	if (x2 < m->distX && y2 < m->distY)
		goto writeMatch;

	// TODO weight one dimension? threshhold?
	return;

writeMatch:
	// must set this here too; otherwise the first image will never have ths set
	if (x >= m->targetX && y >= m->targetY && !m->foundLarger)
		m->foundLarger = TRUE;
	m->best = cs;
	m->distX = abs(m->targetX - x);
	m->distY = abs(m->targetY - y);
}

cairo_surface_t *uiprivImageAppropriateSurface(uiImage *i, GtkWidget *w)
{
	struct matcher m;

	m.best = NULL;
	m.distX = G_MAXINT;
	m.distY = G_MAXINT;
	
	// Use logical size for matching
	m.targetX = i->width;
	m.targetY = i->height;
	m.foundLarger = FALSE;
	g_ptr_array_foreach(i->images, match, &m);
	
	// Return the best match without copying (original contract)
	return m.best;
}

cairo_surface_t *uiprivImageCopyAppropriateSurface(uiImage *i, GtkWidget *w)
{
	struct matcher m;
	cairo_surface_t *copy;
	cairo_t *cr;
	int width, height;
	cairo_format_t format;

	m.best = NULL;
	m.distX = G_MAXINT;
	m.distY = G_MAXINT;
	
	// Use logical size for matching
	m.targetX = i->width;
	m.targetY = i->height;
	m.foundLarger = FALSE;
	g_ptr_array_foreach(i->images, match, &m);
	
	if (m.best == NULL)
		return NULL;

	// Create a copy of the surface to ensure copy-owned semantics
	width = cairo_image_surface_get_width(m.best);
	height = cairo_image_surface_get_height(m.best);
	format = cairo_image_surface_get_format(m.best);
	
	copy = cairo_image_surface_create(format, width, height);
	if (cairo_surface_status(copy) != CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(copy);
		return NULL;
	}
	
	cr = cairo_create(copy);
	if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
		cairo_destroy(cr);
		cairo_surface_destroy(copy);
		return NULL;
	}
	
	cairo_set_source_surface(cr, m.best, 0, 0);
	cairo_paint(cr);
	cairo_destroy(cr);
	
	return copy;
}
