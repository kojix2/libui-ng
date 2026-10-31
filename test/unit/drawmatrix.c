#include <math.h>
#include "unit.h"

#define EPSILON 0.00001

static void assertMatrixEqualAt(const uiDrawMatrix *expected,
	const uiDrawMatrix *actual, const char *file, int line)
{
	const char *fields[] = {"M11", "M12", "M21", "M22", "M31", "M32"};
	const double a[] = {expected->M11, expected->M12, expected->M21,
		expected->M22, expected->M31, expected->M32};
	const double b[] = {actual->M11, actual->M12, actual->M21,
		actual->M22, actual->M31, actual->M32};
	int equal = 1;

	for (size_t i = 0; i < sizeof a / sizeof *a; i++) {
		double scale = fabs(a[i]);
		if (fabs(b[i]) > scale)
			scale = fabs(b[i]);
		if (scale < 1.0)
			scale = 1.0;
		if (!(fabs(a[i] - b[i]) <= EPSILON * scale)) {
			print_error("%s: expected %.17g, got %.17g\n", fields[i], a[i], b[i]);
			equal = 0;
		}
	}
	if (!equal)
		_fail(file, line);
}

#define assertMatrixEqual(a, b) \
	assertMatrixEqualAt((a), (b), __FILE__, __LINE__)

static int drawMatrixTestSetup(void **state)
{
	uiDrawMatrix *m = *state;
	uiDrawMatrixSetIdentity(m);
	return 0;
}

static void drawMatrixIdentity(void **state)
{
	uiDrawMatrix *m = *state;
	uiDrawMatrix expected = {
		1.0, 0.0,
		0.0, 1.0,
		0.0, 0.0
	};

	assertMatrixEqual(&expected, m);
}

static void drawMatrixTranslate(void **state)
{
	uiDrawMatrix *m = *state;
	double tx = 0.5;
	double ty = 0.25;
	uiDrawMatrixTranslate(m, tx, ty);

	uiDrawMatrix expected = {
		1.0, 0.0,
		0.0, 1.0,
		tx, ty
	};

	assertMatrixEqual(&expected, m);
}

static void drawMatrixScale(void **state)
{
	uiDrawMatrix *m = *state;
	double cx = 0.25;
	double cy = 0.125;
	double sx = 0.5;
	double sy = 0.25;
	uiDrawMatrixScale(m, cx, cy, sx, sy);

	uiDrawMatrix expected = {
		sx, 0.0,
		0.0, sy,
		-cx * sx + cx,
		-cy * sy + cy
	};

	assertMatrixEqual(&expected, m);
}

#define THETA uiPi / 6
#define COS 1.73205080757 / 2
#define SIN 0.5

static void drawMatrixRotate(void **state)
{
	uiDrawMatrix *m = *state;
	double cx = 0.25;
	double cy = 0.125;
	uiDrawMatrixRotate(m, cx, cy, THETA);

	uiDrawMatrix expected = {
		COS, SIN,
		-SIN, COS,
		-cx * COS + cy * SIN + cx,
		-cx * SIN - cy * COS + cy
	};

	assertMatrixEqual(&expected, m);
}

static void drawMatrixSkew(void **state)
{
	uiDrawMatrix *m = *state;
	double cx = 0.25;
	double cy = 0.125;
	uiDrawMatrix expected = {
		1.0, -1.0,
		1.0, 1.0,
		-cy, cx
	};

	uiDrawMatrixSkew(m, cx, cy, uiPi / 4, -uiPi / 4);
	assertMatrixEqual(&expected, m);
}

static void drawMatrixTRS(void **state)
{
	uiDrawMatrix *m = *state;

	double tx = 0.5;
	double ty = 0.25;
	double sx = 0.3;
	double sy = 0.1;
	uiDrawMatrixTranslate(m, tx, ty);
	uiDrawMatrixRotate(m, tx, ty, THETA);
	uiDrawMatrixScale(m, tx, ty, sx, sy);

	uiDrawMatrix expected = {
		COS * sx, SIN * sy,
		-SIN * sx, COS * sy,
		tx, ty,
	};

	assertMatrixEqual(&expected, m);
}

