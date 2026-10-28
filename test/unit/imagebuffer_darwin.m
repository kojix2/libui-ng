#include "unit.h"
#import "../../darwin/uipriv_darwin.h"
#include "../../darwin/draw.h"

static uiDrawContext canvas(int width, int height)
{
	CGColorSpaceRef color = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
	uiDrawContext c = {CGBitmapContextCreate(NULL, width, height, 8, width * 4, color,
		kCGBitmapByteOrder32Big | kCGImageAlphaPremultipliedLast), height};
	CGColorSpaceRelease(color);
	assert_non_null(c.c);
	CGContextTranslateCTM(c.c, 0, height);
	CGContextScaleCTM(c.c, 1, -1);
	return c;
}

static void pixel(uiDrawContext *c, int x, int y, const unsigned char *expected)
{
	CGContextFlush(c->c);
	const unsigned char *p = CGBitmapContextGetData(c->c);
	assert_memory_equal(p + y * CGBitmapContextGetBytesPerRow(c->c) + x * 4, expected, 4);
}

static const unsigned char red[] = {255, 0, 0, 255};
static const unsigned char blue[] = {0, 0, 255, 255};
static const unsigned char clear[] = {0, 0, 0, 0};

static void orientationAlphaAndStride(void **state)
{
	const unsigned char fixture[] = {
		255, 0, 0, 255, 0, 255, 0, 128, 9, 9, 9, 9,
		0, 0, 255, 255, 255, 255, 255, 0
	};
	unsigned char *input = malloc(sizeof fixture + 1);
	uiDrawContext c = canvas(4, 4);
	uiImageBuffer *b = uiNewImageBuffer(2, 2, uiImageBufferRGBA8Straight);
	(void) state;
	assert_non_null(b);
	assert_non_null(input);
	uiDrawImageBuffer(&c, b, 0, 0, 4, 4, uiImageBufferInterpolationNearest);
	pixel(&c, 0, 0, clear);
	memcpy(input + 1, fixture, sizeof fixture);
	assert_true(uiImageBufferUpdate(b, input + 1, sizeof fixture, 12));
	memset(input, 0, sizeof fixture + 1);
	free(input);
	assert_false(uiImageBufferUpdate(b, fixture, sizeof fixture - 1, 12));
	uiDrawImageBuffer(&c, b, 0, 0, 4, 4, uiImageBufferInterpolationNearest);
	pixel(&c, 0, 0, red);
	pixel(&c, 3, 0, (const unsigned char[]) {0, 128, 0, 128});
	pixel(&c, 0, 3, blue);
	pixel(&c, 3, 3, clear);
	uiFreeImageBuffer(b);
	b = uiNewImageBuffer(1, 1, uiImageBufferRGBA8Premultiplied);
	assert_non_null(b);
	assert_true(uiImageBufferUpdate(b, (const unsigned char[]) {64, 32, 16, 128}, 4, 4));
	CGContextClearRect(c.c, CGRectMake(0, 0, 4, 4));
	uiDrawImageBuffer(&c, b, 0, 0, 4, 4, uiImageBufferInterpolationNearest);
	pixel(&c, 2, 2, (const unsigned char[]) {64, 32, 16, 128});
	uiFreeImageBuffer(b);
	CGContextRelease(c.c);
}

static void croppedUpdatesAndState(void **state)
{
	uiDrawContext c = canvas(12, 12);
	uiImageBuffer *b = uiNewImageBuffer(5, 4, uiImageBufferRGBA8Straight);
	uiDrawImageBufferParams p = {sizeof p, 2, 1, 2, 2, 0, 0, 4, 4,
		uiImageBufferInterpolationNearest};
	CGAffineTransform before, after;
	CGPathRef path;
	(void) state;
	assert_non_null(b);
	assert_true(uiImageBufferUpdateRect(b, 2, 1, 1, 1, red, 4, 4));
	assert_true(uiImageBufferUpdateRect(b, 3, 2, 1, 1, blue, 4, 4));
	assert_false(uiImageBufferUpdateRect(b, 4, 3, 2, 1, red, 4, 4));
	CGContextTranslateCTM(c.c, 2, 3);
	CGContextClipToRect(c.c, CGRectMake(0, 0, 3, 3));
	CGContextMoveToPoint(c.c, 0.25, 0.5);
	CGContextAddLineToPoint(c.c, 1, 1.5);
	path = CGContextCopyPath(c.c);
	before = CGContextGetCTM(c.c);
	CGContextSetInterpolationQuality(c.c, kCGInterpolationLow);
	CGContextSetBlendMode(c.c, kCGBlendModeCopy);
	uiDrawImageBufferWithParams(&c, b, &p);
	after = CGContextGetCTM(c.c);
	assert_true(CGAffineTransformEqualToTransform(before, after));
	assert_int_equal(CGContextGetInterpolationQuality(c.c), kCGInterpolationLow);
	{
		CGPathRef afterPath = CGContextCopyPath(c.c);
		assert_true(CGPathEqualToPath(path, afterPath));
		CGPathRelease(afterPath);
	}
	CGPathRelease(path);
	pixel(&c, 2, 3, red);
	pixel(&c, 4, 5, blue);
	pixel(&c, 5, 5, clear);
	pixel(&c, 2, 5, clear);
	uiFreeImageBuffer(b);
	CGContextRelease(c.c);
}

