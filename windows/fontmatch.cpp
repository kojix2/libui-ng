// 11 march 2018
#include "uipriv_windows.hpp"
#include "attrstr.hpp"

static const std::map<uiTextItalic, DWRITE_FONT_STYLE> dwriteItalics = {
	{ uiTextItalicNormal, DWRITE_FONT_STYLE_NORMAL },
	{ uiTextItalicOblique, DWRITE_FONT_STYLE_OBLIQUE },
	{ uiTextItalicItalic, DWRITE_FONT_STYLE_ITALIC },
};

static const std::map<uiTextStretch, DWRITE_FONT_STRETCH> dwriteStretches = {
	{ uiTextStretchUltraCondensed, DWRITE_FONT_STRETCH_ULTRA_CONDENSED },
	{ uiTextStretchExtraCondensed, DWRITE_FONT_STRETCH_EXTRA_CONDENSED },
	{ uiTextStretchCondensed, DWRITE_FONT_STRETCH_CONDENSED },
	{ uiTextStretchSemiCondensed, DWRITE_FONT_STRETCH_SEMI_CONDENSED },
	{ uiTextStretchNormal, DWRITE_FONT_STRETCH_NORMAL },
	{ uiTextStretchSemiExpanded, DWRITE_FONT_STRETCH_SEMI_EXPANDED },
	{ uiTextStretchExpanded, DWRITE_FONT_STRETCH_EXPANDED },
	{ uiTextStretchExtraExpanded, DWRITE_FONT_STRETCH_EXTRA_EXPANDED },
	{ uiTextStretchUltraExpanded, DWRITE_FONT_STRETCH_ULTRA_EXPANDED },
};

// DirectWrite accepts weights from 1 through 999, while libui accepts 0
// through 1000. The named weights otherwise use the same numeric values.
DWRITE_FONT_WEIGHT uiprivWeightToDWriteWeight(uiTextWeight w)
{
	if (w < 1)
		return (DWRITE_FONT_WEIGHT) 1;
	if (w > 999)
		return (DWRITE_FONT_WEIGHT) 999;
	return (DWRITE_FONT_WEIGHT) w;
}

DWRITE_FONT_STYLE uiprivItalicToDWriteStyle(uiTextItalic i)
{
	return dwriteItalics.at(i);
}

DWRITE_FONT_STRETCH uiprivStretchToDWriteStretch(uiTextStretch s)
{
	return dwriteStretches.at(s);
}

void uiprivFontDescriptorFromIDWriteFont(IDWriteFont *font, uiFontDescriptor *uidesc)
{
	DWRITE_FONT_STYLE dwitalic;
	DWRITE_FONT_STRETCH dwstretch;

	if (font == NULL) {
		uidesc->Weight = uiTextWeightNormal;
		uidesc->Italic = uiTextItalicNormal;
		uidesc->Stretch = uiTextStretchNormal;
		return;
	}

	dwitalic = font->GetStyle();
	// DirectWrite's entire output range already fits within libui's range.
	uidesc->Weight = (uiTextWeight) (font->GetWeight());
	dwstretch = font->GetStretch();

	for (uidesc->Italic = uiTextItalicNormal; uidesc->Italic < uiTextItalicItalic; uidesc->Italic++)
		if (dwriteItalics.at(uidesc->Italic) == dwitalic)
			break;
	for (uidesc->Stretch = uiTextStretchUltraCondensed; uidesc->Stretch < uiTextStretchUltraExpanded; uidesc->Stretch++)
		if (dwriteStretches.at(uidesc->Stretch) == dwstretch)
			break;
}
