#include "../../windows/uipriv_windows.hpp"
#include "../../windows/area.hpp"

extern "C" BOOL auditAreaInside(const uiArea *a)
{
	return a->inside;
}

extern "C" BOOL auditAreaCapturing(const uiArea *a)
{
	return a->capturing;
}

extern "C" BOOL auditAreaTracking(const uiArea *a)
{
	return a->tracking;
}
