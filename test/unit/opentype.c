#include "unit.h"
#include "../../common/uipriv.h"

/* Exercise the portable implementation without a native allocator or GUI.
 * Count live allocations through teardown, including after an assertion fails.
 * Family comparison belongs to the existing backend attrstr tests, not these value round trips. */
static size_t allocations;

void *uiprivAlloc(size_t size, const char *type)
{
	void *p = calloc(1, size);
	assert_non_null(p);
	allocations++;
	return p;
}

void *uiprivRealloc(void *p, size_t size, const char *type)
{
	int wasNull = p == NULL;
	void *out = realloc(p, size);
	assert_non_null(out);
	if (wasNull)
		allocations++;
	return out;
}

void uiprivFree(void *p)
{
	assert_non_null(p);
	assert_true(allocations > 0);
	free(p);
	allocations--;
}

int uiprivStricmp(const char *a, const char *b)
{
	fail_msg("unexpected backend family comparison");
	return 0;
}

void uiprivDoImplBug(const char *file, const char *line, const char *func,
	const char *format, ...)
{
	print_error("unexpected attribute error in %s: %s\n", func, format);
	_fail(file, atoi(line));
}

struct textState {
	uiOpenTypeFeatures *features;
	uiOpenTypeFeatures *clone;
	uiAttribute *attribute;
};

static int textSetup(void **state)
{
	struct textState *s = *state;

	memset(s, 0, sizeof *s);
	s->features = uiNewOpenTypeFeatures();
	return 0;
}

static int textTeardown(void **state)
{
	struct textState *s = *state;

	if (s->attribute != NULL)
		uiFreeAttribute(s->attribute);
	if (s->clone != NULL)
		uiFreeOpenTypeFeatures(s->clone);
	if (s->features != NULL)
		uiFreeOpenTypeFeatures(s->features);
	assert_int_equal(allocations, 0);
	return 0;
}

static void assertFeatureAt(const uiOpenTypeFeatures *features, const char *tag,
	int present, uint32_t expected, const char *file, int line)
{
	uint32_t value = 0x12345678;
	int found = uiOpenTypeFeaturesGet(features,
		tag[0], tag[1], tag[2], tag[3], &value);

	if (!!found != present || value != (present ? expected : 0x12345678)) {
		print_error("%.4s: expected present=%d value=%lu, got present=%d value=%lu\n",
			tag, present, (unsigned long) (present ? expected : 0x12345678),
			!!found, (unsigned long) value);
		_fail(file, line);
	}
}

#define assertFeature(f, tag, present, value) \
	assertFeatureAt(f, tag, present, value, __FILE__, __LINE__)

struct featureCollector {
	const uiOpenTypeFeatures *source;
	unsigned int seen;
	int count;
	uiForEach result;
};

static uiForEach collectFeature(const uiOpenTypeFeatures *features,
	char a, char b, char c, char d, uint32_t value, void *data)
{
	struct featureCollector *collector = data;
	const char tags[][5] = { "kern", "liga", "ss01" };
	const uint32_t values[] = { 0, UINT32_MAX, 7 };
	const char tag[] = { a, b, c, d };
	size_t i;

	assert_ptr_equal(features, collector->source);
	for (i = 0; i < sizeof tags / sizeof *tags; i++)
		if (memcmp(tag, tags[i], 4) == 0)
			break;
	assert_true(i < sizeof tags / sizeof *tags);
	assert_false(collector->seen & (1u << i));
	assert_int_equal(value, values[i]);
	collector->seen |= 1u << i;
	collector->count++;
	return collector->result;
}

static void addSampleFeatures(uiOpenTypeFeatures *features)
{
	/* Insertion order must not become an enumeration-order expectation. */
	uiOpenTypeFeaturesAdd(features, 's', 's', '0', '1', 7);
	uiOpenTypeFeaturesAdd(features, 'l', 'i', 'g', 'a', UINT32_MAX);
	uiOpenTypeFeaturesAdd(features, 'k', 'e', 'r', 'n', 0);
}

static void featuresEmptyAndMissing(void **state)
{
	struct textState *s = *state;
	struct featureCollector collector = { s->features, 0, 0, uiForEachContinue };

	assertFeature(s->features, "kern", 0, 0);
	uiOpenTypeFeaturesRemove(s->features, 'k', 'e', 'r', 'n');
	uiOpenTypeFeaturesForEach(s->features, collectFeature, &collector);
	assert_int_equal(collector.count, 0);
	s->clone = uiOpenTypeFeaturesClone(s->features);
	assert_ptr_not_equal(s->clone, s->features);
	assertFeature(s->clone, "kern", 0, 0);
}

