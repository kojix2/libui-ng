#include <float.h>
#include <math.h>
#include "../ui.h"
#include "uipriv.h"

int uiprivImageViewContentModeValid(uiImageViewContentMode mode)
{
	return mode == uiImageViewContentCenter ||
		mode == uiImageViewContentFit;
}

static int compareRatios(double leftNumerator, double leftDenominator,
	double rightNumerator, double rightDenominator)
{
	double leftNumeratorMantissa, leftDenominatorMantissa;
	double rightNumeratorMantissa, rightDenominatorMantissa;
	double leftMantissa, rightMantissa;
	int leftNumeratorExponent, leftDenominatorExponent;
	int rightNumeratorExponent, rightDenominatorExponent;
	int leftExponent, rightExponent;

	leftNumeratorMantissa = frexp(leftNumerator, &leftNumeratorExponent);
	leftDenominatorMantissa = frexp(leftDenominator, &leftDenominatorExponent);
	rightNumeratorMantissa = frexp(rightNumerator, &rightNumeratorExponent);
	rightDenominatorMantissa = frexp(rightDenominator, &rightDenominatorExponent);
	leftExponent = leftNumeratorExponent - leftDenominatorExponent;
	rightExponent = rightNumeratorExponent - rightDenominatorExponent;
	if (leftExponent < rightExponent)
		return -1;
	if (leftExponent > rightExponent)
		return 1;
	leftMantissa = leftNumeratorMantissa * rightDenominatorMantissa;
	rightMantissa = rightNumeratorMantissa * leftDenominatorMantissa;
	if (leftMantissa < rightMantissa)
		return -1;
	if (leftMantissa > rightMantissa)
		return 1;
	return 0;
}

static double multiplyDivide(double value, double numerator,
	double denominator, double maximum)
{
	double valueMantissa, numeratorMantissa, denominatorMantissa;
	double result;
	int valueExponent, numeratorExponent, denominatorExponent;

	valueMantissa = frexp(value, &valueExponent);
	numeratorMantissa = frexp(numerator, &numeratorExponent);
	denominatorMantissa = frexp(denominator, &denominatorExponent);
	result = ldexp(valueMantissa * numeratorMantissa /
		denominatorMantissa,
		valueExponent + numeratorExponent - denominatorExponent);
	if (!(result > 0))
		result = maximum < DBL_MIN ? maximum : DBL_MIN;
	if (result > maximum)
		result = maximum;
	return result;
}

void uiprivImageViewComputeRect(double viewWidth, double viewHeight,
	double imageWidth, double imageHeight, uiImageViewContentMode mode,
	double *x, double *y, double *width, double *height)
{
	*x = 0;
	*y = 0;
	*width = 0;
	*height = 0;
	if (!uiprivImagePositiveFinite(viewWidth) ||
		!uiprivImagePositiveFinite(viewHeight) ||
		!uiprivImagePositiveFinite(imageWidth) ||
		!uiprivImagePositiveFinite(imageHeight) ||
		!uiprivImageViewContentModeValid(mode))
		return;

	if (mode == uiImageViewContentCenter) {
		*width = imageWidth;
		*height = imageHeight;
	} else if (compareRatios(imageWidth, imageHeight,
		viewWidth, viewHeight) >= 0) {
		*width = viewWidth;
		*height = multiplyDivide(viewWidth, imageHeight,
			imageWidth, viewHeight);
	} else {
		*height = viewHeight;
		*width = multiplyDivide(viewHeight, imageWidth,
			imageHeight, viewWidth);
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
