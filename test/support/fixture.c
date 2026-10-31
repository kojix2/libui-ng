#include "fixture.h"

int unitWindowOnClosingQuit(uiWindow *w, void *data)
{
	uiQuit();
	return 1;
}

int unitTestsSetup(void **state)
{
	*state = malloc(sizeof(struct state));
	assert_non_null(*state);
	return 0;
}

int unitTestsTeardown(void **state)
{
	free(*state);
	return 0;
}

int unitTestSetup(void **_state)
{
	struct state *state = *_state;
	uiInitOptions o = {0};

	assert_null(uiInit(&o));
	state->c = NULL;
	state->w = uiNewWindow(UNIT_TEST_WINDOW_TITLE, UNIT_TEST_WINDOW_WIDTH, UNIT_TEST_WINDOW_HEIGHT, 0);
	uiWindowOnClosing(state->w, unitWindowOnClosingQuit, NULL);
	return 0;
}

int unitTestTeardown(void **_state)
{
	struct state *state = *_state;

	if (state->c != NULL)
		uiWindowSetChild(state->w, uiControl(state->c));
	uiControlShow(uiControl(state->w));
	uiMainSteps();
	uiMainStep(1);
	uiControlDestroy(uiControl(state->w));
	uiUninit();
	return 0;
}

/* The child must already belong to the window; no display or event pumping. */
int unitTestTeardownHidden(void **_state)
{
	struct state *state = *_state;

	uiControlDestroy(uiControl(state->w));
	uiUninit();
	return 0;
}
