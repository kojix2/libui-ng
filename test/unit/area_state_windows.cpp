#include "../../windows/uipriv_windows.hpp"
#include "../../windows/area.hpp"
#ifdef _UI_STATIC
#include "../../windows/draw.hpp"

extern "C" float auditPathLength(uiDrawPath *path)
{
	FLOAT length = -1;

	if (pathGeometry(path)->ComputeLength(NULL, 0.01f, &length) != S_OK)
		return -1;
	return length;
}
#endif

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
