#ifndef __LIBUI_ATTRSTR_HPP__
#define __LIBUI_ATTRSTR_HPP__

#include "../common/attrstr.h"

// dwrite.cpp
extern IDWriteFactory *dwfactory;
extern HRESULT uiprivInitDrawText(void);
extern void uiprivUninitDrawText(void);
struct fontCollection {
	IDWriteFontCollection *fonts;
	WCHAR userLocale[LOCALE_NAME_MAX_LENGTH];
	int userLocaleSuccess;
};
extern fontCollection *uiprivLoadFontCollection(void);
extern WCHAR *uiprivFontCollectionFamilyName(fontCollection *fc, IDWriteFontFamily *family);
extern void uiprivFontCollectionFree(fontCollection *fc);
extern WCHAR *uiprivFontCollectionCorrectString(fontCollection *fc, IDWriteLocalizedStrings *names);

// opentype.cpp
extern IDWriteTypography *uiprivOpenTypeFeaturesToIDWriteTypography(const uiOpenTypeFeatures *otf);

// fontmatch.cpp
extern DWRITE_FONT_WEIGHT uiprivWeightToDWriteWeight(uiTextWeight w);
extern DWRITE_FONT_STYLE uiprivItalicToDWriteStyle(uiTextItalic i);
extern DWRITE_FONT_STRETCH uiprivStretchToDWriteStretch(uiTextStretch s);
extern void uiprivFontDescriptorFromIDWriteFont(IDWriteFont *font, uiFontDescriptor *uidesc);

// typographic points are 1/72 inch; DirectWrite font sizes are in DIPs (1/96 inch)
static inline FLOAT uiprivDWriteSizeFromPointSize(double size)
{
	return (FLOAT) (size * (96.0 / 72.0));
}

// attrstr.cpp
// TODO
struct drawTextBackgroundParams;
extern void uiprivAttributedStringApplyAttributesToDWriteTextLayout(uiDrawTextLayoutParams *p, IDWriteTextLayout *layout, std::vector<struct drawTextBackgroundParams *> **backgroundFuncs);

// drawtext.cpp
class drawingEffectsAttr : public IUnknown {
	ULONG refcount;

	bool hasColor;
	double r;
	double g;
	double b;
	double a;

	bool hasUnderline;
	uiUnderline u;

	bool hasUnderlineColor;
	double ur;
	double ug;
	double ub;
	double ua;
public:
	drawingEffectsAttr(void);

	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef(void);
	virtual ULONG STDMETHODCALLTYPE Release(void);

	void setColor(double red, double green, double blue, double alpha);
	void setUnderline(uiUnderline underlineType);
	void setUnderlineColor(double red, double green, double blue, double alpha);
	HRESULT mkColorBrush(ID2D1RenderTarget *rt, ID2D1SolidColorBrush **brush);
	HRESULT underline(uiUnderline *underlineType);
	HRESULT mkUnderlineBrush(ID2D1RenderTarget *rt, ID2D1SolidColorBrush **brush);
};
// TODO figure out where this type should *really* go in all the headers...
struct drawTextBackgroundParams {
	size_t start;
	size_t end;
	double r;
	double g;
	double b;
	double a;
};

// fontdialog.cpp
struct fontDialogParams {
	IDWriteFont *font;
	double size;
	WCHAR *familyName;
	WCHAR *styleName;
};
extern BOOL uiprivShowFontDialog(HWND parent, struct fontDialogParams *params);
extern void uiprivLoadInitialFontDialogParams(struct fontDialogParams *params);
extern void uiprivDestroyFontDialogParams(struct fontDialogParams *params);
extern WCHAR *uiprivFontDialogParamsToString(struct fontDialogParams *params);

#endif

