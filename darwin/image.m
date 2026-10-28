// 25 june 2016
#import "uipriv_darwin.h"
#include <float.h>
#include <limits.h>
#include <stdlib.h>

struct uiImage {
	NSImage *i;
	NSSize size;
};

uiImage *uiNewImage(double width, double height)
{
	uiImage *i;

	if (!(width > 0) || width > INT_MAX ||
		!(height > 0) || height > INT_MAX) {
		uiprivUserBug("uiNewImage() dimensions must be finite, positive, and no greater than INT_MAX.");
		return NULL;
	}
	i = uiprivNew(uiImage);
	i->size = NSMakeSize(width, height);
	i->i = [[NSImage alloc] initWithSize:i->size];
	return i;
}

void uiImageGetSize(uiImage *i, double *width, double *height)
{
	*width = i->size.width;
	*height = i->size.height;
}

static void freeImage(void *p)
{
	uiImage *i = p;

	[i->i release];
	uiprivFree(i);
}

void uiFreeImage(uiImage *i)
{
	if (uiprivUserCallbackDeferFree(i, freeImage))
		return;
	freeImage(i);
}

void uiImageAppend(uiImage *i, const void *pixels, int pixelWidth, int pixelHeight, int byteStride)
{
	NSBitmapImageRep *repCalibrated, *repsRGB;
	int x, y;
	const uint8_t *pix;
	uint8_t *data;
	NSInteger realStride;

	if (i == NULL) {
		uiprivUserBug("You cannot append a uiImage representation to NULL.");
		return;
	}
	if (pixels == NULL) {
		uiprivUserBug("You cannot append a NULL pixel buffer to a uiImage.");
		return;
	}
	if (pixelWidth <= 0) {
		uiprivUserBug("You cannot append a uiImage representation with pixel width %d.", pixelWidth);
		return;
	}
	if (pixelHeight <= 0) {
		uiprivUserBug("You cannot append a uiImage representation with pixel height %d.", pixelHeight);
		return;
	}
	if (pixelWidth > INT_MAX / 4) {
		uiprivUserBug("You cannot append a uiImage representation with pixel width %d.", pixelWidth);
		return;
	}
	if (byteStride < pixelWidth * 4) {
		uiprivUserBug("You cannot append a uiImage representation with byte stride %d and pixel width %d.", byteStride, pixelWidth);
		return;
	}
	if (!uiprivImagePixelBufferSpan(pixelWidth, pixelHeight,
		byteStride, NULL)) {
		uiprivUserBug("The uiImage representation pixel buffer is too large to address on this platform.");
		return;
	}

	repCalibrated = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
		pixelsWide:pixelWidth
		pixelsHigh:pixelHeight
		bitsPerSample:8
		samplesPerPixel:4
		hasAlpha:YES
		isPlanar:NO
		colorSpaceName:NSCalibratedRGBColorSpace
		bitmapFormat:0
		bytesPerRow:0
		bitsPerPixel:32];
	if (repCalibrated == nil)
		return;

	// Apple doesn't explicitly document this, but we apparently need to use native system endian for the data :|
	// TODO split this into a utility routine?
	// TODO find proper documentation
	pix = (const uint8_t *) pixels;
	data = (uint8_t *) [repCalibrated bitmapData];
	if (data == NULL) {
		[repCalibrated release];
		return;
	}
	realStride = [repCalibrated bytesPerRow];
	if (realStride < pixelWidth * 4) {
		[repCalibrated release];
		return;
	}
	for (y = 0; y < pixelHeight; y++) {
		for (x = 0; x < pixelWidth * 4; x += 4) {
			union {
				uint32_t v32;
				uint8_t v8[4];
			} v;

			v.v32 = ((uint32_t) (pix[x + 3])) << 24;
			v.v32 |= ((uint32_t) (pix[x + 2])) << 16;
			v.v32 |= ((uint32_t) (pix[x + 1])) << 8;
			v.v32 |= ((uint32_t) (pix[x]));
			data[x] = v.v8[0];
			data[x + 1] = v.v8[1];
			data[x + 2] = v.v8[2];
			data[x + 3] = v.v8[3];
		}
		if (y + 1 < pixelHeight) {
			pix += byteStride;
			data += realStride;
		}
	}

	// we can't call the constructor with this, but we can retag (NOT convert)
	repsRGB = [repCalibrated bitmapImageRepByRetaggingWithColorSpace:[NSColorSpace sRGBColorSpace]];

	[i->i addRepresentation:repsRGB];
	[repsRGB setSize:i->size];
	// don't release repsRGB; it may be equivalent to repCalibrated
	// do release repCalibrated though; NSImage has a ref to either it or to repsRGB
	[repCalibrated release];
}

