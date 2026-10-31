#ifndef LIBUI_TEST_FIXTURE_H
#define LIBUI_TEST_FIXTURE_H

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>

#include "../../ui.h"

#define UNIT_TEST_NAMED(name, fn, setup, teardown) \
	{ (name), (fn), (setup), (teardown), NULL }

#define UNIT_TEST_WINDOW_WIDTH 300
#define UNIT_TEST_WINDOW_HEIGHT 200
#define UNIT_TEST_WINDOW_TITLE "Unit Test"

/* Group state owns the allocation only. Each case owns its libui lifetime. */
struct state {
	uiWindow *w;
	uiControl *c;
};

int unitWindowOnClosingQuit(uiWindow *w, void *data);
int unitTestsSetup(void **state);
int unitTestsTeardown(void **state);

/* Initialize libui, clear c, and create a hidden window with a close callback.
 * Set c to a control owned by the case, or leave it NULL. Feature-specific
 * state belongs in the test, not in this shared fixture.
 */
int unitTestSetup(void **state);

/* Legacy sequence: attach c (if any), show the window, enter stepping mode,
 * run one blocking step, destroy the window and its child, then uninitialize.
 * Keep this order for existing unit tests, including after an assertion fails.
 */
int unitTestTeardown(void **state);

/* Destroy the window and its already attached child, then uninitialize.
 * The case must transfer c to w with uiWindowSetChild() before assertions.
 * This path neither shows the window nor runs the event loop. It still needs
 * a GUI session for uiInit(); "hidden" does not mean headless.
 */
int unitTestTeardownHidden(void **state);

#define uiControlPtrFromState(t, s) (t**)&(((struct state *)*(s))->c)

#endif
