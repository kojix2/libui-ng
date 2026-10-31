#ifndef LIBUI_ENTRY_TEST_H
#define LIBUI_ENTRY_TEST_H

#include "unit.h"

#define uiEntryPtrFromState(s) uiControlPtrFromState(uiEntry, s)

/* Native cases stay in the existing component's registration and fixture. */
#if !defined(_WIN32) && !defined(__APPLE__)
void searchEntrySetTextDoesNotNotifyLater(void **state);
void searchEntrySetterCancelsPendingUserChange(void **state);
#endif

#endif
