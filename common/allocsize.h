#ifndef UIPRIV_ALLOCSIZE_H
#define UIPRIV_ALLOCSIZE_H

#include <stddef.h>
#include <stdint.h>

static int uiprivAllocSizeWithHeader(size_t size, size_t extra, size_t *total)
{
	if (size > SIZE_MAX - extra)
		return 0;
	*total = extra + size;
	return 1;
}

#endif
