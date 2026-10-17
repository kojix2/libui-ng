// 8 september 2015
#include "uipriv_windows.hpp"
#include "area.hpp"
#include <cmath>

struct scrollParams {
	int *pos;
	int pagesize;
	int length;
	int64_t *wheelCarry;
	UINT *lastWheelAmount;
	UINT wheelSPIAction;
};

static void scrollto(uiArea *a, int which, struct scrollParams *p, int64_t pos)
{
	SCROLLINFO si;
	int64_t maxPos = (int64_t) p->length - p->pagesize;

	// note that the pos < 0 check is /after/ the p->length - p->pagesize check
	// it used to be /before/; this was actually a bug in Raymond Chen's original algorithm: if there are fewer than a page's worth of items, p->length - p->pagesize will be negative and our content draw at the bottom of the window
	// this SHOULD have the same effect with that bug fixed and no others introduced... (thanks to devin on irc.badnik.net for confirming this logic)
	if (pos > maxPos)
		pos = maxPos;
	if (pos < 0)
		pos = 0;

	// Direct2D doesn't have a method for scrolling the existing contents of a render target.
	// We'll have to just invalidate everything and hope for the best.
	invalidateRect(a->hwnd, NULL, FALSE);

	*(p->pos) = pos;

	// now commit our new scrollbar setup...
	ZeroMemory(&si, sizeof (SCROLLINFO));
	si.cbSize = sizeof (SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
	si.nPage = p->pagesize;
	si.nMin = 0;
	si.nMax = p->length > 0 ? p->length - 1 : 0;		// endpoint inclusive
	si.nPos = *(p->pos);
	SetScrollInfo(a->hwnd, which, &si, TRUE);
}

static void scrollby(uiArea *a, int which, struct scrollParams *p, int64_t delta)
{
	scrollto(a, which, p, (int64_t) *(p->pos) + delta);
}

static void scroll(uiArea *a, int which, struct scrollParams *p, WPARAM wParam, LPARAM lParam)
{
	int64_t pos;
	SCROLLINFO si;

	pos = *(p->pos);
	switch (LOWORD(wParam)) {
	case SB_LEFT:			// also SB_TOP
		pos = 0;
		break;
	case SB_RIGHT:		// also SB_BOTTOM
		pos = p->length - p->pagesize;
		break;
	case SB_LINELEFT:		// also SB_LINEUP
		pos--;
		break;
	case SB_LINERIGHT:		// also SB_LINEDOWN
		pos++;
		break;
	case SB_PAGELEFT:		// also SB_PAGEUP
		pos -= p->pagesize;
		break;
	case SB_PAGERIGHT:	// also SB_PAGEDOWN
		pos += p->pagesize;
		break;
	case SB_THUMBPOSITION:
		ZeroMemory(&si, sizeof (SCROLLINFO));
		si.cbSize = sizeof (SCROLLINFO);
		si.fMask = SIF_POS;
		if (GetScrollInfo(a->hwnd, which, &si) == 0)
			logLastError(L"error getting thumb position for area");
		pos = si.nPos;
		break;
	case SB_THUMBTRACK:
		ZeroMemory(&si, sizeof (SCROLLINFO));
		si.cbSize = sizeof (SCROLLINFO);
		si.fMask = SIF_TRACKPOS;
		if (GetScrollInfo(a->hwnd, which, &si) == 0)
			logLastError(L"error getting thumb track position for area");
		pos = si.nTrackPos;
		break;
	}
	scrollto(a, which, p, pos);
}

static void wheelscroll(uiArea *a, int which, struct scrollParams *p, WPARAM wParam, LPARAM lParam)
{
	int64_t numerator;
	int64_t lines;
	UINT scrollAmount;

	// This is the Windows default for both vertical lines and horizontal characters.
	scrollAmount = 3;
	if (SystemParametersInfoW(p->wheelSPIAction, 0, &scrollAmount, 0) == 0)
		logLastError(L"error getting area wheel scroll amount");
	if (scrollAmount == WHEEL_PAGESCROLL)
		scrollAmount = p->pagesize;
	if (*p->lastWheelAmount != scrollAmount) {
		*p->wheelCarry = 0;
		*p->lastWheelAmount = scrollAmount;
	}
	if (scrollAmount == 0)		// no mouse wheel scrolling (or pagesize == 0)
		return;
	// the rest of this is basically http://blogs.msdn.com/b/oldnewthing/archive/2003/08/07/54615.aspx and http://blogs.msdn.com/b/oldnewthing/archive/2003/08/11/54624.aspx
	// see those pages for information on subtleties
	numerator = *p->wheelCarry +
		(int64_t) GET_WHEEL_DELTA_WPARAM(wParam) * scrollAmount;
	lines = numerator / WHEEL_DELTA;
	*p->wheelCarry = numerator % WHEEL_DELTA;
	scrollby(a, which, p, which == SB_HORZ ? lines : -lines);
}

static void hscrollParams(uiArea *a, struct scrollParams *p)
{
	RECT r;
	double width;

	ZeroMemory(p, sizeof (struct scrollParams));
	p->pos = &(a->hscrollpos);
	uiWindowsEnsureGetClientRect(a->hwnd, &r);
	width = r.right - r.left;
	pixelsToDIP(a, &width, NULL);
	p->pagesize = (int) width;
	p->length = a->scrollWidth;
	p->wheelCarry = &(a->hwheelCarry);
	p->lastWheelAmount = &(a->hwheelAmount);
	p->wheelSPIAction = SPI_GETWHEELSCROLLCHARS;
}

static void hscrollto(uiArea *a, int pos)
{
	struct scrollParams p;

	hscrollParams(a, &p);
	scrollto(a, SB_HORZ, &p, pos);
}

static void hscrollby(uiArea *a, int delta)
{
	struct scrollParams p;

	hscrollParams(a, &p);
	scrollby(a, SB_HORZ, &p, delta);
}

static void hscroll(uiArea *a, WPARAM wParam, LPARAM lParam)
{
	struct scrollParams p;

	hscrollParams(a, &p);
	scroll(a, SB_HORZ, &p, wParam, lParam);
}

static void hwheelscroll(uiArea *a, WPARAM wParam, LPARAM lParam)
{
	struct scrollParams p;

	hscrollParams(a, &p);
	wheelscroll(a, SB_HORZ, &p, wParam, lParam);
}

static void vscrollParams(uiArea *a, struct scrollParams *p)
{
	RECT r;
	double height;

	ZeroMemory(p, sizeof (struct scrollParams));
	p->pos = &(a->vscrollpos);
	uiWindowsEnsureGetClientRect(a->hwnd, &r);
	height = r.bottom - r.top;
	pixelsToDIP(a, NULL, &height);
	p->pagesize = (int) height;
	p->length = a->scrollHeight;
	p->wheelCarry = &(a->vwheelCarry);
	p->lastWheelAmount = &(a->vwheelAmount);
	p->wheelSPIAction = SPI_GETWHEELSCROLLLINES;
}

static void vscrollto(uiArea *a, int pos)
{
	struct scrollParams p;

	vscrollParams(a, &p);
	scrollto(a, SB_VERT, &p, pos);
}

static void vscrollby(uiArea *a, int delta)
{
	struct scrollParams p;

	vscrollParams(a, &p);
	scrollby(a, SB_VERT, &p, delta);
}

static void vscroll(uiArea *a, WPARAM wParam, LPARAM lParam)
{
	struct scrollParams p;

	vscrollParams(a, &p);
	scroll(a, SB_VERT, &p, wParam, lParam);
}

static void vwheelscroll(uiArea *a, WPARAM wParam, LPARAM lParam)
{
	struct scrollParams p;

	vscrollParams(a, &p);
	wheelscroll(a, SB_VERT, &p, wParam, lParam);
}

BOOL areaDoScroll(uiArea *a, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *lResult)
{
	if (!a->scrolling)
		return FALSE;

	switch (uMsg) {
	case WM_HSCROLL:
		hscroll(a, wParam, lParam);
		*lResult = 0;
		return TRUE;
	case WM_MOUSEHWHEEL:
		hwheelscroll(a, wParam, lParam);
		*lResult = 0;
		return TRUE;
	case WM_VSCROLL:
		vscroll(a, wParam, lParam);
		*lResult = 0;
		return TRUE;
	case WM_MOUSEWHEEL:
		vwheelscroll(a, wParam, lParam);
		*lResult = 0;
		return TRUE;
	}
	return FALSE;
}

void areaScrollOnResize(uiArea *a, RECT *client)
{
	areaUpdateScroll(a);
}

static int scrollToShow(int current, int pagesize, int length, double start, double size)
{
	double end;
	double visibleSize;
	double target;
	int64_t maxPos = (int64_t) length - pagesize;

	if (!std::isfinite(start) || !std::isfinite(size))
		return current;

	end = start + size;
	if (end < start) {
		double temp;

		temp = start;
		start = end;
		end = temp;
	}
	visibleSize = end - start;
	target = current;
	if (visibleSize > pagesize || start < current)
		target = floor(start);
	else if (end > (double) current + pagesize)
		target = ceil(end - pagesize);
	if (maxPos < 0)
		maxPos = 0;
	if (target < 0)
		target = 0;
	if (target > maxPos)
		target = (double) maxPos;
	return (int) target;
}

void areaScrollTo(uiArea *a, double x, double y, double width, double height)
{
	struct scrollParams hp;
	struct scrollParams vp;

	hscrollParams(a, &hp);
	vscrollParams(a, &vp);
	hscrollto(a, scrollToShow(a->hscrollpos, hp.pagesize, hp.length, x, width));
	vscrollto(a, scrollToShow(a->vscrollpos, vp.pagesize, vp.length, y, height));
}

void areaUpdateScroll(uiArea *a)
{
	RECT client;
	LONG_PTR style;
	double width, height, barWidth, barHeight;
	BOOL horizontal, vertical, nextHorizontal, nextVertical;
	int i;

	if (a->updatingScroll)
		return;
	a->updatingScroll = TRUE;
	uiWindowsEnsureGetClientRect(a->hwnd, &client);
	style = GetWindowLongPtrW(a->hwnd, GWL_STYLE);
	width = client.right - client.left;
	height = client.bottom - client.top;
	barWidth = GetSystemMetrics(SM_CXVSCROLL);
	barHeight = GetSystemMetrics(SM_CYHSCROLL);
	if ((style & WS_VSCROLL) != 0)
		width += barWidth;
	if ((style & WS_HSCROLL) != 0)
		height += barHeight;
	pixelsToDIP(a, &width, &height);
	pixelsToDIP(a, &barWidth, &barHeight);
	// Start without either bar and add only those required by the content.
	horizontal = a->scrollWidth > width;
	vertical = a->scrollHeight > height;
	for (i = 0; i < 2; i++) {
		nextHorizontal = a->scrollWidth > width - (vertical ? barWidth : 0);
		nextVertical = a->scrollHeight > height - (horizontal ? barHeight : 0);
		if (nextHorizontal == horizontal && nextVertical == vertical)
			break;
		horizontal = nextHorizontal;
		vertical = nextVertical;
	}
	ShowScrollBar(a->hwnd, SB_HORZ, horizontal);
	ShowScrollBar(a->hwnd, SB_VERT, vertical);
	// The final client rect now reflects the chosen combination of bars.
	hscrollby(a, 0);
	vscrollby(a, 0);
	a->updatingScroll = FALSE;
}
