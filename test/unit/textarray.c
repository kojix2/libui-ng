#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../../ui.h"
#include "../../common/uipriv.h"

static size_t allocations;
static size_t freedArrays;
static size_t freedStrings;

void *uiprivAlloc(size_t size, const char *type)
{
	void *p;

	(void) type;
	allocations++;
	p = calloc(1, size);
	assert_non_null(p);
	return p;
}

void uiprivFree(void *p)
{
	assert_non_null(p);
	freedArrays++;
	free(p);
}

void uiFreeText(char *text)
{
	assert_non_null(text);
	freedStrings++;
	free(text);
}

static char *copyText(const char *text)
{
	size_t length = strlen(text) + 1;
	char *copy = malloc(length);

	assert_non_null(copy);
	memcpy(copy, text, length);
	return copy;
}

static void resetCounters(void)
{
	allocations = 0;
	freedArrays = 0;
	freedStrings = 0;
}

static void nullAndEmptyArray(void **state)
{
	char **texts;

	(void) state;
	resetCounters();
	uiFreeTextArray(NULL);
	assert_int_equal(freedArrays, 0);
	texts = uiprivNewTextArray(0);
	assert_non_null(texts);
	assert_null(texts[0]);
	uiFreeTextArray(texts);
	assert_int_equal(allocations, 1);
	assert_int_equal(freedArrays, 1);
	assert_int_equal(freedStrings, 0);
}

static void completeAndPartialArrays(void **state)
{
	char **texts;

	(void) state;
	resetCounters();
	texts = uiprivNewTextArray(3);
	assert_non_null(texts);
	assert_null(texts[0]);
	assert_null(texts[1]);
	assert_null(texts[2]);
	assert_null(texts[3]);
	texts[0] = copyText("");
	texts[1] = copyText("first");
	texts[2] = copyText("second");
	uiFreeTextArray(texts);
	assert_int_equal(freedStrings, 3);
	assert_int_equal(freedArrays, 1);
	texts = uiprivNewTextArray(3);
	texts[0] = copyText("partial");
	uiFreeTextArray(texts);
	assert_int_equal(freedStrings, 4);
	assert_int_equal(freedArrays, 2);
}

static void sizeOverflowIsRejected(void **state)
{
	(void) state;
	resetCounters();
	assert_null(uiprivNewTextArray(SIZE_MAX));
	assert_null(uiprivNewTextArray(SIZE_MAX / sizeof(char *)));
	assert_int_equal(allocations, 0);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(nullAndEmptyArray),
		cmocka_unit_test(completeAndPartialArrays),
		cmocka_unit_test(sizeOverflowIsRejected),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
