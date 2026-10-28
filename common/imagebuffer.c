#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../ui.h"
#include "uipriv.h"

int uiprivImageBufferInputSpan(int width, int height, size_t stride, size_t *span)
{
	size_t row;

	if (width <= 0 || height <= 0 || (size_t) width > SIZE_MAX / 4)
		return 0;
	row = (size_t) width * 4;
	if (stride < row || (size_t) (height - 1) > (SIZE_MAX - row) / stride)
		return 0;
	*span = (size_t) (height - 1) * stride + row;
	return 1;
}

int uiprivImageBufferStorageSize(int width, int height, size_t stride, size_t *size)
{
	size_t span;

	if (!uiprivImageBufferInputSpan(width, height, stride, &span) ||
		(size_t) height > SIZE_MAX / stride)
		return 0;
	*size = (size_t) height * stride;
	return *size <= PTRDIFF_MAX;
}

void uiprivImageBufferConvert(unsigned char *dest, size_t destStride,
	const unsigned char *source, size_t sourceStride, int width, int height,
	uiImageBufferFormat format, uiprivImageBufferOrder order)
{
	int x, y;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			size_t offset = (size_t) x * 4;
			unsigned int r = source[offset], g = source[offset + 1];
			unsigned int b = source[offset + 2], a = source[offset + 3];
			uint32_t argb;

			if (format == uiImageBufferRGBA8Straight) {
				r = (r * a + 127) / 255;
				g = (g * a + 127) / 255;
				b = (b * a + 127) / 255;
			}
			if (order == uiprivImageBufferARGB32) {
				argb = (a << 24) | (r << 16) | (g << 8) | b;
				memcpy(dest + offset, &argb, 4);
			} else {
				dest[offset] = (unsigned char) (order == uiprivImageBufferBGRA ? b : r);
				dest[offset + 1] = (unsigned char) g;
				dest[offset + 2] = (unsigned char) (order == uiprivImageBufferBGRA ? r : b);
				dest[offset + 3] = (unsigned char) a;
			}
		}
		// Never form a pointer beyond the final row's readable bytes.
		if (y + 1 < height) {
			source += sourceStride;
			dest += destStride;
		}
	}
}

uiImageBuffer *uiNewImageBuffer(int width, int height, uiImageBufferFormat format)
{
	uiImageBuffer *buffer;
	size_t size;

	if (format != uiImageBufferRGBA8Straight &&
		format != uiImageBufferRGBA8Premultiplied)
		return NULL;
	if (width <= 0 || (size_t) width > SIZE_MAX / 4 ||
		!uiprivImageBufferStorageSize(width, height, (size_t) width * 4, &size))
		return NULL;
	buffer = (uiImageBuffer *) calloc(1, sizeof *buffer);
	if (buffer == NULL)
		return NULL;
	buffer->width = width;
	buffer->height = height;
	buffer->format = format;
	buffer->stride = (size_t) width * 4;
	if (!uiprivImageBufferCreate(buffer)) {
		free(buffer);
		return NULL;
	}
	return buffer;
}

void uiFreeImageBuffer(uiImageBuffer *buffer)
{
	if (buffer == NULL)
		return;
	uiprivImageBufferDestroy(buffer);
	free(buffer);
}

void uiImageBufferGetSize(const uiImageBuffer *buffer, int *width, int *height)
{
	if (buffer == NULL || width == NULL || height == NULL) {
		uiprivUserBug("uiImageBufferGetSize() requires a buffer and both output pointers.");
		return;
	}
	*width = buffer->width;
	*height = buffer->height;
}

int uiImageBufferUpdate(uiImageBuffer *buffer, const void *pixels,
	size_t byteLength, size_t byteStride)
{
	if (buffer == NULL)
		return 0;
	return uiImageBufferUpdateRect(buffer, 0, 0, buffer->width, buffer->height,
		pixels, byteLength, byteStride);
}

int uiprivImageBufferRectValid(const uiImageBuffer *buffer,
	int x, int y, int width, int height)
{
	// Subtract only after checking the origin, avoiding signed x+width overflow.
	return buffer != NULL && x >= 0 && y >= 0 && width > 0 && height > 0 &&
		x <= buffer->width && y <= buffer->height &&
		width <= buffer->width - x && height <= buffer->height - y;
}

int uiImageBufferUpdateRect(uiImageBuffer *buffer,
	int x, int y, int width, int height, const void *pixels,
	size_t byteLength, size_t byteStride)
{
	size_t required;

	if (!uiprivImageBufferRectValid(buffer, x, y, width, height) || pixels == NULL ||
		!uiprivImageBufferInputSpan(width, height, byteStride, &required) ||
		byteLength < required)
		return 0;
	if (!uiprivImageBufferPrepareUpdate(buffer))
		return 0;
	// Buffer construction validated the storage extent; the rectangle fits in it.
	uiprivImageBufferConvert(buffer->pixels + (size_t) y * buffer->stride + (size_t) x * 4,
		buffer->stride, (const unsigned char *) pixels, byteStride, width, height,
		buffer->format, buffer->order);
	uiprivImageBufferUpdated(buffer);
	return 1;
}

int uiprivImageBufferDrawValid(const uiDrawContext *c, const uiImageBuffer *buffer,
	double x, double y, double width, double height,
	uiImageBufferInterpolation interpolation)
{
	return c != NULL && buffer != NULL &&
		(interpolation == uiImageBufferInterpolationSmooth ||
		 interpolation == uiImageBufferInterpolationNearest) &&
		uiprivImageFinite(x) && uiprivImageFinite(y) &&
		uiprivImagePositiveFinite(width) && uiprivImagePositiveFinite(height) &&
		uiprivImageFinite(x + width) && uiprivImageFinite(y + height) &&
		x + width > x && y + height > y;
}

int uiprivImageBufferDrawParamsValid(const uiDrawContext *c,
	const uiImageBuffer *buffer, const uiDrawImageBufferParams *params)
{
	return params != NULL &&
		params->Size >= offsetof(uiDrawImageBufferParams, Interpolation) + sizeof params->Interpolation &&
		uiprivImageBufferRectValid(buffer, params->SourceX, params->SourceY,
			params->SourceWidth, params->SourceHeight) &&
		uiprivImageBufferDrawValid(c, buffer, params->X, params->Y,
			params->Width, params->Height, params->Interpolation);
}

void uiDrawImageBuffer(uiDrawContext *c, const uiImageBuffer *buffer,
	double x, double y, double width, double height,
	uiImageBufferInterpolation interpolation)
{
	uiDrawImageBufferParams params;

	if (buffer == NULL)
		return;
	params.Size = sizeof params;
	params.SourceX = 0;
	params.SourceY = 0;
	params.SourceWidth = buffer->width;
	params.SourceHeight = buffer->height;
	params.X = x;
	params.Y = y;
	params.Width = width;
	params.Height = height;
	params.Interpolation = interpolation;
	uiDrawImageBufferWithParams(c, buffer, &params);
}