NSImage *uiprivImageNSImage(const uiImage *i)
{
	if (i == NULL)
		return nil;
	return i->i;
}

struct imageBufferStorage {
	unsigned int references;
	unsigned char *pixels;
};

struct imageBufferNative {
	CGImageRef image;
	CGColorSpaceRef color;
	struct imageBufferStorage *storage;
};

static struct imageBufferStorage *newImageBufferStorage(size_t size, int clear)
{
	struct imageBufferStorage *storage = malloc(sizeof *storage);

	if (storage == NULL)
		return NULL;
	storage->pixels = clear ? calloc(1, size) : malloc(size);
	if (storage->pixels == NULL) {
		free(storage);
		return NULL;
	}
	storage->references = 1;
	return storage;
}

static void releaseImageBufferStorage(struct imageBufferStorage *storage)
{
	// CoreGraphics may release a provider on a worker thread. The buffer's
	// reference keeps storage alive while the UI thread checks for readers.
	if (__atomic_sub_fetch(&storage->references, 1, __ATOMIC_ACQ_REL) == 0) {
		free(storage->pixels);
		free(storage);
	}
}

static void releaseImageBufferData(void *info, const void *data, size_t size)
{
	releaseImageBufferStorage(info);
}

int uiprivImageBufferCreate(uiImageBuffer *buffer)
{
	struct imageBufferNative *native;
	size_t size;

	if (!uiprivImageBufferStorageSize(buffer->width, buffer->height, buffer->stride, &size))
		return 0;
	native = calloc(1, sizeof *native);
	if (native == NULL)
		return 0;
	native->color = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
	if (native->color == NULL) {
		free(native);
		return 0;
	}
	native->storage = newImageBufferStorage(size, 1);
	if (native->storage == NULL) {
		CGColorSpaceRelease(native->color);
		free(native);
		return 0;
	}
	buffer->pixels = native->storage->pixels;
	buffer->native = native;
	buffer->order = uiprivImageBufferRGBA;
	return 1;
}

void uiprivImageBufferDestroy(uiImageBuffer *buffer)
{
	struct imageBufferNative *native = buffer->native;

	if (native->image != NULL)
		CGImageRelease(native->image);
	releaseImageBufferStorage(native->storage);
	CGColorSpaceRelease(native->color);
	free(native);
}

int uiprivImageBufferPrepareUpdate(uiImageBuffer *buffer)
{
	struct imageBufferNative *native = buffer->native;
	struct imageBufferStorage *storage;
	size_t size = buffer->stride * (size_t) buffer->height;

	// Drop our cached image before checking for external readers. Synchronous
	// bitmap draws normally release the provider here, allowing in-place edits.
	if (native->image != NULL) {
		CGImageRelease(native->image);
		native->image = NULL;
	}
	if (__atomic_load_n(&native->storage->references, __ATOMIC_ACQUIRE) > 1) {
		// PDF/retained images may still use these bytes. Copy only when an old
		// version survives; allocation failure leaves the previous pixels intact.
		storage = newImageBufferStorage(size, 0);
		if (storage == NULL)
			return 0;
		memcpy(storage->pixels, buffer->pixels, size);
		releaseImageBufferStorage(native->storage);
		native->storage = storage;
		buffer->pixels = storage->pixels;
	}
	return 1;
}

void uiprivImageBufferUpdated(uiImageBuffer *buffer, int x, int y, int width, int height)
{
	// PrepareUpdate already invalidated the cached image.
}

CGImageRef uiprivImageBufferImage(const uiImageBuffer *buffer)
{
	struct imageBufferNative *native = buffer->native;
	CGDataProviderRef provider;
	CGImageRef image = NULL;

	if (native->image != NULL)
		return native->image;
	provider = CGDataProviderCreateWithData(native->storage, buffer->pixels,
		buffer->stride * (size_t) buffer->height, releaseImageBufferData);
	if (provider == NULL)
		return NULL;
	__atomic_add_fetch(&native->storage->references, 1, __ATOMIC_RELAXED);
	image = CGImageCreate(buffer->width, buffer->height, 8, 32,
		buffer->stride, native->color, kCGBitmapByteOrder32Big | kCGImageAlphaPremultipliedLast,
		provider, NULL, false, kCGRenderingIntentDefault);
	CGDataProviderRelease(provider);
	if (image == NULL)
		return NULL;
	native->image = image;
	return image;
}
