// Headless benchmark of the GTK/Cairo or macOS/CoreGraphics backend.
// Configure with -DCMAKE_BUILD_TYPE=Release -DLIBUI_BUILD_BENCHMARKS=ON,
// build the imagebuffer-bench target, then run artifacts/Release/imagebuffer-bench.
// LIBUI_BUILD_TESTS=OFF avoids requiring cmocka for benchmark-only builds.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __APPLE__
#include <CoreGraphics/CoreGraphics.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#else
#include <gtk/gtk.h>
#endif
#include "../../ui.h"
#ifdef __APPLE__
#include "../../darwin/draw.h"

static double monotonicMicroseconds(void)
{
	mach_timebase_info_data_t timebase;
	mach_timebase_info(&timebase);
	return (double) mach_absolute_time() * timebase.numer / timebase.denom / 1000;
}
#else
#include "../../unix/draw.h"

static double monotonicMicroseconds(void)
{
	return (double) g_get_monotonic_time();
}
#endif

struct benchmark {
	uiImageBuffer *buffer;
	unsigned char *input;
	size_t stride, length;
	int patchWidth, patchHeight;
	int update, draw;
	uiDrawContext context;
	uiDrawImageBufferParams params;
};

static double sample(struct benchmark *b, int iterations)
{
	double start = monotonicMicroseconds();
	int i;

	for (i = 0; i < iterations; i++) {
		if (b->update && !uiImageBufferUpdateRect(b->buffer, 0, 0,
			b->patchWidth, b->patchHeight, b->input + 1, b->length, b->stride)) {
			fprintf(stderr, "ImageBuffer update failed\n");
			exit(1);
		}
		if (b->draw)
			uiDrawImageBufferWithParams(&b->context, b->buffer, &b->params);
	}
#ifdef __APPLE__
	if (b->draw)
		CGContextFlush(b->context.c);
#endif
	return monotonicMicroseconds() - start;
}

static int compare(const void *a, const void *b)
{
	double x = *(const double *) a, y = *(const double *) b;

	return (x > y) - (x < y);
}

static void run(const char *name, int width, int height, int pw, int ph,
	uiImageBufferFormat format, int update, int draw, double scale,
	uiImageBufferInterpolation interpolation)
{
	struct benchmark b = {0};
#ifndef __APPLE__
	cairo_surface_t *output = NULL;
#endif
	double samples[5];
	int iterations = 1, i, x, y;

	b.buffer = uiNewImageBuffer(width, height, format);
	b.stride = (size_t) width * 4 + 13;
	b.length = (size_t) (height - 1) * b.stride + (size_t) width * 4;
	b.input = malloc(b.length + 1);
	if (b.buffer == NULL || b.input == NULL) {
		fprintf(stderr, "ImageBuffer benchmark allocation failed\n");
		exit(1);
	}
	// Vary alpha and color; include unaligned input and padded rows.
	for (y = 0; y < height; y++)
		for (x = 0; x < width; x++) {
			unsigned char *p = b.input + 1 + (size_t) y * b.stride + (size_t) x * 4;
			unsigned int alpha = (unsigned char) (x * 13 + y * 7);
			int c;
			for (c = 0; c < 3; c++) {
				unsigned int value = (unsigned char) (x * 37 + y * 11 + c * 53);
				p[c] = (unsigned char) (format == uiImageBufferRGBA8Straight ?
					value : value * alpha / 255);
			}
			p[3] = (unsigned char) alpha;
		}
	if (!uiImageBufferUpdate(b.buffer, b.input + 1, b.length, b.stride))
		exit(1);
	if (draw) {
#ifdef __APPLE__
		CGColorSpaceRef color = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
		b.context.c = CGBitmapContextCreate(NULL, (size_t) (width * scale),
			(size_t) (height * scale), 8, 0, color,
			kCGBitmapByteOrder32Big | kCGImageAlphaPremultipliedLast);
		CGColorSpaceRelease(color);
		if (b.context.c == NULL)
			exit(1);
		CGContextTranslateCTM(b.context.c, 0, height * scale);
		CGContextScaleCTM(b.context.c, 1, -1);
#else
		output = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
			(int) (width * scale), (int) (height * scale));
		b.context.cr = cairo_create(output);
#endif
		b.params = (uiDrawImageBufferParams) {sizeof b.params, 0, 0, width, height,
			0, 0, width * scale, height * scale, interpolation};
	}
	b.patchWidth = pw;
	b.patchHeight = ph;
	b.update = update;
	b.draw = draw;
	// Calibrate to >= 50 ms, then report the median of five samples in us/op.
	while (sample(&b, iterations) < 50000 && iterations < 1048576)
		iterations *= 2;
	for (i = 0; i < 5; i++)
		samples[i] = sample(&b, iterations) / iterations;
	qsort(samples, 5, sizeof samples[0], compare);
