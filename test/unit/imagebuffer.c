#include "unit.h"

#include <stdint.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <string.h>

// Exercise allocation failures without adding a production allocator API.
static int failAllocation = -1;
static int allocations;
static int prepareFails;
static int diagnostics;
static void *testCalloc(size_t count, size_t size)
{
	void *p;
	if (failAllocation == 0)
		return NULL;
	if (failAllocation > 0)
		failAllocation--;
	p = calloc(count, size);
	if (p != NULL)
		allocations++;
	return p;
}
static void testFree(void *p)
{
	if (p != NULL)
		allocations--;
	free(p);
}
#define calloc testCalloc
#define free testFree
#include "../../common/imagebuffer.c"
#undef calloc
#undef free

void uiprivDoUserBug(const char *file, const char *line, const char *func,
	const char *format, ...)
{
	(void) file; (void) line; (void) func; (void) format;
	diagnostics++;
}

int uiprivImageBufferCreate(uiImageBuffer *buffer)
{
	buffer->pixels = testCalloc(buffer->height, buffer->stride);
	buffer->order = uiprivImageBufferRGBA;
	return buffer->pixels != NULL;
}
void uiprivImageBufferDestroy(uiImageBuffer *buffer)
{
	testFree(buffer->pixels);
}
int uiprivImageBufferPrepareUpdate(uiImageBuffer *buffer)
{
	(void) buffer;
	return !prepareFails;
}
void uiprivImageBufferUpdated(uiImageBuffer *buffer, int x, int y, int width, int height)
{
	(void) buffer;
}

static int drawCalls;
static uiDrawImageBufferParams lastDraw;
void uiDrawImageBufferWithParams(uiDrawContext *c, const uiImageBuffer *buffer,
	const uiDrawImageBufferParams *params)
{
	if (uiprivImageBufferDrawParamsValid(c, buffer, params)) {
		lastDraw = *params;
		drawCalls++;
	}
}

static void construction(void **state)
{
	uiImageBuffer *buffer;
	int w, h;
	(void) state;
	assert_null(uiNewImageBuffer(0, 1, uiImageBufferRGBA8Straight));
	assert_null(uiNewImageBuffer(1, -1, uiImageBufferRGBA8Straight));
	assert_null(uiNewImageBuffer(1, 1, (uiImageBufferFormat) 2));
	assert_null(uiNewImageBuffer(INT_MAX, INT_MAX, uiImageBufferRGBA8Straight));
	failAllocation = 0;
	assert_null(uiNewImageBuffer(2, 2, uiImageBufferRGBA8Straight));
	failAllocation = 1;
	assert_null(uiNewImageBuffer(2, 2, uiImageBufferRGBA8Straight));
	failAllocation = -1;
	assert_int_equal(allocations, 0);
	buffer = uiNewImageBuffer(3, 2, uiImageBufferRGBA8Straight);
	assert_non_null(buffer);
	uiImageBufferGetSize(buffer, &w, &h);
	assert_int_equal(w, 3);
	assert_int_equal(h, 2);
	assert_memory_equal(buffer->pixels, (unsigned char[24]) {0}, 24);
	uiImageBufferGetSize(NULL, &w, &h);
	uiImageBufferGetSize(buffer, NULL, &h);
	uiImageBufferGetSize(buffer, &w, NULL);
	assert_int_equal(diagnostics, 3);
	uiFreeImageBuffer(buffer);
	uiFreeImageBuffer(NULL);
	assert_int_equal(allocations, 0);
}

static void spans(void **state)
{
	size_t size;
	(void) state;
	assert_true(uiprivImageBufferInputSpan(2, 2, 12, &size));
	assert_int_equal(size, 20);
	assert_true(uiprivImageBufferInputSpan(3, 2, 16, &size));
	assert_int_equal(size, 28);
	assert_true(uiprivImageBufferInputSpan(2, 3, 12, &size));
	assert_int_equal(size, 32);
	assert_true(uiprivImageBufferInputSpan(1, 1, SIZE_MAX, &size));
	assert_int_equal(size, 4);
	assert_false(uiprivImageBufferInputSpan(2, 2, 7, &size));
	assert_false(uiprivImageBufferInputSpan(1, 2, SIZE_MAX, &size));
	assert_false(uiprivImageBufferInputSpan(1, 3, SIZE_MAX / 2, &size));
	assert_false(uiprivImageBufferInputSpan(-1, 2, 8, &size));
	assert_false(uiprivImageBufferInputSpan(1, 0, 4, &size));
	assert_false(uiprivImageBufferStorageSize(1, 2, SIZE_MAX / 2, &size));
}

