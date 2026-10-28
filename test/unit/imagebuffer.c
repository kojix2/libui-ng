#include "unit.h"

// Build with a mock backend for failure injection, or against the real Cairo backend.
#ifdef IMAGEBUFFER_CAIRO_TEST
#include <math.h>
#include <float.h>
#include <string.h>
#include <gtk/gtk.h>
#include "../../common/uipriv.h"
#include "../../unix/draw.h"

static uint32_t pixel(cairo_surface_t *surface, int x, int y)
{
	uint32_t value;
	cairo_surface_flush(surface);
	memcpy(&value, cairo_image_surface_get_data(surface) +
		y * cairo_image_surface_get_stride(surface) + x * 4, 4);
	return value;
}

static void submittedDraws(void **state)
{
	cairo_surface_t *recording = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, NULL);
	cairo_t *cr = cairo_create(recording);
	uiDrawContext context = {cr, NULL, NULL};
	uiImageBuffer *buffer = uiNewImageBuffer(1, 1, uiImageBufferRGBA8Straight);
	unsigned char red[] = {255, 0, 0, 255};
	unsigned char blue[] = {0, 0, 255, 255};
	uiImageBuffer *patch = uiNewImageBuffer(3, 2, uiImageBufferRGBA8Straight);
	uiDrawImageBufferParams crop = {sizeof crop, 2, 1, 1, 1, 128, 0, 1, 1,
		uiImageBufferInterpolationNearest};
	unsigned char clear[24] = {0};
	cairo_surface_t *output;
	cairo_t *replay;
	int i;
	(void) state;
	for (i = 0; i < 64; i++) {
		assert_true(uiImageBufferUpdate(buffer, red, 4, 4));
		uiDrawImageBuffer(&context, buffer, i * 2, 0, 1, 1, uiImageBufferInterpolationNearest);
		assert_true(uiImageBufferUpdate(buffer, blue, 4, 4));
		uiDrawImageBuffer(&context, buffer, i * 2 + 1, 0, 1, 1, uiImageBufferInterpolationNearest);
	}
	assert_non_null(patch);
	assert_true(uiImageBufferUpdateRect(patch, 2, 1, 1, 1, red, 4, 4));
	uiDrawImageBufferWithParams(&context, patch, &crop);
	assert_true(uiImageBufferUpdateRect(patch, 2, 1, 1, 1, blue, 4, 4));
	crop.X = 129;
	uiDrawImageBufferWithParams(&context, patch, &crop);
	assert_true(uiImageBufferUpdateRect(patch, 0, 0, 3, 2, clear, sizeof clear, 12));
	uiFreeImageBuffer(patch);
	uiFreeImageBuffer(buffer);
	assert_int_equal(cairo_status(cr), CAIRO_STATUS_SUCCESS);
	cairo_destroy(cr);
	// Replay only after all updates and Free, and replay in two destinations.
	for (i = 0; i < 2; i++) {
		int x;
		output = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 130, 1);
		replay = cairo_create(output);
		cairo_set_source_surface(replay, recording, 0, 0);
		cairo_paint(replay);
		for (x = 0; x < 128; x++)
			assert_int_equal(pixel(output, x, 0), x % 2 ? 0xFF0000FF : 0xFFFF0000);
		assert_int_equal(pixel(output, 128, 0), 0xFFFF0000);
		assert_int_equal(pixel(output, 129, 0), 0xFF0000FF);
		cairo_destroy(replay);
		cairo_surface_destroy(output);
	}
	cairo_surface_destroy(recording);
}

