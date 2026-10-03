// 3 october 2026
#include "../ui.h"
#include "uipriv.h"

#include <stdint.h>

enum {
	maxGridRows = 10000,
	maxGridColumns = 10000,
	maxGridCells = 100000,
};

int uiprivGridDimensions(int xmin, int ymin, int xmax, int ymax,
	int *xcount, int *ycount, int *cellCount)
{
	int64_t width;
	int64_t height;
	int64_t cells;

	width = ((int64_t) xmax) - ((int64_t) xmin);
	height = ((int64_t) ymax) - ((int64_t) ymin);
	if (width <= 0 || height <= 0)
		return 0;
	if (width > maxGridColumns || height > maxGridRows)
		return 0;
	cells = width * height;
	if (cells > maxGridCells)
		return 0;
	*xcount = (int) width;
	*ycount = (int) height;
	*cellCount = (int) cells;
	return 1;
}