static void featuresReplaceAndEnumerate(void **state)
{
	struct textState *s = *state;
	struct featureCollector collector = { s->features, 0, 0, uiForEachContinue };

	addSampleFeatures(s->features);
	uiOpenTypeFeaturesAdd(s->features, 'k', 'e', 'r', 'n', UINT32_MAX);
	assertFeature(s->features, "kern", 1, UINT32_MAX);
	uiOpenTypeFeaturesAdd(s->features, 'k', 'e', 'r', 'n', 0);
	assertFeature(s->features, "kern", 1, 0);
	assertFeature(s->features, "liga", 1, UINT32_MAX);
	assertFeature(s->features, "ss01", 1, 7);
	assertFeature(s->features, "ss02", 0, 0);
	uiOpenTypeFeaturesForEach(s->features, collectFeature, &collector);
	assert_int_equal(collector.count, 3);
	assert_int_equal(collector.seen, 7);
}

static void featuresRemoveAfterGrowth(void **state)
{
	struct textState *s = *state;
	int i;

	/* Twenty real stylistic-set tags cross the current initial capacity. */
	for (i = 20; i >= 1; i--)
		uiOpenTypeFeaturesAdd(s->features, 's', 's',
			(char) ('0' + i / 10), (char) ('0' + i % 10), (uint32_t) i);
	uiOpenTypeFeaturesRemove(s->features, 's', 's', '0', '1');
	uiOpenTypeFeaturesRemove(s->features, 's', 's', '1', '0');
	uiOpenTypeFeaturesRemove(s->features, 's', 's', '2', '0');
	uiOpenTypeFeaturesRemove(s->features, 's', 's', '1', '0');
	for (i = 1; i <= 20; i++) {
		char tag[] = { 's', 's', (char) ('0' + i / 10), (char) ('0' + i % 10), 0 };
		assertFeature(s->features, tag, i != 1 && i != 10 && i != 20, (uint32_t) i);
	}
	uiOpenTypeFeaturesAdd(s->features, 's', 's', '1', '0', UINT32_MAX);
	assertFeature(s->features, "ss10", 1, UINT32_MAX);
}

static void featuresCloneIsIndependent(void **state)
{
	struct textState *s = *state;

	addSampleFeatures(s->features);
	s->clone = uiOpenTypeFeaturesClone(s->features);
	assert_ptr_not_equal(s->clone, s->features);
	uiOpenTypeFeaturesAdd(s->features, 'k', 'e', 'r', 'n', 42);
	uiOpenTypeFeaturesRemove(s->clone, 'l', 'i', 'g', 'a');
	assertFeature(s->features, "liga", 1, UINT32_MAX);
	assertFeature(s->features, "kern", 1, 42);
	assertFeature(s->clone, "liga", 0, 0);
	assertFeature(s->clone, "kern", 1, 0);
	uiFreeOpenTypeFeatures(s->features);
	s->features = NULL;
	assertFeature(s->clone, "ss01", 1, 7);
	uiOpenTypeFeaturesAdd(s->clone, 'k', 'e', 'r', 'n', 9);
	assertFeature(s->clone, "kern", 1, 9);
}

static void featuresEnumerationStops(void **state)
{
	struct textState *s = *state;
	struct featureCollector collector = { s->features, 0, 0, uiForEachStop };

	addSampleFeatures(s->features);
	uiOpenTypeFeaturesForEach(s->features, collectFeature, &collector);
	assert_int_equal(collector.count, 1);
	/* Early termination does not remove any feature. */
	assertFeature(s->features, "kern", 1, 0);
	assertFeature(s->features, "liga", 1, UINT32_MAX);
	assertFeature(s->features, "ss01", 1, 7);
}

static void freeTestAttribute(struct textState *s)
{
	uiFreeAttribute(s->attribute);
	s->attribute = NULL;
}

static void attributeFontValues(void **state)
{
	struct textState *s = *state;
	char family[] = "Example Sans";
	const uiTextWeight weights[] = { uiTextWeightMinimum, (uiTextWeight) 473, uiTextWeightMaximum };
	size_t i;
	int value;

	s->attribute = uiNewFamilyAttribute(family);
	family[0] = 'X';
	assert_int_equal(uiAttributeGetType(s->attribute), uiAttributeTypeFamily);
	assert_string_equal(uiAttributeFamily(s->attribute), "Example Sans");
	freeTestAttribute(s);
	s->attribute = uiNewSizeAttribute(12.5);
	assert_int_equal(uiAttributeGetType(s->attribute), uiAttributeTypeSize);
	assert_float_equal(uiAttributeSize(s->attribute), 12.5, 1e-12);
	freeTestAttribute(s);
	for (i = 0; i < sizeof weights / sizeof *weights; i++) {
		s->attribute = uiNewWeightAttribute(weights[i]);
		assert_int_equal(uiAttributeGetType(s->attribute), uiAttributeTypeWeight);
		assert_int_equal(uiAttributeWeight(s->attribute), weights[i]);
		freeTestAttribute(s);
	}
	for (value = uiTextItalicNormal; value <= uiTextItalicItalic; value++) {
		s->attribute = uiNewItalicAttribute((uiTextItalic) value);
		assert_int_equal(uiAttributeGetType(s->attribute), uiAttributeTypeItalic);
		assert_int_equal(uiAttributeItalic(s->attribute), value);
		freeTestAttribute(s);
	}
	for (value = uiTextStretchUltraCondensed; value <= uiTextStretchUltraExpanded; value++) {
		s->attribute = uiNewStretchAttribute((uiTextStretch) value);
		assert_int_equal(uiAttributeGetType(s->attribute), uiAttributeTypeStretch);
		assert_int_equal(uiAttributeStretch(s->attribute), value);
		freeTestAttribute(s);
	}
}

