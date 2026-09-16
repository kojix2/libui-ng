// 11 june 2015
#include "uipriv_unix.h"
#include <float.h>

struct uiLabel {
	uiUnixControl c;
	GtkWidget *widget;
	GtkLabel *label;
	double fontSize;
	double defaultFontSize;
};

uiUnixControlAllDefaults(uiLabel)

char *uiLabelText(uiLabel *l)
{
	return uiUnixStrdupText(gtk_label_get_text(l->label));
}

void uiLabelSetText(uiLabel *l, const char *text)
{
	gtk_label_set_text(l->label, text);
}

static double labelDefaultFontSize(uiLabel *l)
{
	GtkStyleContext *style;
	PangoFontDescription *fontdesc;
	double size;

	style = gtk_widget_get_style_context(l->widget);
	gtk_style_context_get(style, GTK_STATE_FLAG_NORMAL,
		"font", &fontdesc, NULL);
	size = pango_units_to_double(pango_font_description_get_size(fontdesc));
	if (pango_font_description_get_size_is_absolute(fontdesc)) {
		double resolution;

		resolution = pango_cairo_context_get_resolution(
			gtk_widget_get_pango_context(l->widget));
		if (resolution <= 0)
			resolution = 96;
		size *= 72.0 / resolution;
	}
	pango_font_description_free(fontdesc);
	return size;
}

double uiLabelFontSize(uiLabel *l)
{
	return l->fontSize;
}

void uiLabelSetFontSize(uiLabel *l, double size)
{
	PangoAttrList *attrs;
	PangoAttribute *attr;

	if (!(size > 0) || size > DBL_MAX ||
		size > ((double) G_MAXINT / PANGO_SCALE)) {
		uiprivUserBug("uiLabelSetFontSize() size must be finite, positive, and representable by Pango.");
		return;
	}
	attrs = pango_attr_list_new();
	attr = pango_attr_size_new(pango_units_from_double(size));
	pango_attr_list_insert(attrs, attr);
	gtk_label_set_attributes(l->label, attrs);
	pango_attr_list_unref(attrs);
	l->fontSize = size;
	gtk_widget_queue_resize(l->widget);
}

void uiLabelResetFontSize(uiLabel *l)
{
	uiLabelSetFontSize(l, l->defaultFontSize);
}

uiLabel *uiNewLabel(const char *text)
{
	uiLabel *l;

	uiUnixNewControl(uiLabel, l);

	l->widget = gtk_label_new(text);
	l->label = GTK_LABEL(l->widget);

	gtk_label_set_xalign(l->label, 0);
	gtk_label_set_yalign(l->label, 0);
	l->fontSize = labelDefaultFontSize(l);
	l->defaultFontSize = l->fontSize;

	return l;
}
