// 11 october 2015
#include <math.h>
#include "../ui.h"
#include "uipriv.h"

void uiDrawMatrixSetIdentity(uiDrawMatrix *m)
{
	m->M11 = 1;
	m->M12 = 0;
	m->M21 = 0;
	m->M22 = 1;
	m->M31 = 0;
	m->M32 = 0;
}

// The rest of this file provides basic utilities in case the platform doesn't provide any of its own for these tasks.
// Keep these as minimal as possible. They should generally not call other fallbacks.

// see https://msdn.microsoft.com/en-us/library/windows/desktop/ff684171%28v=vs.85%29.aspx#skew_transform
void uiprivFallbackSkew(uiDrawMatrix *m, double x, double y, double xamount, double yamount)
{
	uiDrawMatrix n;
	double xtan, ytan;

	xtan = tan(xamount);
	ytan = tan(yamount);
	uiDrawMatrixSetIdentity(&n);
	// In row-vector form, a skew about (x, y) is
	// T(-x, -y) * S * T(x, y).
	n.M12 = ytan;
	n.M21 = xtan;
	n.M31 = -y * xtan;
	n.M32 = -x * ytan;
	uiDrawMatrixMultiply(m, &n);
}

// the basic algorithm is from cairo
// but it's the same algorithm as the transform point, just without M31 and M32 taken into account, so let's just do that instead
void uiprivFallbackTransformSize(uiDrawMatrix *m, double *x, double *y)
{
	uiDrawMatrix m2;

	m2 = *m;
	m2.M31 = 0;
	m2.M32 = 0;
	uiDrawMatrixTransformPoint(&m2, x, y);
}
