#ifndef __LIBUI_DRAW_HPP__
#define __LIBUI_DRAW_HPP__

// TODO consolidate the drawing declarations currently split between this file,
// uipriv_windows.hpp, and _uipriv_migrate.hpp.

// draw.cpp
extern ID2D1Factory *d2dfactory;
static inline FLOAT uiprivD2DFloat(double value)
{
	return (FLOAT) value;
}

static inline void uiprivInitBrushProperties(D2D1_BRUSH_PROPERTIES *props, FLOAT opacity)
{
	ZeroMemory(props, sizeof (D2D1_BRUSH_PROPERTIES));
	props->opacity = opacity;
	props->transform._11 = 1;
	props->transform._22 = 1;
}

struct drawState {
	ID2D1DrawingStateBlock *dsb;
	ID2D1PathGeometry *clip;
};

struct uiDrawContext {
	ID2D1RenderTarget *rt;
	// uiDrawContext is allocated with uiprivNew(), which does not run C++
	// constructors, so construct and destroy the vector separately.
	std::vector<struct drawState> *states;
	ID2D1PathGeometry *currentClip;
};

// drawpath.cpp
extern ID2D1PathGeometry *pathGeometry(uiDrawPath *p);

// drawmatrix.cpp
extern void m2d(uiDrawMatrix *m, D2D1_MATRIX_3X2_F *d);

#endif

