#include <float.h>
#include <limits.h>
#include "../ui.h"
#include "uipriv.h"

int uiprivImageFinite(double value)
{
	return value >= -DBL_MAX && value <= DBL_MAX;
}

int uiprivImagePositiveFinite(double value)
{
	return value > 0 && value <= DBL_MAX;
}

int uiprivImageTargetPixelSize(double size)
{
	int pixels;

	if (!uiprivImagePositiveFinite(size))
		return 0;
	if (size >= INT_MAX)
		return INT_MAX;
	if (size <= 1)
		return 1;
	pixels = (int) size;
	if ((double) pixels < size)
		pixels++;
	return pixels;
}

int uiprivImagePixelBufferSpan(int pixelWidth, int pixelHeight,
	int byteStride, size_t *span)
{
	uint64_t bytes;
	uint64_t rowBytes;

	if (span != NULL)
		*span = 0;
	if (pixelWidth <= 0 || pixelHeight <= 0 || byteStride <= 0)
		return 0;
	rowBytes = (uint64_t) pixelWidth * 4;
	if ((uint64_t) byteStride < rowBytes)
		return 0;
	bytes = (uint64_t) byteStride * pixelHeight;
	if (bytes > SIZE_MAX)
		return 0;
	if (span != NULL)
		*span = (size_t) bytes;
	return 1;
}

void uiprivImageRepMatcherInit(uiprivImageRepMatcher *m,
	int targetWidth, int targetHeight)
{
	if (targetWidth < 1)
		targetWidth = 1;
	if (targetHeight < 1)
		targetHeight = 1;
	m->targetWidth = targetWidth;
	m->targetHeight = targetHeight;
	m->bestWidth = 0;
	m->bestHeight = 0;
	m->bestDistance = INT64_MAX;
	m->bestIsLargeEnough = 0;
	m->hasBest = 0;
}

int uiprivImageRepMatcherAdd(uiprivImageRepMatcher *m,
	int width, int height)
{
	int isLargeEnough;
	int64_t dx, dy, distance;

	if (width <= 0 || height <= 0)
		return 0;
	isLargeEnough = width >= m->targetWidth && height >= m->targetHeight;
	dx = (int64_t) width - m->targetWidth;
	dy = (int64_t) height - m->targetHeight;
	if (dx < 0)
		dx = -dx;
	if (dy < 0)
		dy = -dy;
	distance = dx + dy;

	if (m->hasBest) {
		if (isLargeEnough != m->bestIsLargeEnough) {
			if (!isLargeEnough)
				return 0;
		} else if (distance > m->bestDistance)
			return 0;
		else if (distance == m->bestDistance) {
			if (width > m->bestWidth)
				return 0;
			if (width == m->bestWidth && height >= m->bestHeight)
				return 0;
		}
	}

	m->bestWidth = width;
	m->bestHeight = height;
	m->bestDistance = distance;
	m->bestIsLargeEnough = isLargeEnough;
	m->hasBest = 1;
	return 1;
}

void uiprivImageFitRect(int imageWidth, int imageHeight,
	int boundsWidth, int boundsHeight,
	int *x, int *y, int *width, int *height)
{
	int64_t scaled;

	*x = 0;
	*y = 0;
	*width = 0;
	*height = 0;
	if (imageWidth <= 0 || imageHeight <= 0 ||
		boundsWidth <= 0 || boundsHeight <= 0)
		return;

	if ((int64_t) imageWidth * boundsHeight >
		(int64_t) boundsWidth * imageHeight) {
		*width = boundsWidth;
		scaled = (int64_t) imageHeight * boundsWidth / imageWidth;
		*height = scaled < 1 ? 1 : (int) scaled;
	} else {
		*height = boundsHeight;
		scaled = (int64_t) imageWidth * boundsHeight / imageHeight;
		*width = scaled < 1 ? 1 : (int) scaled;
	}
	*x = (boundsWidth - *width) / 2;
	*y = (boundsHeight - *height) / 2;
}
