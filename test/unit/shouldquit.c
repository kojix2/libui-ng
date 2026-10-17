#include "unit.h"
#include "../../common/uipriv.h"

int uiprivUserCallbackEnter(uiControl *c)
{
	(void) c;
	return 1;
}

void uiprivUserCallbackLeave(void)
{
}

static int shouldQuitAndCount(void *data)
{
	int *calls = data;

	(*calls)++;
	return 1;
}

static void shouldQuitDoesNotSurviveUninit(void **state)
{
	int calls = 0;

	uiOnShouldQuit(shouldQuitAndCount, &calls);
	assert_int_equal(uiprivShouldQuit(), 1);
	assert_int_equal(calls, 1);

	uiprivUninitShouldQuit();
	assert_int_equal(uiprivShouldQuit(), 0);
	assert_int_equal(calls, 1);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(shouldQuitDoesNotSurviveUninit),
	};

	return cmocka_run_group_tests_name("should quit", tests, NULL, NULL);
}