static void drawMatrixMultiply(void **state)
{
	// Test the same transform as drawMatrixTRS with uiDrawMatrixMultiply
	uiDrawMatrix *m = *state;

	uiDrawMatrix t;
	double tx = 0.5;
	double ty = 0.25;
	uiDrawMatrixSetIdentity(&t);
	uiDrawMatrixTranslate(&t, tx, ty);

	uiDrawMatrix r;
	uiDrawMatrixSetIdentity(&r);
	uiDrawMatrixRotate(&r, tx, ty, THETA);

	uiDrawMatrix s;
	double sx = 0.3;
	double sy = 0.1;
	uiDrawMatrixSetIdentity(&s);
	uiDrawMatrixScale(&s, tx, ty, sx, sy);

	uiDrawMatrixMultiply(m, &t);
	uiDrawMatrixMultiply(m, &r);
	uiDrawMatrixMultiply(m, &s);

	uiDrawMatrix expected = {
		COS * sx, SIN * sy,
		-SIN * sx, COS * sy,
		tx, ty,
	};

	assertMatrixEqual(&expected, m);
}

/* Integer inputs keep these expected coordinates exact on every backend. */
static void drawMatrixTransformPoint(void **state)
{
	uiDrawMatrix *m = *state;
	uiDrawMatrix transform = {2, 3, 5, 7, 11, 13};
	double x = 17, y = 19;

	*m = transform;
	uiDrawMatrixTransformPoint(m, &x, &y);
	if (x != 140 || y != 197)
		fail_msg("point (17, 19): expected (140, 197), got (%.17g, %.17g)", x, y);
	assertMatrixEqual(&transform, m);
}

static void drawMatrixTransformSize(void **state)
{
	uiDrawMatrix *m = *state;
	uiDrawMatrix transform = {2, 3, 5, 7, 11, 13};
	double x = 17, y = 19;

	*m = transform;
	uiDrawMatrixTransformSize(m, &x, &y);
	if (x != 129 || y != 184)
		fail_msg("size (17, 19): expected (129, 184), got (%.17g, %.17g)", x, y);
	assertMatrixEqual(&transform, m);
}

static void drawMatrixInvertIdentity(void **state)
{
	uiDrawMatrix *m = *state;
	uiDrawMatrix expected = {
		1.0, 0.0,
		0.0, 1.0,
		0.0, 0.0
	};

	assert_true(uiDrawMatrixInvertible(m));
	assert_true(uiDrawMatrixInvert(m));
	assertMatrixEqual(&expected, m);
}

static void drawMatrixInvertSelfInverse(void **state)
{
	uiDrawMatrix *m = *state;
	uiDrawMatrix expected = {
		-1.0, 0.0,
		0.0, -1.0,
		0.0, 0.0
	};

	*m = expected;
	assert_true(uiDrawMatrixInvertible(m));
	assert_true(uiDrawMatrixInvert(m));
	assertMatrixEqual(&expected, m);
}

static void drawMatrixInvertSingular(void **state)
{
	uiDrawMatrix *m = *state;
	uiDrawMatrix expected = {
		1.0, 0.0,
		0.0, 0.0,
		2.0, 3.0
	};

	*m = expected;
	assert_false(uiDrawMatrixInvertible(m));
	assert_false(uiDrawMatrixInvert(m));
	assertMatrixEqual(&expected, m);
}

#define drawMatrixUnitTest(f, matrix) \
	cmocka_unit_test_prestate_setup_teardown((f), drawMatrixTestSetup, NULL, (matrix))

int drawMatrixRunUnitTests(void)
{
	uiDrawMatrix matrix;
	const struct CMUnitTest tests[] = {
		drawMatrixUnitTest(drawMatrixIdentity, &matrix),
		drawMatrixUnitTest(drawMatrixTranslate, &matrix),
		drawMatrixUnitTest(drawMatrixScale, &matrix),
		drawMatrixUnitTest(drawMatrixRotate, &matrix),
		drawMatrixUnitTest(drawMatrixSkew, &matrix),
		drawMatrixUnitTest(drawMatrixTRS, &matrix),
		drawMatrixUnitTest(drawMatrixMultiply, &matrix),
		drawMatrixUnitTest(drawMatrixTransformPoint, &matrix),
		drawMatrixUnitTest(drawMatrixTransformSize, &matrix),
		drawMatrixUnitTest(drawMatrixInvertIdentity, &matrix),
		drawMatrixUnitTest(drawMatrixInvertSelfInverse, &matrix),
		drawMatrixUnitTest(drawMatrixInvertSingular, &matrix),
	};

	return cmocka_run_group_tests_name("uiDrawMatrix", tests, NULL, NULL);
}