static void attributeColorsAndUnderlines(void **state)
{
	struct textState *s = *state;
	uiUnderlineColor color;
	double r, g, b, a;
	int value;

	s->attribute = uiNewColorAttribute(0, 0.25, 0.5, 1);
	assert_int_equal(uiAttributeGetType(s->attribute), uiAttributeTypeColor);
	uiAttributeColor(s->attribute, &r, &g, &b, &a);
	assert_float_equal(r, 0, 1e-12);
	assert_float_equal(g, 0.25, 1e-12);
	assert_float_equal(b, 0.5, 1e-12);
	assert_float_equal(a, 1, 1e-12);
	freeTestAttribute(s);
	/* There is no documented public background-color getter. */
	s->attribute = uiNewBackgroundAttribute(1, 0.5, 0.25, 0);
	assert_int_equal(uiAttributeGetType(s->attribute), uiAttributeTypeBackground);
	freeTestAttribute(s);
	for (value = uiUnderlineNone; value <= uiUnderlineSuggestion; value++) {
		s->attribute = uiNewUnderlineAttribute((uiUnderline) value);
		assert_int_equal(uiAttributeGetType(s->attribute), uiAttributeTypeUnderline);
		assert_int_equal(uiAttributeUnderline(s->attribute), value);
		freeTestAttribute(s);
	}
	for (value = uiUnderlineColorCustom; value <= uiUnderlineColorAuxiliary; value++) {
		double red = value == uiUnderlineColorCustom ? 1 : 0;
		double alpha = value == uiUnderlineColorCustom ? 0.5 : 0;

		s->attribute = uiNewUnderlineColorAttribute((uiUnderlineColor) value, red, 0, 0, alpha);
		assert_int_equal(uiAttributeGetType(s->attribute), uiAttributeTypeUnderlineColor);
		uiAttributeUnderlineColor(s->attribute, &color, &r, &g, &b, &a);
		assert_int_equal(color, value);
		assert_float_equal(r, red, 1e-12);
		assert_float_equal(g, 0, 1e-12);
		assert_float_equal(b, 0, 1e-12);
		assert_float_equal(a, alpha, 1e-12);
		freeTestAttribute(s);
	}
}

static void attributeFeaturesOwnCopy(void **state)
{
	struct textState *s = *state;
	const uiOpenTypeFeatures *borrowed;

	addSampleFeatures(s->features);
	s->attribute = uiNewFeaturesAttribute(s->features);
	assert_int_equal(uiAttributeGetType(s->attribute), uiAttributeTypeFeatures);
	borrowed = uiAttributeFeatures(s->attribute);
	assert_ptr_not_equal(borrowed, s->features);
	uiOpenTypeFeaturesAdd(s->features, 'k', 'e', 'r', 'n', 42);
	uiOpenTypeFeaturesRemove(s->features, 'l', 'i', 'g', 'a');
	uiFreeOpenTypeFeatures(s->features);
	s->features = NULL;
	assertFeature(borrowed, "kern", 1, 0);
	assertFeature(borrowed, "liga", 1, UINT32_MAX);
	assertFeature(borrowed, "ss01", 1, 7);
}

int main(void)
{
	struct textState state;
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_prestate_setup_teardown(featuresEmptyAndMissing, textSetup, textTeardown, &state),
		cmocka_unit_test_prestate_setup_teardown(featuresReplaceAndEnumerate, textSetup, textTeardown, &state),
		cmocka_unit_test_prestate_setup_teardown(featuresRemoveAfterGrowth, textSetup, textTeardown, &state),
		cmocka_unit_test_prestate_setup_teardown(featuresCloneIsIndependent, textSetup, textTeardown, &state),
		cmocka_unit_test_prestate_setup_teardown(featuresEnumerationStops, textSetup, textTeardown, &state),
		cmocka_unit_test_prestate_setup_teardown(attributeFontValues, textSetup, textTeardown, &state),
		cmocka_unit_test_prestate_setup_teardown(attributeColorsAndUnderlines, textSetup, textTeardown, &state),
		cmocka_unit_test_prestate_setup_teardown(attributeFeaturesOwnCopy, textSetup, textTeardown, &state),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