static void submittedPDFDraws(void **state)
{
	CFMutableDataRef data = CFDataCreateMutable(NULL, 0);
	CGDataConsumerRef consumer = CGDataConsumerCreateWithCFData(data);
	CGRect bounds = CGRectMake(0, 0, 130, 2);
	uiDrawContext c = {CGPDFContextCreate(consumer, &bounds, NULL), 2};
	uiImageBuffer *b = uiNewImageBuffer(1, 1, uiImageBufferRGBA8Straight);
	uiImageBuffer *patch = uiNewImageBuffer(3, 2, uiImageBufferRGBA8Straight);
	uiDrawImageBufferParams p = {sizeof p, 2, 1, 1, 1, 128, 0, 1, 2,
		uiImageBufferInterpolationNearest};
	int i;
	(void) state;
	assert_non_null(c.c);
	assert_non_null(b);
	assert_non_null(patch);
	CGDataConsumerRelease(consumer);
	CGPDFContextBeginPage(c.c, NULL);
	CGContextTranslateCTM(c.c, 0, 2);
	CGContextScaleCTM(c.c, 1, -1);
	for (i = 0; i < 64; i++) {
		assert_true(uiImageBufferUpdate(b, red, 4, 4));
		uiDrawImageBuffer(&c, b, i * 2, 0, 1, 2, uiImageBufferInterpolationNearest);
		assert_true(uiImageBufferUpdate(b, blue, 4, 4));
		uiDrawImageBuffer(&c, b, i * 2 + 1, 0, 1, 2, uiImageBufferInterpolationNearest);
	}
	assert_true(uiImageBufferUpdateRect(patch, 2, 1, 1, 1, red, 4, 4));
	uiDrawImageBufferWithParams(&c, patch, &p);
	assert_true(uiImageBufferUpdateRect(patch, 2, 1, 1, 1, blue, 4, 4));
	p.X = 129;
	uiDrawImageBufferWithParams(&c, patch, &p);
	uiFreeImageBuffer(patch);
	uiFreeImageBuffer(b);
	CGPDFContextEndPage(c.c);
	CGPDFContextClose(c.c);
	CGContextRelease(c.c);
	{
		CGDataProviderRef provider = CGDataProviderCreateWithCFData(data);
		CGPDFDocumentRef doc = CGPDFDocumentCreateWithProvider(provider);
		assert_non_null(doc);
		// Replay only after update/free, to two independent destinations.
		for (i = 0; i < 2; i++) {
			uiDrawContext output = canvas(130, 2);
			CGContextTranslateCTM(output.c, 0, 2);
			CGContextScaleCTM(output.c, 1, -1);
			CGContextDrawPDFPage(output.c, CGPDFDocumentGetPage(doc, 1));
			for (int x = 0; x < 130; x++)
				pixel(&output, x, 0, x % 2 ? blue : red);
			CGContextRelease(output.c);
		}
		CGPDFDocumentRelease(doc);
		CGDataProviderRelease(provider);
	}
	CFRelease(data);
}

#ifdef _UI_STATIC
static void retainedSnapshots(void **state)
{
	uiImageBuffer *b = uiNewImageBuffer(2, 1, uiImageBufferRGBA8Straight);
	CGImageRef image, cropped;
	CFDataRef bytes;
	unsigned char *pixels;
	(void) state;
	assert_non_null(b);
	assert_true(uiImageBufferUpdateRect(b, 0, 0, 1, 1, red, 4, 4));
	pixels = b->pixels;
	image = uiprivImageBufferImage(b);
	assert_non_null(image);
	assert_ptr_equal(image, uiprivImageBufferImage(b));
	// No external image: invalidate the cache and reuse the backing allocation.
	assert_true(uiImageBufferUpdateRect(b, 1, 0, 1, 1, blue, 4, 4));
	assert_ptr_equal(b->pixels, pixels);
	image = CGImageRetain(uiprivImageBufferImage(b));
	cropped = CGImageCreateWithImageInRect(image, CGRectMake(1, 0, 1, 1));
	assert_non_null(cropped);
	// An explicitly retained image forces a copy, preserving the other pixel.
	assert_true(uiImageBufferUpdateRect(b, 0, 0, 1, 1, blue, 4, 4));
	assert_ptr_not_equal(b->pixels, pixels);
	assert_memory_equal(b->pixels, blue, 4);
	assert_memory_equal(b->pixels + 4, blue, 4);
	uiFreeImageBuffer(b);
	bytes = CGDataProviderCopyData(CGImageGetDataProvider(image));
	assert_non_null(bytes);
	assert_memory_equal(CFDataGetBytePtr(bytes), red, 4);
	assert_memory_equal(CFDataGetBytePtr(bytes) + 4, blue, 4);
	CFRelease(bytes);
	CGImageRelease(image);
	// The cropped image also keeps its provider alive after its parent is freed.
	{
		uiDrawContext output = canvas(1, 1);
		CGContextTranslateCTM(output.c, 0, 1);
		CGContextScaleCTM(output.c, 1, -1);
		CGContextDrawImage(output.c, CGRectMake(0, 0, 1, 1), cropped);
		pixel(&output, 0, 0, blue);
		CGContextRelease(output.c);
	}
	CGImageRelease(cropped);
}
#endif

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(orientationAlphaAndStride),
		cmocka_unit_test(croppedUpdatesAndState),
		cmocka_unit_test(submittedPDFDraws),
#ifdef _UI_STATIC
		cmocka_unit_test(retainedSnapshots),
#endif
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
