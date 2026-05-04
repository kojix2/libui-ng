// 25 june 2016
#import "uipriv_darwin.h"
#include <limits.h>

struct uiImage {
	NSImage *i;
	NSSize size;
};

uiImage *uiNewImage(double width, double height)
{
	uiImage *i;

	i = uiprivNew(uiImage);
	i->size = NSMakeSize(width, height);
	i->i = [[NSImage alloc] initWithSize:i->size];
	return i;
}

void uiFreeImage(uiImage *i)
{
	[i->i release];
	uiprivFree(i);
}

static uint8_t premultiply(uint8_t c, uint8_t a)
{
	return (uint8_t) ((((uint32_t) c) * ((uint32_t) a) + 127) / 255);
}

void uiImageAppend(uiImage *i, void *pixels, int pixelWidth, int pixelHeight, int byteStride)
{
	NSBitmapImageRep *repCalibrated, *repsRGB;
	int x, y;
	uint8_t *pix, *data;
	NSInteger realStride;

	if (i == NULL)
		uiprivUserBug("You cannot append a uiImage representation to NULL.");
	if (pixels == NULL)
		uiprivUserBug("You cannot append a NULL pixel buffer to a uiImage.");
	if (pixelWidth <= 0)
		uiprivUserBug("You cannot append a uiImage representation with pixel width %d.", pixelWidth);
	if (pixelHeight <= 0)
		uiprivUserBug("You cannot append a uiImage representation with pixel height %d.", pixelHeight);
	if (pixelWidth > INT_MAX / 4)
		uiprivUserBug("You cannot append a uiImage representation with pixel width %d.", pixelWidth);
	if (byteStride < pixelWidth * 4)
		uiprivUserBug("You cannot append a uiImage representation with byte stride %d and pixel width %d.", byteStride, pixelWidth);

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

	if (repCalibrated == nil) {
		return; // Failed to create bitmap representation, abort operation
	}

	// Apple doesn't explicitly document this, but we apparently need to use native system endian for the data :|
	// TODO split this into a utility routine?
	// TODO find proper documentation
	pix = (uint8_t *) pixels;
	data = (uint8_t *) [repCalibrated bitmapData];
	if (data == NULL) {
		[repCalibrated release];
		return;
	}
	realStride = [repCalibrated bytesPerRow];
	for (y = 0; y < pixelHeight; y++) {
		for (x = 0; x < pixelWidth * 4; x += 4) {
			union {
				uint32_t v32;
				uint8_t v8[4];
			} v;
			uint8_t a, r, g, b;

			a = pix[x + 3];
			r = premultiply(pix[x], a);
			g = premultiply(pix[x + 1], a);
			b = premultiply(pix[x + 2], a);
			v.v32 = ((uint32_t) a) << 24;
			v.v32 |= ((uint32_t) b) << 16;
			v.v32 |= ((uint32_t) g) << 8;
			v.v32 |= ((uint32_t) r);
			data[x] = v.v8[0];
			data[x + 1] = v.v8[1];
			data[x + 2] = v.v8[2];
			data[x + 3] = v.v8[3];
		}
		pix += byteStride;
		data += realStride;
	}

	// we can't call the constructor with this, but we can retag (NOT convert)
	repsRGB = [repCalibrated bitmapImageRepByRetaggingWithColorSpace:[NSColorSpace sRGBColorSpace]];

	[i->i addRepresentation:repsRGB];
	[repsRGB setSize:i->size];
	// don't release repsRGB; it may be equivalent to repCalibrated
	// do release repCalibrated though; NSImage has a ref to either it or to repsRGB
	[repCalibrated release];
}

NSImage *uiprivImageNSImage(uiImage *i)
{
	return i->i;
}
