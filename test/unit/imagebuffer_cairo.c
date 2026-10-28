#include "unit.h"

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
