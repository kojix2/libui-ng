// 12 february 2017
#include "uipriv_unix.h"
#include "attrstr.h"

struct foreachParams {
	PangoAttrList *attrs;
};

static guint16 pangoAlpha(double alpha)
{
	guint16 value;

	value = (guint16) (alpha * G_MAXUINT16);
	// Pango uses 0 to represent 65536, so use its smallest alpha instead.
	if (value == 0)
		return 1;
	return value;
}

static void addattr(struct foreachParams *p, size_t start, size_t end, PangoAttribute *attr)
{
	if (attr == NULL)		// in case of a future attribute
		return;
	attr->start_index = start;
	attr->end_index = end;
	pango_attr_list_insert(p->attrs, attr);
}

static uiForEach processAttribute(const uiAttributedString *s, const uiAttribute *attr, size_t start, size_t end, void *data)
{
	struct foreachParams *p = (struct foreachParams *) data;
	double r, g, b, a;
	PangoUnderline underline = PANGO_UNDERLINE_NONE;
	uiUnderlineColor colorType;
	const uiOpenTypeFeatures *features;
	GString *featurestr;

	switch (uiAttributeGetType(attr)) {
	case uiAttributeTypeFamily:
		addattr(p, start, end,
			pango_attr_family_new(uiAttributeFamily(attr)));
		break;
	case uiAttributeTypeSize:
		addattr(p, start, end,
			pango_attr_size_new(cairoToPango(uiAttributeSize(attr))));
		break;
	case uiAttributeTypeWeight:
		addattr(p, start, end,
			pango_attr_weight_new(uiprivWeightToPangoWeight(uiAttributeWeight(attr))));
		break;
	case uiAttributeTypeItalic:
		addattr(p, start, end,
			pango_attr_style_new(uiprivItalicToPangoStyle(uiAttributeItalic(attr))));
		break;
	case uiAttributeTypeStretch:
		addattr(p, start, end,
			pango_attr_stretch_new(uiprivStretchToPangoStretch(uiAttributeStretch(attr))));
		break;
	case uiAttributeTypeColor:
		uiAttributeColor(attr, &r, &g, &b, &a);
		addattr(p, start, end,
			pango_attr_foreground_new(
				(guint16) (r * G_MAXUINT16),
				(guint16) (g * G_MAXUINT16),
				(guint16) (b * G_MAXUINT16)));
		addattr(p, start, end,
			uiprivFUTURE_pango_attr_foreground_alpha_new(
				pangoAlpha(a)));
		break;
	case uiAttributeTypeBackground:
		uiAttributeColor(attr, &r, &g, &b, &a);
		addattr(p, start, end,
			pango_attr_background_new(
				(guint16) (r * G_MAXUINT16),
				(guint16) (g * G_MAXUINT16),
				(guint16) (b * G_MAXUINT16)));
		addattr(p, start, end,
			uiprivFUTURE_pango_attr_background_alpha_new(
				pangoAlpha(a)));
		break;
	case uiAttributeTypeUnderline:
		switch (uiAttributeUnderline(attr)) {
		case uiUnderlineNone:
			underline = PANGO_UNDERLINE_NONE;
			break;
		case uiUnderlineSingle:
			underline = PANGO_UNDERLINE_SINGLE;
			break;
		case uiUnderlineDouble:
			underline = PANGO_UNDERLINE_DOUBLE;
			break;
		case uiUnderlineSuggestion:
			underline = PANGO_UNDERLINE_ERROR;
			break;
		}
		addattr(p, start, end,
			pango_attr_underline_new(underline));
		break;
	case uiAttributeTypeUnderlineColor:
		uiAttributeUnderlineColor(attr, &colorType, &r, &g, &b, &a);
		switch (colorType) {
		case uiUnderlineColorCustom:
			addattr(p, start, end,
				pango_attr_underline_color_new(
					(guint16) (r * G_MAXUINT16),
					(guint16) (g * G_MAXUINT16),
					(guint16) (b * G_MAXUINT16)));
			break;
		case uiUnderlineColorSpelling:
			// This layout has no GtkTextView style context from which to
			// obtain the theme's error underline color, so use red.
			addattr(p, start, end,
				pango_attr_underline_color_new(G_MAXUINT16, 0, 0));
			break;
		case uiUnderlineColorGrammar:
			// GTK defines no standard grammar underline color; use green.
			addattr(p, start, end,
				pango_attr_underline_color_new(0, G_MAXUINT16, 0));
			break;
		case uiUnderlineColorAuxiliary:
			// GTK defines no standard auxiliary underline color; use blue.
			addattr(p, start, end,
				pango_attr_underline_color_new(0, 0, G_MAXUINT16));
			break;
		}
		break;
	case uiAttributeTypeFeatures:
		// only generate an attribute if the features object is not NULL
		features = uiAttributeFeatures(attr);
		if (features == NULL)
			break;
		featurestr = uiprivOpenTypeFeaturesToPangoCSSFeaturesString(features);
		addattr(p, start, end,
			uiprivFUTURE_pango_attr_font_features_new(featurestr->str));
		g_string_free(featurestr, TRUE);
		break;
	}
	return uiForEachContinue;
}

PangoAttrList *uiprivAttributedStringToPangoAttrList(uiDrawTextLayoutParams *p)
{
	struct foreachParams fep;

	fep.attrs = pango_attr_list_new();
	uiAttributedStringForEachAttribute(p->String, processAttribute, &fep);
	return fep.attrs;
}
