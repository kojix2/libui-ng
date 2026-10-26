#include <float.h>
#include "../ui.h"
#include "uipriv.h"

static int finiteDrawValue(double value)
{
	return value >= -DBL_MAX && value <= DBL_MAX;
}

void uiDrawPathAddRoundedRectangle(uiDrawPath *p, double x, double y, double width, double height, double radius)
{
	double right, bottom;
	double maxRadius;

	if (uiDrawPathEnded(p))
		uiprivUserBug("You cannot modify a uiDrawPath that has been ended. (path: %p)", p);
	if (!finiteDrawValue(x) || !finiteDrawValue(y) ||
		!finiteDrawValue(width) || width < 0 ||
		!finiteDrawValue(height) || height < 0 ||
		!finiteDrawValue(radius) || radius < 0)
		uiprivUserBug("uiDrawPathAddRoundedRectangle() coordinates and dimensions must be finite, and dimensions and radius must be nonnegative.");
	right = x + width;
	bottom = y + height;
	if (!finiteDrawValue(right) || !finiteDrawValue(bottom))
		uiprivUserBug("uiDrawPathAddRoundedRectangle() rectangle endpoints must be finite.");

	maxRadius = (width < height ? width : height) / 2;
	if (radius > maxRadius)
		radius = maxRadius;
	if (radius == 0) {
		uiDrawPathAddRectangle(p, x, y, width, height);
		return;
	}

	uiDrawPathNewFigureWithArc(p, right - radius, y + radius,
		radius, -uiPi / 2, uiPi / 2, 0);
	uiDrawPathArcTo(p, right - radius, bottom - radius,
		radius, 0, uiPi / 2, 0);
	uiDrawPathArcTo(p, x + radius, bottom - radius,
		radius, uiPi / 2, uiPi / 2, 0);
	uiDrawPathArcTo(p, x + radius, y + radius,
		radius, uiPi, uiPi / 2, 0);
	uiDrawPathCloseFigure(p);
}