static void orientationAlphaAndStride(void **state)
{
	unsigned char *input = malloc(21);
	const unsigned char fixture[] = {
		255, 0, 0, 255, 0, 255, 0, 128,
		9, 9, 9, 9,
		0, 0, 255, 255, 255, 255, 255, 0
	};
	cairo_surface_t *output = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 4, 4);
	cairo_t *cr = cairo_create(output);
	uiDrawContext context = {cr, NULL, NULL};
	uiImageBuffer *buffer = uiNewImageBuffer(2, 2, uiImageBufferRGBA8Straight);
	(void) state;
	uiDrawImageBuffer(&context, buffer, 0, 0, 4, 4, uiImageBufferInterpolationNearest);
	assert_int_equal(pixel(output, 0, 0), 0);
	memcpy(input + 1, fixture, sizeof fixture);
	assert_true(uiImageBufferUpdate(buffer, input + 1, 20, 12));
	memset(input, 0, 21);
	free(input);
	assert_false(uiImageBufferUpdate(buffer, fixture, 19, 12));
	uiDrawImageBuffer(&context, buffer, 0, 0, 4, 4, uiImageBufferInterpolationNearest);
	assert_int_equal(pixel(output, 0, 0), 0xFFFF0000);
	assert_int_equal(pixel(output, 3, 0), 0x80008000);
	assert_int_equal(pixel(output, 0, 3), 0xFF0000FF);
	assert_int_equal(pixel(output, 3, 3), 0);
	uiFreeImageBuffer(buffer);
	buffer = uiNewImageBuffer(1, 1, uiImageBufferRGBA8Premultiplied);
	assert_true(uiImageBufferUpdate(buffer, (unsigned char[]) {64, 32, 16, 128}, 4, 4));
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(cr, 0, 0, 1, 1);
	cairo_paint(cr);
	uiDrawImageBuffer(&context, buffer, 0, 0, 4, 4, uiImageBufferInterpolationNearest);
	assert_int_equal(pixel(output, 2, 2), 0xFF40208F);
	assert_int_equal(cairo_get_operator(cr), CAIRO_OPERATOR_SOURCE);
	uiFreeImageBuffer(buffer);
	cairo_destroy(cr);
	cairo_surface_destroy(output);
}