static void conversion(void **state)
{
	unsigned char input[9], output[9];
	unsigned int component, alpha;
	uint32_t argb;
	(void) state;
	// Exhaust every alpha/component pair, including tie/edge rounding cases.
	for (alpha = 0; alpha <= 255; alpha++)
		for (component = 0; component <= 255; component++) {
			unsigned int expected = (component * alpha + 127) / 255;
			input[1] = (unsigned char) component;
			input[2] = 17;
			input[3] = 93;
			input[4] = (unsigned char) alpha;
			uiprivImageBufferConvert(output + 1, 4, input + 1, 4, 1, 1,
				uiImageBufferRGBA8Straight, uiprivImageBufferRGBA);
			assert_int_equal(output[1], expected);
			assert_int_equal(output[2], (17 * alpha + 127) / 255);
			assert_int_equal(output[3], (93 * alpha + 127) / 255);
			assert_int_equal(output[4], alpha);
			uiprivImageBufferConvert(output + 1, 4, input + 1, 4, 1, 1,
				uiImageBufferRGBA8Straight, uiprivImageBufferBGRA);
			assert_int_equal(output[1], (93 * alpha + 127) / 255);
			assert_int_equal(output[2], (17 * alpha + 127) / 255);
			assert_int_equal(output[3], expected);
			assert_int_equal(output[4], alpha);
			uiprivImageBufferConvert(output + 1, 4, input + 1, 4, 1, 1,
				uiImageBufferRGBA8Straight, uiprivImageBufferARGB32);
			memcpy(&argb, output + 1, 4);
			assert_int_equal(argb, (alpha << 24) | (expected << 16) |
				(((17 * alpha + 127) / 255) << 8) | ((93 * alpha + 127) / 255));
		}
	input[1] = 11; input[2] = 23; input[3] = 47; input[4] = 128;
	uiprivImageBufferConvert(output + 1, 4, input + 1, 4, 1, 1,
		uiImageBufferRGBA8Premultiplied, uiprivImageBufferBGRA);
	assert_memory_equal(output + 1, ((unsigned char[]) {47, 23, 11, 128}), 4);
	uiprivImageBufferConvert(output + 1, 4, input + 1, 4, 1, 1,
		uiImageBufferRGBA8Premultiplied, uiprivImageBufferARGB32);
	memcpy(&argb, output + 1, 4);
	assert_int_equal(argb, UINT32_C(0x800B172F));
}

static void conversionRows(void **state)
{
	const int widths[] = {1, 2, 3, 4, 7, 16, 17, 31, 32, 33, 63, 64, 65};
	size_t i;
	(void) state;
	for (i = 0; i < sizeof widths / sizeof widths[0]; i++) {
		int width = widths[i], row = width * 4, inputStride = row + 3, outputStride = row + 5;
		size_t inputSpan = (size_t) inputStride * 2 + row, outputSpan = (size_t) outputStride * 2 + row;
		unsigned char *input = malloc(inputSpan + 1), *output = malloc(outputSpan + 2);
		int format, order, x, y, channel;
		assert_non_null(input); assert_non_null(output);
		for (format = 0; format < 2; format++)
			for (order = 0; order < 3; order++) {
				memset(input, 0xCC, inputSpan + 1); memset(output, 0xEE, outputSpan + 2);
				for (y = 0; y < 3; y++)
					for (x = 0; x < width; x++) {
						unsigned char *p = input + 1 + (size_t) y * inputStride + x * 4;
						p[3] = (unsigned char) (x * 13 + y * 67);
						for (channel = 0; channel < 3; channel++) {
							unsigned int value = (unsigned char) (x * 37 + y * 11 + channel * 53);
							p[channel] = (unsigned char) (format ? value * p[3] / 255 : value);
						}
					}
				uiprivImageBufferConvert(output + 1, outputStride, input + 1, inputStride,
					width, 3, (uiImageBufferFormat) format, (uiprivImageBufferOrder) order);
				for (y = 0; y < 3; y++) {
					for (x = 0; x < width; x++) {
						const unsigned char *p = input + 1 + (size_t) y * inputStride + x * 4;
						const unsigned char *q = output + 1 + (size_t) y * outputStride + x * 4;
						unsigned char expected[4];
						uint32_t argb;
						for (channel = 0; channel < 3; channel++) {
							unsigned int value = p[channel];
							expected[channel] = (unsigned char) (format ? value : (value * p[3] + 127) / 255);
						}
						expected[3] = p[3];
						if (order == uiprivImageBufferARGB32) {
							argb = ((uint32_t) p[3] << 24) | ((uint32_t) expected[0] << 16) |
								((uint32_t) expected[1] << 8) | expected[2];
							assert_memory_equal(q, &argb, 4);
						} else {
							assert_int_equal(q[0], expected[order == uiprivImageBufferBGRA ? 2 : 0]);
							assert_int_equal(q[1], expected[1]);
							assert_int_equal(q[2], expected[order == uiprivImageBufferBGRA ? 0 : 2]);
							assert_int_equal(q[3], expected[3]);
						}
					}
					if (y < 2)
						for (x = row; x < outputStride; x++)
							assert_int_equal(output[1 + (size_t) y * outputStride + x], 0xEE);
				}
				assert_int_equal(output[0], 0xEE);
				assert_int_equal(output[outputSpan + 1], 0xEE);
			}
		free(input); free(output);
	}
}

