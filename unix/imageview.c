// uiImageView — GTK3 implementation (MVP, copy-owned)
#include "uipriv_unix.h"
#include "ui.h"

#define uiImageViewSignature 0x49566965

struct uiImageView {
	uiUnixControl c;
	GtkWidget *widget;          // actual widget exposed to libui
	GtkWidget *area;            // GtkDrawingArea for custom draw
	uiImageViewContentMode mode;
	uiImage *image;             // owned copy for drawing (may be NULL)
};

uiUnixControlAllDefaultsExceptDestroy(uiImageView)

static void uiImageViewDestroy(uiControl *c)
{
	uiImageView *v = uiImageView(c);
	if (v->image) {
		uiFreeImage(v->image);
		v->image = NULL;
	}
	g_object_unref(v->widget);
	uiFreeControl(uiControl(v));
}

static void compute_target_rect(int viewW, int viewH, double imgW, double imgH,
	uiImageViewContentMode mode, double *dx, double *dy, double *dw, double *dh)
{
	double vw = viewW, vh = viewH, iw = imgW, ih = imgH;
	
	// Initialize output parameters to avoid uninitialized warnings
	*dx = *dy = *dw = *dh = 0;
	
	if (iw <= 0 || ih <= 0 || vw <= 0 || vh <= 0) {
		return;
	}
	
	double sx = vw / iw, sy = vh / ih;
	switch (mode) {
	case uiImageViewContentCenter:
		*dw = iw; *dh = ih;
		break;
	case uiImageViewContentFit: {
		double s = sx < sy ? sx : sy;
		*dw = iw * s; *dh = ih * s;
		break;
	}
	}
	*dx = (vw - *dw) * 0.5;
	*dy = (vh - *dh) * 0.5;
}

static gboolean on_draw(GtkWidget *w, cairo_t *cr, gpointer data)
{
	uiImageView *v = uiImageView(data);
	GtkAllocation a;
	cairo_surface_t *surface;
	double imgW, imgH;
	int surfaceW, surfaceH;
	gtk_widget_get_allocation(w, &a);

	if (v->image == NULL)
		return FALSE;

	surface = uiprivImageAppropriateSurface(v->image, w);
	if (surface == NULL)
		return FALSE;

	uiprivImageSize(v->image, &imgW, &imgH);
	surfaceW = cairo_image_surface_get_width(surface);
	surfaceH = cairo_image_surface_get_height(surface);
	if (surfaceW <= 0 || surfaceH <= 0)
		return FALSE;

	double dx, dy, dw, dh;
	compute_target_rect(a.width, a.height, imgW, imgH, v->mode, &dx, &dy, &dw, &dh);

	cairo_save(cr);
	cairo_translate(cr, dx, dy);
	cairo_rectangle(cr, 0, 0, dw, dh);
	cairo_clip(cr);
	cairo_scale(cr, dw / surfaceW, dh / surfaceH);
	cairo_set_source_surface(cr, surface, 0, 0);
	cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_GOOD);
	cairo_paint(cr);
	cairo_restore(cr);
	return TRUE;
}

uiImageView *uiNewImageView(void)
{
	uiImageView *v;

	uiUnixNewControl(uiImageView, v);

	v->area = gtk_drawing_area_new();
	v->widget = v->area;
	v->mode = uiImageViewContentFit;
	v->image = NULL;

	// Default preferred size when no image (MVP)
	gtk_widget_set_size_request(v->widget, 64, 64);

	g_signal_connect(v->area, "draw", G_CALLBACK(on_draw), v);

	return v;
}

void uiImageViewSetContentMode(uiImageView *v, uiImageViewContentMode mode)
{
	v->mode = mode;
	gtk_widget_queue_draw(v->area);
}

void uiImageViewSetImage(uiImageView *v, const uiImage *image)
{
	if (v->image) {
		uiFreeImage(v->image);
		v->image = NULL;
	}

	if (image == NULL) {
		// Reset to default size when no image
		gtk_widget_set_size_request(v->widget, 64, 64);
		gtk_widget_queue_draw(v->area);
		return;
	}

	v->image = uiprivImageCopy((uiImage *)image);

	// Set small minimum size to allow icon-sized usage (16x16)
	// This allows the image to be displayed smaller than its original size
	gtk_widget_set_size_request(v->widget, 16, 16);

	gtk_widget_queue_draw(v->area);
}
