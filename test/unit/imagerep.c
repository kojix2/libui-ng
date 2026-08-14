#include "unit.h"
#include "../../common/uipriv.h"

struct imageSize {
	int width;
	int height;
};

static struct imageSize chooseRep(int targetWidth, int targetHeight,
	const struct imageSize *reps, size_t n)
{
	uiprivImageRepMatcher matcher;
	struct imageSize best = { 0, 0 };
	size_t i;

	uiprivImageRepMatcherInit(&matcher, targetWidth, targetHeight);
	for (i = 0; i < n; i++)
		if (uiprivImageRepMatcherAdd(&matcher, reps[i].width, reps[i].height))
			best = reps[i];
	return best;
}

static void assertImageSize(struct imageSize actual, int width, int height)
{
	assert_int_equal(actual.width, width);
	assert_int_equal(actual.height, height);
}

static void imageRepSelectsExactDestinationSize(void **state)
{
	const struct imageSize reps[] = { { 16, 16 }, { 64, 64 } };

	(void) state;
	assertImageSize(chooseRep(64, 64, reps, 2), 64, 64);
}

static void imageRepSelectsClosestLargerRepresentation(void **state)
{
	const struct imageSize reps[] = {
		{ 16, 16 }, { 64, 64 }, { 128, 128 },
	};

	(void) state;
	assertImageSize(chooseRep(48, 48, reps, 3), 64, 64);
}

static void imageRepFallsBackToClosestSmallerRepresentation(void **state)
{
	const struct imageSize reps[] = {
		{ 16, 16 }, { 64, 64 }, { 128, 128 },
	};

	(void) state;
	assertImageSize(chooseRep(256, 256, reps, 3), 128, 128);
}

static void imageRepHandlesNonUniformDestination(void **state)
{
	const struct imageSize reps[] = {
		{ 16, 16 }, { 32, 32 }, { 64, 64 },
	};

	(void) state;
	assertImageSize(chooseRep(64, 32, reps, 3), 64, 64);
}

static void imageRepSelectionDoesNotDependOnAppendOrder(void **state)
{
	const struct imageSize ascending[] = {
		{ 16, 16 }, { 64, 64 }, { 128, 128 },
	};
	const struct imageSize descending[] = {
		{ 128, 128 }, { 64, 64 }, { 16, 16 },
	};

	(void) state;
	assertImageSize(chooseRep(48, 48, ascending, 3), 64, 64);
	assertImageSize(chooseRep(48, 48, descending, 3), 64, 64);
}

int imageRepRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(imageRepSelectsExactDestinationSize),
		cmocka_unit_test(imageRepSelectsClosestLargerRepresentation),
		cmocka_unit_test(imageRepFallsBackToClosestSmallerRepresentation),
		cmocka_unit_test(imageRepHandlesNonUniformDestination),
		cmocka_unit_test(imageRepSelectionDoesNotDependOnAppendOrder),
	};

	return cmocka_run_group_tests_name("uiImage", tests, NULL, NULL);
}
