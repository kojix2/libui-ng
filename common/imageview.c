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
	if (!uiprivImagePositiveFinite(viewWidth) ||
		!uiprivImagePositiveFinite(viewHeight) ||
		!uiprivImagePositiveFinite(imageWidth) ||
		!uiprivImagePositiveFinite(imageHeight))
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
	if (!uiprivImagePositiveFinite(*width) ||
		!uiprivImagePositiveFinite(*height)) {
		*width = 0;
		*height = 0;
		return;
	}
	*x = (viewWidth - *width) / 2;
	*y = (viewHeight - *height) / 2;
}
