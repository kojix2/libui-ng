#ifndef __LIBUI_AREA_HPP__
#define __LIBUI_AREA_HPP__

struct uiArea {
	uiWindowsControl c;
	HWND hwnd;
	uiAreaHandler *ah;
	int (*onScroll)(uiArea *, uiAreaScrollEvent *, void *);
	void *onScrollData;

	BOOL scrolling;
	int scrollWidth;
	int scrollHeight;
	int hscrollpos;
	int vscrollpos;
	int64_t hwheelCarry;
	int64_t vwheelCarry;
	UINT hwheelAmount;
	UINT vwheelAmount;
	BOOL updatingScroll;

	uiprivClickCounter cc;
	BOOL capturing;
	BOOL inMouseDownEvent;

	BOOL inside;
	BOOL tracking;

	ID2D1HwndRenderTarget *rt;
};

// areadraw.cpp
extern BOOL areaDoDraw(uiArea *a, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *lResult);
extern void areaDrawOnResize(uiArea *, RECT *);

// areascroll.cpp
extern BOOL areaDoScroll(uiArea *a, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *lResult);
extern void areaScrollOnResize(uiArea *, RECT *);
extern void areaScrollTo(uiArea *a, double x, double y, double width, double height);
extern void areaUpdateScroll(uiArea *a);

// areaevents.cpp
extern BOOL areaDoEvents(uiArea *a, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *lResult);
extern BOOL areaDoWheel(uiArea *a, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *lResult);

// areautil.cpp
extern void loadAreaSize(uiArea *a, ID2D1RenderTarget *rt, double *width, double *height);
extern void pixelsToDIPWithRT(ID2D1RenderTarget *rt, double *x, double *y);
extern void pixelsToDIP(uiArea *a, double *x, double *y);

#endif
