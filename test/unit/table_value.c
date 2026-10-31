#include <math.h>
#include "table_test.h"

void tableValueRoundTrips(void **data)
{
	struct tableTestState *state = *data;
	uiTableValue *value;
	double r, g, b, a;

	value = uiNewTableValueString("value");
	assert_int_equal(uiTableValueGetType(value), uiTableValueTypeString);
	assert_string_equal(uiTableValueString(value), "value");
	uiFreeTableValue(value);

	value = uiNewTableValueImage(state->image);
	assert_int_equal(uiTableValueGetType(value), uiTableValueTypeImage);
	assert_ptr_equal(uiTableValueImage(value), state->image);
	uiFreeTableValue(value);

	value = uiNewTableValueInt(42);
	assert_int_equal(uiTableValueGetType(value), uiTableValueTypeInt);
	assert_int_equal(uiTableValueInt(value), 42);
	uiFreeTableValue(value);

	value = uiNewTableValueColor(0.1, 0.2, 0.3, 0.4);
	assert_int_equal(uiTableValueGetType(value), uiTableValueTypeColor);
	uiTableValueColor(value, &r, &g, &b, &a);
	// 0.1/0.2/0.3/0.4 are not exactly representable in binary, and on
	// i386 the x87 FPU keeps intermediates in 80-bit extended precision,
	// so an exact == comparison fails there. Compare with tolerance.
	assert_true(fabs(r - 0.1) < 1e-9);
	assert_true(fabs(g - 0.2) < 1e-9);
	assert_true(fabs(b - 0.3) < 1e-9);
	assert_true(fabs(a - 0.4) < 1e-9);
	uiFreeTableValue(value);
}
