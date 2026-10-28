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

#ifdef _WIN32
	// Windows ABIs are little-endian. Whole-word loads/stores also let the
	// compiler vectorize swizzling without alignment requirements on input.
	if (order == uiprivImageBufferBGRA) {
		for (y = 0; y < height; y++) {
			if (format == uiImageBufferRGBA8Straight) {
				for (x = 0; x < width; x++) {
					uint32_t pixel, rb, g, alpha;
					memcpy(&pixel, source + (size_t) x * 4, 4);
					alpha = pixel >> 24;
					// Independent 16-bit lanes; rounded division by 255 is
					// (t + (t >> 8)) >> 8 for t = component * alpha + 128.
					rb = (pixel & UINT32_C(0x00FF00FF)) * alpha + UINT32_C(0x00800080);
					rb = ((rb + ((rb >> 8) & UINT32_C(0x00FF00FF))) >> 8) & UINT32_C(0x00FF00FF);
					g = ((pixel >> 8) & 255) * alpha + 128;
					g = (g + (g >> 8)) >> 8;
					pixel = (pixel & UINT32_C(0xFF000000)) | (g << 8) |
						(rb << 16) | (rb >> 16);
					memcpy(dest + (size_t) x * 4, &pixel, 4);
				}
			} else {
				for (x = 0; x < width; x++) {
					uint32_t pixel;
					memcpy(&pixel, source + (size_t) x * 4, 4);
					pixel = (pixel & UINT32_C(0xFF00FF00)) |
						((pixel & UINT32_C(0xFF)) << 16) | ((pixel >> 16) & UINT32_C(0xFF));
					memcpy(dest + (size_t) x * 4, &pixel, 4);
				}
			}
			if (y + 1 < height) { source += sourceStride; dest += destStride; }
		}
		return;
	}
#endif
	// Cairo's ARGB32 is a native-endian word. Select the alpha conversion once
	// per row so the inner loops only convert and store pixels. memcpy keeps
	// unaligned input/output valid and lets the compiler combine the stores.
	if (order == uiprivImageBufferARGB32) {
		for (y = 0; y < height; y++) {
			if (format == uiImageBufferRGBA8Straight) {
				for (x = 0; x < width; x++) {
					size_t offset = (size_t) x * 4;
					unsigned int a = source[offset + 3];
					unsigned int r = (source[offset] * a + 127) / 255;
					unsigned int g = (source[offset + 1] * a + 127) / 255;
					unsigned int b = (source[offset + 2] * a + 127) / 255;
					uint32_t argb = (a << 24) | (r << 16) | (g << 8) | b;
					memcpy(dest + offset, &argb, 4);
				}
			} else {
				for (x = 0; x < width; x++) {
					size_t offset = (size_t) x * 4;
					uint32_t argb = ((uint32_t) source[offset + 3] << 24) |
						((uint32_t) source[offset] << 16) |
						((uint32_t) source[offset + 1] << 8) | source[offset + 2];
					memcpy(dest + offset, &argb, 4);
				}
			}
			// Do not step beyond the final row's readable bytes.
			if (y + 1 < height) {
				source += sourceStride;
				dest += destStride;
			}
		}
		return;
	}
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			size_t offset = (size_t) x * 4;
			unsigned int r = source[offset], g = source[offset + 1];
			unsigned int b = source[offset + 2], a = source[offset + 3];

			if (format == uiImageBufferRGBA8Straight) {
				r = (r * a + 127) / 255;
				g = (g * a + 127) / 255;
				b = (b * a + 127) / 255;
			}
			dest[offset] = (unsigned char) (order == uiprivImageBufferBGRA ? b : r);
			dest[offset + 1] = (unsigned char) g;
			dest[offset + 2] = (unsigned char) (order == uiprivImageBufferBGRA ? r : b);
			dest[offset + 3] = (unsigned char) a;
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
	uiprivImageBufferUpdated(buffer, x, y, width, height);
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