static void rectangularDimensions(void **state)
{
	const struct { int width, height, sx, sy, sw, sh, drawWidth, drawHeight; } cases[] = {
		{3, 2, 0, 0, 3, 2, 9, 4}, {2, 3, 0, 0, 2, 3, 4, 9},
		{1, 5, 0, 0, 1, 5, 3, 10}, {5, 1, 0, 0, 5, 1, 10, 3},
		{5, 4, 1, 1, 3, 2, 9, 4}, {4, 5, 1, 1, 2, 3, 4, 9},
		{5, 4, 4, 3, 1, 1, 3, 2}, {5, 4, 0, 1, 1, 3, 2, 6},
		{5, 4, 1, 0, 4, 1, 8, 3}, {5, 4, 1, 1, 3, 3, 1, 1}
	};
	size_t i;
	(void) state;
	for (i = 0; i < sizeof cases / sizeof cases[0]; i++) {
		int w = cases[i].width, h = cases[i].height;
		int dw = cases[i].drawWidth, dh = cases[i].drawHeight;
		size_t stride = (size_t) w * 4 + 7;
		size_t length = (size_t) (h - 1) * stride + (size_t) w * 4;
		unsigned char *input = malloc(length + 1);
		uiImageBuffer *buffer = uiNewImageBuffer(w, h, uiImageBufferRGBA8Straight);
		cairo_surface_t *output = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, dw + 6, dh + 10);
		cairo_t *cr = cairo_create(output);
		uiDrawContext context = {cr, NULL, NULL};
		uiDrawImageBufferParams p = {sizeof p, cases[i].sx, cases[i].sy,
			cases[i].sw, cases[i].sh, 3, 5, dw, dh, uiImageBufferInterpolationNearest};
		unsigned char patch[] = {200, 30, 90, 255};
		int px = p.SourceX + p.SourceWidth - 1, py = p.SourceY + p.SourceHeight - 1;
		int x, y, width, height;

		assert_non_null(input);
		assert_non_null(buffer);
		uiImageBufferGetSize(buffer, &width, &height);
		assert_int_equal(width, w);
		assert_int_equal(height, h);
		memset(input, 0xEE, length + 1);
		for (y = 0; y < h; y++)
			for (x = 0; x < w; x++) {
				unsigned char *p = input + 1 + (size_t) y * stride + (size_t) x * 4;
				p[0] = (unsigned char) (11 + x * 29 + y * 7);
				p[1] = (unsigned char) (19 + x * 13 + y * 31);
				p[2] = (unsigned char) (23 + x * 17 + y * 37);
				p[3] = 255;
			}
		assert_false(uiImageBufferUpdate(buffer, input + 1, length - 1, stride));
		assert_true(uiImageBufferUpdate(buffer, input + 1, length, stride));
		memset(input, 0, length + 1);
		free(input);
		assert_true(uiImageBufferUpdateRect(buffer, px, py, 1, 1, patch, sizeof patch, 4));
		if (p.SourceX == 0 && p.SourceY == 0 && p.SourceWidth == w && p.SourceHeight == h)
			uiDrawImageBuffer(&context, buffer, 3, 5, dw, dh, uiImageBufferInterpolationNearest);
		else
			uiDrawImageBufferWithParams(&context, buffer, &p);
		for (y = 0; y < dh + 10; y++)
			for (x = 0; x < dw + 6; x++) {
				uint32_t expected = 0;
				if (x >= 3 && x < 3 + dw && y >= 5 && y < 5 + dh) {
					// Map destination pixel centers into the source, including downscales.
					int sx = p.SourceX + (2 * (x - 3) + 1) * p.SourceWidth / (2 * dw);
					int sy = p.SourceY + (2 * (y - 5) + 1) * p.SourceHeight / (2 * dh);
					expected = UINT32_C(0xFF000000) |
						((uint32_t) (11 + sx * 29 + sy * 7) << 16) |
						((uint32_t) (19 + sx * 13 + sy * 31) << 8) |
						(uint32_t) (23 + sx * 17 + sy * 37);
					if (sx == px && sy == py)
						expected = UINT32_C(0xFFC81E5A);
				}
				assert_int_equal(pixel(output, x, y), expected);
			}
		// A size-only allocation detects reads of fields before validating Size.
		{
			size_t *shortParams = malloc(sizeof *shortParams);
			assert_non_null(shortParams);
			*shortParams = sizeof *shortParams;
			uiDrawImageBufferWithParams(&context, buffer, (const uiDrawImageBufferParams *) shortParams);
			free(shortParams);
		}
		p.Size = SIZE_MAX;
		uiDrawImageBufferWithParams(&context, buffer, &p);
		p.SourceWidth = INT_MAX;
		uiDrawImageBufferWithParams(&context, buffer, &p);
		uiDrawImageBufferWithParams(&context, buffer, NULL);
		assert_int_equal(cairo_status(cr), CAIRO_STATUS_SUCCESS);
		uiFreeImageBuffer(buffer);
		cairo_destroy(cr);
		cairo_surface_destroy(output);
	}
}