static void updates(void **state)
{
	uiImageBuffer *buffer = uiNewImageBuffer(3, 2, uiImageBufferRGBA8Straight);
	unsigned char *input = malloc(29);
	unsigned char before[24];
	(void) state;
	assert_non_null(input);
	// Unaligned, padded rows, with exactly the readable final row allocated.
	memset(input, 255, 29);
	input[1] = 12; input[17] = 37;
	assert_true(uiImageBufferUpdate(buffer, input + 1, 28, 16));
	assert_int_equal(buffer->pixels[0], 12);
	assert_int_equal(buffer->pixels[12], 37);
	memcpy(before, buffer->pixels, 24);
	memset(input, 0, 29);
	assert_memory_equal(buffer->pixels, before, 24);
	assert_false(uiImageBufferUpdate(buffer, input + 1, 27, 16));
	assert_false(uiImageBufferUpdate(buffer, input, 29, 11));
	assert_false(uiImageBufferUpdate(buffer, input, SIZE_MAX, SIZE_MAX));
	assert_false(uiImageBufferUpdate(buffer, NULL, 28, 16));
	assert_false(uiImageBufferUpdate(NULL, input, 28, 16));
	prepareFails = 1;
	assert_false(uiImageBufferUpdate(buffer, input, 24, 12));
	prepareFails = 0;
	assert_memory_equal(buffer->pixels, before, 24);
	free(input);
	assert_memory_equal(buffer->pixels, before, 24);
	uiFreeImageBuffer(buffer);
	assert_int_equal(allocations, 0);
}

