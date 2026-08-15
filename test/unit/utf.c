#include "unit.h"
#include "../../common/utf.h"

#define replacementRune 0xFFFD

struct utf8Case {
	uint32_t rune;
	uint32_t decoded;
	size_t len;
	unsigned char encoded[4];
};

static void utf8EncodeDecode(void **state)
{
	const struct utf8Case cases[] = {
		{ 0x000000, 0x000000, 1, { 0x00 } },
		{ 0x00007F, 0x00007F, 1, { 0x7F } },
		{ 0x000080, 0x000080, 2, { 0xC2, 0x80 } },
		{ 0x0007FF, 0x0007FF, 2, { 0xDF, 0xBF } },
		{ 0x000800, 0x000800, 3, { 0xE0, 0xA0, 0x80 } },
		{ 0x00D7FF, 0x00D7FF, 3, { 0xED, 0x9F, 0xBF } },
		{ 0x00D800, replacementRune, 3, { 0xEF, 0xBF, 0xBD } },
		{ 0x00E000, 0x00E000, 3, { 0xEE, 0x80, 0x80 } },
		{ 0x00FFFF, 0x00FFFF, 3, { 0xEF, 0xBF, 0xBF } },
		{ 0x010000, 0x010000, 4, { 0xF0, 0x90, 0x80, 0x80 } },
		{ 0x10FFFF, 0x10FFFF, 4, { 0xF4, 0x8F, 0xBF, 0xBF } },
		{ 0x110000, replacementRune, 3, { 0xEF, 0xBF, 0xBD } },
	};
	size_t i;

	(void) state;
	for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		char encoded[4];
		const char *next;
		uint32_t decoded;
		size_t len;

		len = uiprivUTF8EncodeRune(cases[i].rune, encoded);
		assert_int_equal(len, cases[i].len);
		assert_memory_equal(encoded, cases[i].encoded, len);
		next = uiprivUTF8DecodeRune(encoded, len, &decoded);
		assert_ptr_equal(next, encoded + len);
		assert_int_equal(decoded, cases[i].decoded);
	}
}

struct invalidUTF8Case {
	size_t len;
	unsigned char encoded[4];
};

static void utf8DecodeInvalid(void **state)
{
	const struct invalidUTF8Case cases[] = {
		{ 1, { 0x80 } },
		{ 2, { 0xC0, 0x80 } },
		{ 3, { 0xE0, 0x80, 0x80 } },
		{ 3, { 0xED, 0xA0, 0x80 } },
		{ 3, { 0xE2, 0x28, 0xA1 } },
		{ 3, { 0xF0, 0x90, 0x80 } },
		{ 4, { 0xF4, 0x90, 0x80, 0x80 } },
	};
	size_t i;

	(void) state;
	for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		const char *encoded;
		const char *next;
		uint32_t rune;

		encoded = (const char *) cases[i].encoded;
		next = uiprivUTF8DecodeRune(encoded, cases[i].len, &rune);
		assert_ptr_equal(next, encoded + 1);
		assert_int_equal(rune, replacementRune);
	}
}

struct utf16Case {
	uint32_t rune;
	uint32_t decoded;
	size_t len;
	uint16_t encoded[2];
};

static void utf16EncodeDecode(void **state)
{
	const struct utf16Case cases[] = {
		{ 0x000000, 0x000000, 1, { 0x0000 } },
		{ 0x00D7FF, 0x00D7FF, 1, { 0xD7FF } },
		{ 0x00D800, replacementRune, 1, { 0xFFFD } },
		{ 0x00E000, 0x00E000, 1, { 0xE000 } },
		{ 0x00FFFF, 0x00FFFF, 1, { 0xFFFF } },
		{ 0x010000, 0x010000, 2, { 0xD800, 0xDC00 } },
		{ 0x10FFFF, 0x10FFFF, 2, { 0xDBFF, 0xDFFF } },
		{ 0x110000, replacementRune, 1, { 0xFFFD } },
	};
	size_t i;

	(void) state;
	for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		uint16_t encoded[2];
		const uint16_t *next;
		uint32_t decoded;
		size_t len;

		len = uiprivUTF16EncodeRune(cases[i].rune, encoded);
		assert_int_equal(len, cases[i].len);
		assert_memory_equal(encoded, cases[i].encoded,
			len * sizeof(encoded[0]));
		next = uiprivUTF16DecodeRune(encoded, len, &decoded);
		assert_ptr_equal(next, encoded + len);
		assert_int_equal(decoded, cases[i].decoded);
	}
}

static void utf16DecodeInvalid(void **state)
{
	const uint16_t lowFirst[] = { 0xDC00 };
	const uint16_t truncated[] = { 0xD800 };
	const uint16_t badPair[] = { 0xD800, 0x0041 };
	const uint16_t *next;
	uint32_t rune;

	(void) state;
	next = uiprivUTF16DecodeRune(lowFirst, 1, &rune);
	assert_ptr_equal(next, lowFirst + 1);
	assert_int_equal(rune, replacementRune);
	next = uiprivUTF16DecodeRune(truncated, 1, &rune);
	assert_ptr_equal(next, truncated + 1);
	assert_int_equal(rune, replacementRune);
	next = uiprivUTF16DecodeRune(badPair, 2, &rune);
	assert_ptr_equal(next, badPair + 1);
	assert_int_equal(rune, replacementRune);
}

static void utfRoundTripAllRunes(void **state)
{
	uint32_t rune;

	(void) state;
	for (rune = 0; rune <= 0x10FFFF; rune++) {
		char utf8[4];
		uint16_t utf16[2];
		uint32_t decoded, expected;
		size_t len;

		expected = rune;
		if (rune >= 0xD800 && rune < 0xE000)
			expected = replacementRune;

		len = uiprivUTF8EncodeRune(rune, utf8);
		if (uiprivUTF8DecodeRune(utf8, len, &decoded) != utf8 + len ||
			decoded != expected)
			fail_msg("UTF-8 round trip failed for U+%06X",
				(unsigned int) rune);

		len = uiprivUTF16EncodeRune(rune, utf16);
		if (uiprivUTF16DecodeRune(utf16, len, &decoded) != utf16 + len ||
			decoded != expected)
			fail_msg("UTF-16 round trip failed for U+%06X",
				(unsigned int) rune);
	}
}

static void utfCounts(void **state)
{
	const char utf8[] = "A\xC3\xA9\xF0\x9F\x98\x80";
	const char boundedUTF8[] = { 0, 'A' };
	const uint16_t utf16[] = { 'A', 0x00E9, 0xD83D, 0xDE00, 0 };
	const uint16_t boundedUTF16[] = { 0, 'A' };

	(void) state;
	assert_int_equal(uiprivUTF8UTF16Count(utf8, 0), 4);
	assert_int_equal(uiprivUTF8UTF16Count(boundedUTF8, 2), 2);
	assert_int_equal(uiprivUTF16UTF8Count(utf16, 0), 7);
	assert_int_equal(uiprivUTF16UTF8Count(boundedUTF16, 2), 2);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(utf8EncodeDecode),
		cmocka_unit_test(utf8DecodeInvalid),
		cmocka_unit_test(utf16EncodeDecode),
		cmocka_unit_test(utf16DecodeInvalid),
		cmocka_unit_test(utfRoundTripAllRunes),
		cmocka_unit_test(utfCounts),
	};

	return cmocka_run_group_tests_name("UTF", tests, NULL, NULL);
}
