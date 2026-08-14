#include "../ui.h"
#include "uipriv.h"

void uiprivImageViewComputeRect(double viewWidth, double viewHeight,
	double imageWidth, double imageHeight, uiImageViewContentMode mode,
	double *x, double *y, double *width, double *height)
{
	double scaleX, scaleY, scale;

	*x = 0;
	*y = 0;
	*width = 0;
	*height = 0;
	if (viewWidth <= 0 || viewHeight <= 0 ||
		imageWidth <= 0 || imageHeight <= 0)
		return;

	if (mode == uiImageViewContentCenter) {
		*width = imageWidth;
		*height = imageHeight;
	} else {
		scaleX = viewWidth / imageWidth;
		scaleY = viewHeight / imageHeight;
		scale = scaleX < scaleY ? scaleX : scaleY;
		*width = imageWidth * scale;
		*height = imageHeight * scale;
	}
	*x = (viewWidth - *width) / 2;
	*y = (viewHeight - *height) / 2;
}