static void rectangleUpdates(void **state)
{
	const int rectangles[][4] = {{0, 0, 1, 1}, {4, 3, 1, 1},
		{1, 0, 3, 2}, {0, 1, 2, 3}, {0, 0, 5, 4}};
	const int invalid[][4] = {{-1, 0, 1, 1}, {0, -1, 1, 1},
		{5, 0, 1, 1}, {0, 4, 1, 1}, {4, 3, 2, 1}, {4, 3, 1, 2},
		{0, 0, 0, 1}, {0, 0, 1, -1}, {INT_MAX, 0, 1, 1},
		{0, 0, INT_MAX, 1}, {0, INT_MAX, 1, INT_MAX}};
	unsigned char base[80], before[80];
	int format, order, x, y;
	size_t i;
	(void) state;
	for (i = 0; i < 20; i++) {
		base[i * 4] = 30; base[i * 4 + 1] = 40;
		base[i * 4 + 2] = 50; base[i * 4 + 3] = 255;
	}
	for (format = 0; format < 2; format++)
		for (order = 0; order < 3; order++) {
			uiImageBuffer *buffer = uiNewImageBuffer(5, 4, (uiImageBufferFormat) format);
			assert_non_null(buffer);
			buffer->order = (uiprivImageBufferOrder) order;
			for (i = 0; i < sizeof rectangles / sizeof rectangles[0]; i++) {
				int rx = rectangles[i][0], ry = rectangles[i][1];
				int rw = rectangles[i][2], rh = rectangles[i][3];
				size_t stride = (size_t) rw * 4 + 3;
				size_t length = (size_t) (rh - 1) * stride + (size_t) rw * 4;
				unsigned char *input = malloc(length + 1);
				assert_non_null(input);
				assert_true(uiImageBufferUpdate(buffer, base, sizeof base, 20));
				memcpy(before, buffer->pixels, sizeof before);
				memset(input, 0xEE, length + 1);
				for (y = 0; y < rh; y++)
					for (x = 0; x < rw; x++) {
						unsigned char *p = input + 1 + (size_t) y * stride + (size_t) x * 4;
						p[0] = 60; p[1] = 100; p[2] = 20;
						p[3] = (unsigned char) (((x + y) % 3) == 0 ? 0 :
							((x + y) % 3) == 1 ? 128 : 255);
						if (p[3] == 0)
							p[0] = p[1] = p[2] = 0;
					}
				assert_false(uiImageBufferUpdateRect(buffer, rx, ry, rw, rh, input + 1, length - 1, stride));
				assert_memory_equal(before, buffer->pixels, sizeof before);
				assert_true(uiImageBufferUpdateRect(buffer, rx, ry, rw, rh, input + 1, length, stride));
				memset(input, 0, length + 1);
				free(input);
				for (y = 0; y < 4; y++)
					for (x = 0; x < 5; x++) {
						unsigned char expected[4];
						unsigned int r = 30, g = 40, b = 50, a = 255;
						uint32_t argb;
						if (x >= rx && x < rx + rw && y >= ry && y < ry + rh) {
							int phase = (x - rx + y - ry) % 3;
							a = phase == 0 ? 0 : phase == 1 ? 128 : 255;
							r = a ? 60 : 0; g = a ? 100 : 0; b = a ? 20 : 0;
							if (format == uiImageBufferRGBA8Straight) {
								r = (r * a + 127) / 255;
								g = (g * a + 127) / 255;
								b = (b * a + 127) / 255;
							}
						}
						if (order == uiprivImageBufferARGB32) {
							argb = (a << 24) | (r << 16) | (g << 8) | b;
							memcpy(expected, &argb, 4);
						} else {
							expected[0] = (unsigned char) (order == uiprivImageBufferBGRA ? b : r);
							expected[1] = (unsigned char) g;
							expected[2] = (unsigned char) (order == uiprivImageBufferBGRA ? r : b);
							expected[3] = (unsigned char) a;
						}
						assert_memory_equal(buffer->pixels + (y * 5 + x) * 4, expected, 4);
					}
			}
			memcpy(before, buffer->pixels, sizeof before);
			for (i = 0; i < sizeof invalid / sizeof invalid[0]; i++)
				assert_false(uiImageBufferUpdateRect(buffer, invalid[i][0], invalid[i][1],
					invalid[i][2], invalid[i][3], base, sizeof base, 20));
			assert_false(uiImageBufferUpdateRect(buffer, 0, 0, 2, 2, base, sizeof base, 7));
			assert_false(uiImageBufferUpdateRect(buffer, 0, 0, 2, 2, base, SIZE_MAX, SIZE_MAX));
			assert_false(uiImageBufferUpdateRect(buffer, 0, 0, 1, 1, NULL, 4, 4));
			prepareFails = 1;
			assert_false(uiImageBufferUpdateRect(buffer, 1, 1, 1, 1, base, 4, 4));
			prepareFails = 0;
			assert_memory_equal(before, buffer->pixels, sizeof before);
			uiFreeImageBuffer(buffer);
		}
	assert_false(uiImageBufferUpdateRect(NULL, 0, 0, 1, 1, base, 4, 4));
	assert_int_equal(allocations, 0);
}

static void drawArguments(void **state)
{
	uiImageBuffer *b = uiNewImageBuffer(1, 1, uiImageBufferRGBA8Straight);
	// The helper never dereferences a context.
	uiDrawContext *c = (uiDrawContext *) b;
	(void) state;
	assert_true(uiprivImageBufferDrawValid(c, b, -1, 2, 3, 4, uiImageBufferInterpolationNearest));
	assert_false(uiprivImageBufferDrawValid(NULL, b, 0, 0, 1, 1, 0));
	assert_false(uiprivImageBufferDrawValid(c, NULL, 0, 0, 1, 1, 0));
	assert_false(uiprivImageBufferDrawValid(c, b, NAN, 0, 1, 1, 0));
	assert_false(uiprivImageBufferDrawValid(c, b, 0, 0, INFINITY, 1, 0));
	assert_false(uiprivImageBufferDrawValid(c, b, DBL_MAX, 0, DBL_MAX, 1, 0));
	assert_false(uiprivImageBufferDrawValid(c, b, DBL_MAX, 0, 1, 1, 0));
	assert_false(uiprivImageBufferDrawValid(c, b, 0, 0, 0, 1, 0));
	assert_false(uiprivImageBufferDrawValid(c, b, 0, 0, 1, 1, (uiImageBufferInterpolation) 2));
	uiFreeImageBuffer(b);
}

