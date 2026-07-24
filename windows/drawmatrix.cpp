// 7 september 2015
#include "uipriv_windows.hpp"
#include "draw.hpp"

void m2d(uiDrawMatrix *m, D2D1_MATRIX_3X2_F *d)
{
	d->_11 = uiprivD2DFloat(m->M11);
	d->_12 = uiprivD2DFloat(m->M12);
	d->_21 = uiprivD2DFloat(m->M21);
	d->_22 = uiprivD2DFloat(m->M22);
	d->_31 = uiprivD2DFloat(m->M31);
	d->_32 = uiprivD2DFloat(m->M32);
}

static void d2m(D2D1_MATRIX_3X2_F *d, uiDrawMatrix *m)
{
	m->M11 = d->_11;
	m->M12 = d->_12;
	m->M21 = d->_21;
	m->M22 = d->_22;
	m->M31 = d->_31;
	m->M32 = d->_32;
}

void uiDrawMatrixTranslate(uiDrawMatrix *m, double x, double y)
{
	D2D1_MATRIX_3X2_F dm;

	m2d(m, &dm);
	dm = dm * D2D1::Matrix3x2F::Translation(uiprivD2DFloat(x), uiprivD2DFloat(y));
	d2m(&dm, m);
}

void uiDrawMatrixScale(uiDrawMatrix *m, double xCenter, double yCenter, double x, double y)
{
	D2D1_MATRIX_3X2_F dm;
	D2D1_POINT_2F center;

	m2d(m, &dm);
	center.x = uiprivD2DFloat(xCenter);
	center.y = uiprivD2DFloat(yCenter);
	dm = dm * D2D1::Matrix3x2F::Scale(uiprivD2DFloat(x), uiprivD2DFloat(y), center);
	d2m(&dm, m);
}

#define r2d(x) (x * (180.0 / uiPi))

void uiDrawMatrixRotate(uiDrawMatrix *m, double x, double y, double amount)
{
	D2D1_MATRIX_3X2_F dm;
	D2D1_POINT_2F center;

	m2d(m, &dm);
	center.x = uiprivD2DFloat(x);
	center.y = uiprivD2DFloat(y);
	dm = dm * D2D1::Matrix3x2F::Rotation(uiprivD2DFloat(r2d(amount)), center);
	d2m(&dm, m);
}

void uiDrawMatrixSkew(uiDrawMatrix *m, double x, double y, double xamount, double yamount)
{
	D2D1_MATRIX_3X2_F dm;
	D2D1_POINT_2F center;

	m2d(m, &dm);
	center.x = uiprivD2DFloat(x);
	center.y = uiprivD2DFloat(y);
	dm = dm * D2D1::Matrix3x2F::Skew(uiprivD2DFloat(r2d(xamount)), uiprivD2DFloat(r2d(yamount)), center);
	d2m(&dm, m);
}

void uiDrawMatrixMultiply(uiDrawMatrix *dest, uiDrawMatrix *src)
{
	D2D1_MATRIX_3X2_F c, d;

	m2d(dest, &c);
	m2d(src, &d);
	c = c * d;
	d2m(&c, dest);
}

int uiDrawMatrixInvertible(uiDrawMatrix *m)
{
	D2D1_MATRIX_3X2_F d;

	m2d(m, &d);
	return D2D1IsMatrixInvertible(&d) != FALSE;
}

int uiDrawMatrixInvert(uiDrawMatrix *m)
{
	D2D1_MATRIX_3X2_F d;

	m2d(m, &d);
	if (D2D1InvertMatrix(&d) == FALSE)
		return 0;
	d2m(&d, m);
	return 1;
}

void uiDrawMatrixTransformPoint(uiDrawMatrix *m, double *x, double *y)
{
	D2D1::Matrix3x2F dm;
	D2D1_POINT_2F pt;

	m2d(m, &dm);
	pt.x = uiprivD2DFloat(*x);
	pt.y = uiprivD2DFloat(*y);
	pt = dm.TransformPoint(pt);
	*x = pt.x;
	*y = pt.y;
}

void uiDrawMatrixTransformSize(uiDrawMatrix *m, double *x, double *y)
{
	uiprivFallbackTransformSize(m, x, y);
}
