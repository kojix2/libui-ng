#ifndef UIPRIV_WINDOWS_DROP_HPP
#define UIPRIV_WINDOWS_DROP_HPP
#include "../common/drop.h"
typedef int (*uiprivDropPosition)(uiControl *, POINTL, double *, double *);
extern uiprivDropEndpoint *uiprivWindowsDropNew(uiControl *, HWND, uiprivDropPosition);
#endif