#ifndef __APPLE__
	if (draw && (cairo_status(b.context.cr) != CAIRO_STATUS_SUCCESS ||
		cairo_surface_status(output) != CAIRO_STATUS_SUCCESS))
		exit(1);
#endif
	printf("%-24s %-13s %8.2f us/op\n", name,
		format == uiImageBufferRGBA8Straight ? "straight" : "premultiplied", samples[2]);
	if (draw) {
#ifdef __APPLE__
		CGContextRelease(b.context.c);
#else
		cairo_destroy(b.context.cr);
		cairo_surface_destroy(output);
#endif
	}
	uiFreeImageBuffer(b.buffer);
	free(b.input);
}

#ifdef __APPLE__
static size_t residentBytes(void)
{
	mach_task_basic_info_data_t info;
	mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
	if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t) &info, &count) != KERN_SUCCESS)
		exit(1);
	return info.resident_size;
}

static int memory(void)
{
	// Run separately from timings: touch CPU storage, then submit one 4K image
	// to a tiny bitmap. The delta includes any native image/cache allocations.
	CGColorSpaceRef color = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
	uiDrawContext c = {CGBitmapContextCreate(NULL, 1, 1, 8, 4, color,
		kCGBitmapByteOrder32Big | kCGImageAlphaPremultipliedLast), 1};
	uiImageBuffer *buffer;
	size_t before, updated, drawn;
	CGColorSpaceRelease(color);
	if (c.c == NULL)
		return 1;
	before = residentBytes();
	buffer = uiNewImageBuffer(3840, 2160, uiImageBufferRGBA8Premultiplied);
	if (buffer == NULL)
		return 1;
	// Internal storage is deliberately not accessed: use a small reusable row.
	{
		unsigned char row[3840 * 4];
		int y;
		memset(row, 255, sizeof row);
		for (y = 0; y < 2160; y++)
			if (!uiImageBufferUpdateRect(buffer, 0, y, 3840, 1, row, sizeof row, sizeof row))
				return 1;
	}
	updated = residentBytes();
	uiDrawImageBuffer(&c, buffer, 0, 0, 3840, 2160, uiImageBufferInterpolationNearest);
	CGContextFlush(c.c);
	drawn = residentBytes();
	printf("4K resident delta: updated %.2f MiB, drawn %.2f MiB, draw overhead %.2f MiB\n",
		((double) updated - before) / 1048576, ((double) drawn - before) / 1048576,
		((double) drawn - updated) / 1048576);
	uiFreeImageBuffer(buffer);
	CGContextRelease(c.c);
	return 0;
}
#endif

int main(int argc, char **argv)
{
	int format;

#ifdef __APPLE__
	if (argc == 2 && strcmp(argv[1], "--memory") == 0)
		return memory();
	printf("macOS ImageBuffer, CoreGraphics; median of 5 samples\n");
#else
	(void) argc;
	(void) argv;
	printf("GTK ImageBuffer, Cairo %s; median of 5 samples\n", cairo_version_string());
#endif
	for (format = 0; format < 2; format++) {
		run("update 640x480", 640, 480, 640, 480, format, 1, 0, 1, uiImageBufferInterpolationNearest);
		run("update 1920x1080", 1920, 1080, 1920, 1080, format, 1, 0, 1, uiImageBufferInterpolationNearest);
		run("update 3840x2160", 3840, 2160, 3840, 2160, format, 1, 0, 1, uiImageBufferInterpolationNearest);
		run("patch 64x64 in 1080p", 1920, 1080, 64, 64, format, 1, 0, 1, uiImageBufferInterpolationNearest);
		run("draw 1080p nearest", 1920, 1080, 1920, 1080, format, 0, 1, 1, uiImageBufferInterpolationNearest);
		run("draw 1080p smooth half", 1920, 1080, 1920, 1080, format, 0, 1, 0.5, uiImageBufferInterpolationSmooth);
		run("update+draw 1080p", 1920, 1080, 1920, 1080, format, 1, 1, 1, uiImageBufferInterpolationNearest);
		run("patch+draw 1080p", 1920, 1080, 64, 64, format, 1, 1, 1, uiImageBufferInterpolationNearest);
	}
	return 0;
}
