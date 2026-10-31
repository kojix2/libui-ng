#ifndef LIBUI_AREA_TEST_H
#define LIBUI_AREA_TEST_H

#include "unit.h"

#define uiAreaFromState(s) ((uiArea *) (((struct state *) *(s))->c))

/* Existing Area observation state, shared only with its native cases. Keep
 * the original setup/case reset points; this header adds no shared lifetime.
 * These symbols belong to the test executable, never the libui SDK.
 */
extern int dragBrokenCalls;

#if !defined(_WIN32) && !defined(__APPLE__)
void scrollingAreaScrollAfterSetSize(void **state);
void areaDragBrokenForImplicitPointerGrab(void **state);
#endif

#endif