static void drawParameters(void **state)
{
	uiImageBuffer *buffer = uiNewImageBuffer(3, 2, uiImageBufferRGBA8Straight);
	uiDrawContext *context = (uiDrawContext *) buffer;
	uiDrawImageBufferParams p = {sizeof p, 1, 0, 2, 2, 3, 5, 8, 6, uiImageBufferInterpolationNearest};
	size_t *shortParams = malloc(sizeof *shortParams);
	(void) state;
	assert_non_null(shortParams);
	*shortParams = sizeof *shortParams;
	assert_false(uiprivImageBufferDrawParamsValid(context, buffer, (const uiDrawImageBufferParams *) shortParams));
	free(shortParams);
	assert_true(uiprivImageBufferDrawParamsValid(context, buffer, &p));
	assert_false(uiprivImageBufferDrawParamsValid(context, buffer, NULL));
	assert_false(uiprivImageBufferDrawParamsValid(NULL, buffer, &p));
	assert_false(uiprivImageBufferDrawParamsValid(context, NULL, &p));
	p.Size = offsetof(uiDrawImageBufferParams, Interpolation) + sizeof p.Interpolation - 1;
	assert_false(uiprivImageBufferDrawParamsValid(context, buffer, &p));
	p.Size++;
	assert_true(uiprivImageBufferDrawParamsValid(context, buffer, &p));
	p.Size = SIZE_MAX;
	assert_true(uiprivImageBufferDrawParamsValid(context, buffer, &p));
	p.SourceX = -1;
	assert_false(uiprivImageBufferDrawParamsValid(context, buffer, &p));
	p.SourceX = 2;
	assert_false(uiprivImageBufferDrawParamsValid(context, buffer, &p));
	p.SourceX = 1; p.SourceY = 1;
	assert_false(uiprivImageBufferDrawParamsValid(context, buffer, &p));
	p.SourceY = 0; p.SourceWidth = 0;
	assert_false(uiprivImageBufferDrawParamsValid(context, buffer, &p));
	p.SourceWidth = INT_MAX;
	assert_false(uiprivImageBufferDrawParamsValid(context, buffer, &p));
	p.SourceWidth = 2; p.X = NAN;
	assert_false(uiprivImageBufferDrawParamsValid(context, buffer, &p));
	p.X = 3; p.Interpolation = (uiImageBufferInterpolation) 2;
	assert_false(uiprivImageBufferDrawParamsValid(context, buffer, &p));
	drawCalls = 0;
	uiDrawImageBuffer(context, buffer, 3, 5, 8, 6, uiImageBufferInterpolationNearest);
	assert_int_equal(drawCalls, 1);
	assert_int_equal(lastDraw.Size, sizeof lastDraw);
	assert_int_equal(lastDraw.SourceX, 0);
	assert_int_equal(lastDraw.SourceY, 0);
	assert_int_equal(lastDraw.SourceWidth, 3);
	assert_int_equal(lastDraw.SourceHeight, 2);
	assert_true(lastDraw.X == 3 && lastDraw.Y == 5 && lastDraw.Width == 8 && lastDraw.Height == 6);
	assert_int_equal(lastDraw.Interpolation, uiImageBufferInterpolationNearest);
	uiDrawImageBuffer(context, NULL, 0, 0, 1, 1, uiImageBufferInterpolationSmooth);
	assert_int_equal(drawCalls, 1);
	uiFreeImageBuffer(buffer);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(construction), cmocka_unit_test(spans),
		cmocka_unit_test(conversion), cmocka_unit_test(conversionRows), cmocka_unit_test(updates),
		cmocka_unit_test(rectangleUpdates), cmocka_unit_test(drawArguments),
		cmocka_unit_test(drawParameters),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
