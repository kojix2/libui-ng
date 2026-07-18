#include "unit.h"

struct expectedAttribute {
	uiAttributeType type;
	size_t start;
	size_t end;
	const char *family;
	double r;
	double g;
	double b;
	double a;
};

struct attributeCollector {
	const struct expectedAttribute *expected;
	size_t len;
	size_t n;
};

static uiForEach checkAttribute(const uiAttributedString *s, const uiAttribute *a, size_t start, size_t end, void *data)
{
	struct attributeCollector *c = (struct attributeCollector *) data;
	const struct expectedAttribute *e;
	double r, g, b, alpha;

	(void) s;
	assert_true(c->n < c->len);
	e = &(c->expected[c->n]);
	assert_int_equal(e->type, uiAttributeGetType(a));
	assert_int_equal(e->start, start);
	assert_int_equal(e->end, end);
	if (e->type == uiAttributeTypeFamily)
		assert_string_equal(e->family, uiAttributeFamily(a));
	if (e->type == uiAttributeTypeColor) {
		uiAttributeColor(a, &r, &g, &b, &alpha);
		assert_true(e->r == r);
		assert_true(e->g == g);
		assert_true(e->b == b);
		assert_true(e->a == alpha);
	}
	c->n++;
	return uiForEachContinue;
}

static void assertAttributes(uiAttributedString *s, const struct expectedAttribute *expected, size_t len)
{
	struct attributeCollector c;

	c.expected = expected;
	c.len = len;
	c.n = 0;
	uiAttributedStringForEachAttribute(s, checkAttribute, &c);
	assert_int_equal(len, c.n);
}

static int attrstrSetup(void **state)
{
	uiInitOptions o = {0};

	(void) state;
	assert_null(uiInit(&o));
	return 0;
}

static int attrstrTeardown(void **state)
{
	(void) state;
	uiUninit();
	return 0;
}

static void attrstrSetAttributeDropsLaterOverlap(void **state)
{
	uiAttributedString *s;
	const struct expectedAttribute expected[] = {
		{ uiAttributeTypeColor, 1, 5, NULL, 0.0, 0.0, 1.0, 1.0 },
		{ uiAttributeTypeColor, 5, 6, NULL, 1.0, 0.0, 0.0, 1.0 },
	};

	(void) state;
	s = uiNewAttributedString("abcdef");
	uiAttributedStringSetAttribute(s, uiNewColorAttribute(1.0, 0.0, 0.0, 1.0), 4, 6);
	uiAttributedStringSetAttribute(s, uiNewColorAttribute(0.0, 0.0, 1.0, 1.0), 1, 5);
	assertAttributes(s, expected, sizeof(expected) / sizeof(expected[0]));
	uiFreeAttributedString(s);
}

static void attrstrSetAttributeDropsMultipleOverlaps(void **state)
{
	uiAttributedString *s;
	const struct expectedAttribute expected[] = {
		{ uiAttributeTypeColor, 0, 1, NULL, 1.0, 0.0, 0.0, 1.0 },
		{ uiAttributeTypeColor, 1, 5, NULL, 0.0, 0.0, 1.0, 1.0 },
		{ uiAttributeTypeColor, 5, 6, NULL, 1.0, 0.0, 0.0, 1.0 },
	};

	(void) state;
	s = uiNewAttributedString("abcdef");
	uiAttributedStringSetAttribute(s, uiNewColorAttribute(1.0, 0.0, 0.0, 1.0), 0, 2);
	uiAttributedStringSetAttribute(s, uiNewColorAttribute(1.0, 0.0, 0.0, 1.0), 4, 6);
	uiAttributedStringSetAttribute(s, uiNewColorAttribute(0.0, 0.0, 1.0, 1.0), 1, 5);
	assertAttributes(s, expected, sizeof(expected) / sizeof(expected[0]));
	uiFreeAttributedString(s);
}

static void attrstrSetAttributeMergesAdjacentEqualValues(void **state)
{
	uiAttributedString *s;
	const struct expectedAttribute expected[] = {
		{ uiAttributeTypeColor, 0, 4, NULL, 1.0, 0.0, 0.0, 1.0 },
	};

	(void) state;
	s = uiNewAttributedString("abcd");
	uiAttributedStringSetAttribute(s, uiNewColorAttribute(1.0, 0.0, 0.0, 1.0), 0, 2);
	uiAttributedStringSetAttribute(s, uiNewColorAttribute(1.0, 0.0, 0.0, 1.0), 2, 4);
	assertAttributes(s, expected, sizeof(expected) / sizeof(expected[0]));
	uiFreeAttributedString(s);
}

static void attrstrSetFamilyAttributeMergesAdjacentEqualValues(void **state)
{
	uiAttributedString *s;
	const struct expectedAttribute expected[] = {
		{ uiAttributeTypeFamily, 0, 4, "Arial", 0.0, 0.0, 0.0, 0.0 },
	};

	(void) state;
	s = uiNewAttributedString("abcd");
	uiAttributedStringSetAttribute(s, uiNewFamilyAttribute("Arial"), 0, 2);
	uiAttributedStringSetAttribute(s, uiNewFamilyAttribute("Arial"), 2, 4);
	assertAttributes(s, expected, sizeof(expected) / sizeof(expected[0]));
	uiFreeAttributedString(s);
}

static void attrstrAppendSelf(void **state)
{
	uiAttributedString *s;
	const char *p;

	(void) state;
	s = uiNewAttributedString("abc");
	p = uiAttributedStringString(s);
	uiAttributedStringAppendUnattributed(s, p);
	assert_string_equal(uiAttributedStringString(s), "abcabc");
	uiFreeAttributedString(s);
}

static void attrstrAppendSelfFromInteriorPointer(void **state)
{
	uiAttributedString *s;
	const char *p;

	(void) state;
	s = uiNewAttributedString("abcdef");
	p = uiAttributedStringString(s) + 2;
	uiAttributedStringAppendUnattributed(s, p);
	assert_string_equal(uiAttributedStringString(s), "abcdefcdef");
	uiFreeAttributedString(s);
}

static void attrstrInsertSelfFromInteriorPointer(void **state)
{
	uiAttributedString *s;
	const char *p;

	(void) state;
	s = uiNewAttributedString("abcdef");
	p = uiAttributedStringString(s) + 2;
	uiAttributedStringInsertAtUnattributed(s, p, 3);
	assert_string_equal(uiAttributedStringString(s), "abccdefdef");
	uiFreeAttributedString(s);
}

int attrstrRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(attrstrSetAttributeDropsLaterOverlap,
			attrstrSetup, attrstrTeardown),
		cmocka_unit_test_setup_teardown(attrstrSetAttributeDropsMultipleOverlaps,
			attrstrSetup, attrstrTeardown),
		cmocka_unit_test_setup_teardown(attrstrSetAttributeMergesAdjacentEqualValues,
			attrstrSetup, attrstrTeardown),
		cmocka_unit_test_setup_teardown(attrstrSetFamilyAttributeMergesAdjacentEqualValues,
			attrstrSetup, attrstrTeardown),
		cmocka_unit_test_setup_teardown(attrstrAppendSelf,
			attrstrSetup, attrstrTeardown),
		cmocka_unit_test_setup_teardown(attrstrAppendSelfFromInteriorPointer,
			attrstrSetup, attrstrTeardown),
		cmocka_unit_test_setup_teardown(attrstrInsertSelfFromInteriorPointer,
			attrstrSetup, attrstrTeardown),
	};

	return cmocka_run_group_tests_name("uiAttributedString", tests, NULL, NULL);
}
