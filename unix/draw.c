// 6 september 2015
#include "uipriv_unix.h"
#include "draw.h"

uiDrawContext *uiprivNewContext(cairo_t *cr, GtkStyleContext *style, GtkWidget *widget)
{
	uiDrawContext *c;

	c = uiprivNew(uiDrawContext);
	c->cr = cr;
	c->style = style;
	c->widget = widget;
	return c;
}

void uiprivFreeContext(uiDrawContext *c)
{
	// free neither cr nor style; we own neither
	uiprivFree(c);
}

static cairo_pattern_t *mkbrush(uiDrawBrush *b)
{
	cairo_pattern_t *pat;
	size_t i;

	switch (b->Type) {
	case uiDrawBrushTypeSolid:
		pat = cairo_pattern_create_rgba(b->R, b->G, b->B, b->A);
		break;
	case uiDrawBrushTypeLinearGradient:
		pat = cairo_pattern_create_linear(b->X0, b->Y0, b->X1, b->Y1);
		break;
	case uiDrawBrushTypeRadialGradient:
		// make the start circle radius 0 to make it a point
		pat = cairo_pattern_create_radial(
			b->X0, b->Y0, 0,
			b->X1, b->Y1, b->OuterRadius);
		break;
//	case uiDrawBrushTypeImage:
	}
	if (cairo_pattern_status(pat) != CAIRO_STATUS_SUCCESS)
		uiprivImplBug("error creating pattern in mkbrush(): %s",
			cairo_status_to_string(cairo_pattern_status(pat)));
	switch (b->Type) {
	case uiDrawBrushTypeLinearGradient:
	case uiDrawBrushTypeRadialGradient:
		for (i = 0; i < b->NumStops; i++)
			cairo_pattern_add_color_stop_rgba(pat,
				b->Stops[i].Pos,
				b->Stops[i].R,
				b->Stops[i].G,
				b->Stops[i].B,
				b->Stops[i].A);
	}
	return pat;
}

void uiDrawStroke(uiDrawContext *c, uiDrawPath *path, uiDrawBrush *b, uiDrawStrokeParams *p)
{
	cairo_pattern_t *pat;

	if (!(p->Thickness > 0))
		uiprivUserBug("uiDrawStroke() thickness must be > 0");
	if (p->NumDashes != 0 && p->Dashes == NULL)
		uiprivUserBug("uiDrawStroke() dashes must not be NULL when NumDashes is > 0");

	uiprivRunPath(path, c->cr);
	pat = mkbrush(b);
	cairo_set_source(c->cr, pat);
	switch (p->Cap) {
	case uiDrawLineCapFlat:
		cairo_set_line_cap(c->cr, CAIRO_LINE_CAP_BUTT);
		break;
	case uiDrawLineCapRound:
		cairo_set_line_cap(c->cr, CAIRO_LINE_CAP_ROUND);
		break;
	case uiDrawLineCapSquare:
		cairo_set_line_cap(c->cr, CAIRO_LINE_CAP_SQUARE);
		break;
	}
	switch (p->Join) {
	case uiDrawLineJoinMiter:
		cairo_set_line_join(c->cr, CAIRO_LINE_JOIN_MITER);
		cairo_set_miter_limit(c->cr, p->MiterLimit);
		break;
	case uiDrawLineJoinRound:
		cairo_set_line_join(c->cr, CAIRO_LINE_JOIN_ROUND);
		break;
	case uiDrawLineJoinBevel:
		cairo_set_line_join(c->cr, CAIRO_LINE_JOIN_BEVEL);
		break;
	}
	cairo_set_line_width(c->cr, p->Thickness);
	cairo_set_dash(c->cr, p->Dashes, p->NumDashes, p->DashPhase);
	cairo_stroke(c->cr);
	cairo_pattern_destroy(pat);
}

void uiDrawFill(uiDrawContext *c, uiDrawPath *path, uiDrawBrush *b)
{
	cairo_pattern_t *pat;

	uiprivRunPath(path, c->cr);
	pat = mkbrush(b);
	cairo_set_source(c->cr, pat);
	switch (uiprivPathFillMode(path)) {
	case uiDrawFillModeWinding:
		cairo_set_fill_rule(c->cr, CAIRO_FILL_RULE_WINDING);
		break;
	case uiDrawFillModeAlternate:
		cairo_set_fill_rule(c->cr, CAIRO_FILL_RULE_EVEN_ODD);
		break;
	}
	cairo_fill(c->cr);
	cairo_pattern_destroy(pat);
}

void uiDrawTransform(uiDrawContext *c, uiDrawMatrix *m)
{
	cairo_matrix_t cm;

	uiprivM2C(m, &cm);
	cairo_transform(c->cr, &cm);
}

