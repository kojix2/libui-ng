// 27 september 2026
#include "../ui.h"
#include "uipriv.h"
#include <limits.h>

// uiControl is public ABI, so keep the override in a sidecar.
typedef struct controlMinimumSize controlMinimumSize;
struct controlMinimumSize {
	uiControl *c;
	int width;
	int height;
	controlMinimumSize *next;
};

static controlMinimumSize *minimumSizes;

static controlMinimumSize **findMinimumSize(uiControl *c)
{
	controlMinimumSize **p;

	for (p = &minimumSizes; *p != NULL; p = &((*p)->next))
		if ((*p)->c == c)
			return p;
	return NULL;
}

void uiprivControlMinimumSizeGet(uiControl *c, int *width, int *height)
{
	controlMinimumSize **p;

	p = findMinimumSize(c);
	if (p == NULL) {
		*width = -1;
		*height = -1;
		return;
	}
	*width = (*p)->width;
	*height = (*p)->height;
}

void uiprivControlMinimumSizeApply(uiControl *c, int *width, int *height,
	int isContainer)
{
	int overrideWidth, overrideHeight;

	uiprivControlMinimumSizeGet(c, &overrideWidth, &overrideHeight);
	if (overrideWidth >= 0) {
		if (isContainer) {
			if (*width < overrideWidth)
				*width = overrideWidth;
		} else
			*width = overrideWidth;
	}
	if (overrideHeight >= 0) {
		if (isContainer) {
			if (*height < overrideHeight)
				*height = overrideHeight;
		} else
			*height = overrideHeight;
	}
}

int uiprivMinimumSizeAdd(int a, int b)
{
	if (a < 0 || b < 0)
		uiprivImplBug("negative value in minimum-size sum (%d, %d)", a, b);
	if (a > INT_MAX - b)
		return INT_MAX;
	return a + b;
}

int uiprivMinimumSizeMultiply(int a, int b)
{
	if (a < 0 || b < 0)
		uiprivImplBug("negative value in minimum-size product (%d, %d)", a, b);
	if (b != 0 && a > INT_MAX / b)
		return INT_MAX;
	return a * b;
}

void uiControlSetMinimumSize(uiControl *c, int width, int height)
{
	controlMinimumSize **p;
	controlMinimumSize *size;

	if (c == NULL)
		uiprivUserBug("uiControlSetMinimumSize() cannot be called with NULL");
	if (width < -1 || height < -1)
		uiprivUserBug("uiControlSetMinimumSize() width and height must be at least -1");
	if (uiprivControlDestroyPending(c))
		uiprivUserBug("uiControlSetMinimumSize() cannot be called on a control pending destruction");
	if (uiControlToplevel(c))
		uiprivUserBug("uiControlSetMinimumSize() cannot be called on a top-level uiControl");

	p = findMinimumSize(c);
	if (width == -1 && height == -1) {
		if (p != NULL) {
			size = *p;
			*p = size->next;
			uiprivFree(size);
		}
	} else if (p != NULL) {
		(*p)->width = width;
		(*p)->height = height;
	} else {
		size = uiprivNew(controlMinimumSize);
		size->c = c;
		size->width = width;
		size->height = height;
		size->next = minimumSizes;
		minimumSizes = size;
	}

	uiprivControlMinimumSizeChanged(c);
}

void uiprivControlMinimumSizeRemove(uiControl *c)
{
	controlMinimumSize **p;
	controlMinimumSize *size;

	p = findMinimumSize(c);
	if (p != NULL) {
		size = *p;
		*p = size->next;
		uiprivFree(size);
	}
	uiprivControlMinimumSizeDestroyed(c);
}