static void drawingState(void **state)
{
	cairo_surface_t *output = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 12, 12);
	cairo_t *cr = cairo_create(output);
	uiDrawContext context = {cr, NULL, NULL};
	uiImageBuffer *buffer = uiNewImageBuffer(3, 2, uiImageBufferRGBA8Straight);
	uiDrawImageBufferParams crop = {sizeof crop, 1, 0, 2, 2, 0, 0, 4, 4,
		uiImageBufferInterpolationSmooth};
	unsigned char red[24];
	cairo_matrix_t before, after;
	cairo_path_t *pathBefore, *pathAfter;
	size_t i;
	(void) state;
	for (i = 0; i < sizeof red; i += 4) {
		red[i] = red[i + 3] = 255;
		red[i + 1] = red[i + 2] = 0;
	}
	assert_true(uiImageBufferUpdate(buffer, red, sizeof red, 12));
	cairo_translate(cr, 2, 3);
	cairo_scale(cr, 2, 2);
	cairo_rectangle(cr, 0, 0, 2, 2);
	cairo_clip(cr);
	cairo_move_to(cr, 0.25, 0.5);
	cairo_line_to(cr, 1, 1.5);
	pathBefore = cairo_copy_path(cr);
	cairo_get_matrix(cr, &before);
	uiDrawImageBufferWithParams(&context, buffer, &crop);
	uiDrawImageBuffer(&context, buffer, 1, 1, 4, 4, uiImageBufferInterpolationNearest);
	cairo_get_matrix(cr, &after);
	assert_memory_equal(&before, &after, sizeof before);
	pathAfter = cairo_copy_path(cr);
	assert_int_equal(pathBefore->num_data, pathAfter->num_data);
	// Compare values; cairo_path_data_t unions may contain unspecified padding.
	assert_true(pathBefore->data[1].point.x == pathAfter->data[1].point.x);
	assert_true(pathBefore->data[1].point.y == pathAfter->data[1].point.y);
	assert_true(pathBefore->data[3].point.x == pathAfter->data[3].point.x);
	assert_true(pathBefore->data[3].point.y == pathAfter->data[3].point.y);
	cairo_path_destroy(pathBefore);
	cairo_path_destroy(pathAfter);
	assert_int_equal(pixel(output, 2, 3), 0xFFFF0000);
	assert_int_equal(pixel(output, 5, 6), 0xFFFF0000);
	assert_int_equal(pixel(output, 6, 3), 0);
	uiDrawImageBuffer(&context, buffer, 0, 0, DBL_MIN, 1, uiImageBufferInterpolationNearest);
	uiDrawImageBuffer(&context, buffer, 0, 0, DBL_MIN, DBL_MIN, uiImageBufferInterpolationNearest);
	uiDrawImageBuffer(&context, buffer, 0, 0, DBL_MAX, DBL_MAX, uiImageBufferInterpolationNearest);
	uiDrawImageBuffer(&context, buffer, DBL_MAX, DBL_MAX, 1, 1, uiImageBufferInterpolationNearest);
	uiDrawImageBuffer(&context, buffer, -DBL_MAX, -DBL_MAX, DBL_MAX, DBL_MAX, uiImageBufferInterpolationNearest);
	uiDrawImageBuffer(&context, buffer, NAN, 0, 1, 1, uiImageBufferInterpolationNearest);
	assert_int_equal(cairo_status(cr), CAIRO_STATUS_SUCCESS);
	uiFreeImageBuffer(buffer);
	cairo_destroy(cr);
	cairo_surface_destroy(output);
}

static void continuousUpdates(void **state)
{
	cairo_surface_t *output = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 16, 16);
	cairo_t *cr = cairo_create(output);
	uiDrawContext context = {cr, NULL, NULL};
	uiImageBuffer *buffer = uiNewImageBuffer(1, 1, uiImageBufferRGBA8Straight);
	unsigned char input[] = {0, 32, 64, 255};
	unsigned int references;
	int i;
	(void) state;
	references = cairo_surface_get_reference_count((cairo_surface_t *) buffer->native);
	for (i = 0; i < 4096; i++) {
		input[0] = (unsigned char) i;
		assert_true(uiImageBufferUpdate(buffer, input, 4, 4));
		uiDrawImageBuffer(&context, buffer, 0, 0, 16, 16, uiImageBufferInterpolationNearest);
	}
	assert_int_equal(pixel(output, 8, 8), 0xFFFF2040);
	assert_int_equal(cairo_surface_get_reference_count((cairo_surface_t *) buffer->native), references);
	uiFreeImageBuffer(buffer);
	cairo_destroy(cr);
	cairo_surface_destroy(output);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(submittedDraws),
		cmocka_unit_test(orientationAlphaAndStride),
		cmocka_unit_test(rectangularDimensions),
		cmocka_unit_test(drawingState),
		cmocka_unit_test(continuousUpdates),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}

#else
#include <stdint.h>
#include <limits.h>
#include <float.h>
#include <limits.h>
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
void uiprivImageBufferUpdated(uiImageBuffer *buffer)
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
		cmocka_unit_test(conversion), cmocka_unit_test(updates),
		cmocka_unit_test(rectangleUpdates), cmocka_unit_test(drawArguments),
		cmocka_unit_test(drawParameters),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}

#endif