void uiDrawClip(uiDrawContext *c, uiDrawPath *path)
{
	uiprivRunPath(path, c->cr);
	switch (uiprivPathFillMode(path)) {
	case uiDrawFillModeWinding:
		cairo_set_fill_rule(c->cr, CAIRO_FILL_RULE_WINDING);
		break;
	case uiDrawFillModeAlternate:
		cairo_set_fill_rule(c->cr, CAIRO_FILL_RULE_EVEN_ODD);
		break;
	}
	cairo_clip(c->cr);
}

void uiDrawSave(uiDrawContext *c)
{
	cairo_save(c->cr);
}

void uiDrawRestore(uiDrawContext *c)
{
	cairo_restore(c->cr);
}

void uiDrawImage(uiDrawContext *c, uiImage *img, double x, double y, double width, double height)
{
	cairo_surface_t *surface;
	cairo_pattern_t *pattern;
	double scaleX, scaleY;
	int surfaceWidth, surfaceHeight;

	// Enhanced parameter validation
	if (c == NULL || img == NULL || width <= 0 || height <= 0)
		return;

	// Select appropriate resolution image with widget context
	surface = uiprivImageAppropriateSurface(img, c->widget);
	if (surface == NULL)
		return;

	// Validate surface status
	if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS)
		return;

	// Handle different surface types appropriately
	if (cairo_surface_get_type(surface) == CAIRO_SURFACE_TYPE_RECORDING) {
		// For recording surfaces, we can get ink extents and scale properly
		double inkX, inkY, inkWidth, inkHeight;
		
		cairo_recording_surface_ink_extents(surface, &inkX, &inkY, &inkWidth, &inkHeight);
		
		// Only proceed if we have valid ink extents
		if (inkWidth > 0 && inkHeight > 0) {
			cairo_save(c->cr);

			pattern = cairo_pattern_create_for_surface(surface);
			if (cairo_pattern_status(pattern) != CAIRO_STATUS_SUCCESS) {
				cairo_restore(c->cr);
				return;
			}
			cairo_pattern_set_filter(pattern, CAIRO_FILTER_BILINEAR);
			cairo_pattern_set_extend(pattern, CAIRO_EXTEND_NONE);

			// Calculate scaling factors based on ink extents
			scaleX = width / inkWidth;
			scaleY = height / inkHeight;

			// Apply transformation: translate to position, then scale
			cairo_translate(c->cr, x, y);
			cairo_scale(c->cr, scaleX, scaleY);
			cairo_translate(c->cr, -inkX, -inkY);

			cairo_set_source(c->cr, pattern);
			cairo_paint(c->cr);

			cairo_pattern_destroy(pattern);
			cairo_restore(c->cr);
			return;
		}
		// Fall through to generic handling if ink extents are invalid
	}
	
	if (cairo_surface_get_type(surface) != CAIRO_SURFACE_TYPE_IMAGE) {
		// For other non-image surfaces, paint at native scale with clipping
		cairo_save(c->cr);

		pattern = cairo_pattern_create_for_surface(surface);
		if (cairo_pattern_status(pattern) != CAIRO_STATUS_SUCCESS) {
			cairo_restore(c->cr);
			return;
		}
		cairo_pattern_set_filter(pattern, CAIRO_FILTER_BILINEAR);
		cairo_pattern_set_extend(pattern, CAIRO_EXTEND_NONE);

		// Clip to destination rect
		cairo_rectangle(c->cr, x, y, width, height);
		cairo_clip(c->cr);

		// Translate to (x, y) so the surface is anchored correctly
		cairo_translate(c->cr, x, y);

		cairo_set_source(c->cr, pattern);
		cairo_paint(c->cr);

		cairo_pattern_destroy(pattern);
		cairo_restore(c->cr);
		return;
	}

	// Get surface dimensions (safe for image surfaces)
	surfaceWidth = cairo_image_surface_get_width(surface);
	surfaceHeight = cairo_image_surface_get_height(surface);
	if (surfaceWidth <= 0 || surfaceHeight <= 0)
		return;

	// Calculate scaling factors
	scaleX = width / (double)surfaceWidth;
	scaleY = height / (double)surfaceHeight;

	// Save current drawing state
	cairo_save(c->cr);

	// Create pattern with high-quality scaling filter
	pattern = cairo_pattern_create_for_surface(surface);
	if (cairo_pattern_status(pattern) != CAIRO_STATUS_SUCCESS) {
		cairo_restore(c->cr);
		return;
	}
	cairo_pattern_set_filter(pattern, CAIRO_FILTER_BILINEAR);
	cairo_pattern_set_extend(pattern, CAIRO_EXTEND_NONE);

	// Apply transformation matrix
	cairo_translate(c->cr, x, y);
	cairo_scale(c->cr, scaleX, scaleY);

	// Draw the image with pattern
	cairo_set_source(c->cr, pattern);
	cairo_paint(c->cr);

	// Cleanup
	cairo_pattern_destroy(pattern);
	cairo_restore(c->cr);
}
