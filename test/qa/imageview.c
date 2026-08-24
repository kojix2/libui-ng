#include <stdio.h>

#include "qa.h"

static uiImageView *minimumImageView;
static uiLabel *minimumImageViewStatus;
static int minimumImageViewStage;

static uiImage *newCheckerImage(void)
{
	unsigned char pixels[32 * 16 * 4];
	uiImage *image;
	int x, y;

	for (y = 0; y < 16; y++)
		for (x = 0; x < 32; x++) {
			int offset = (y * 32 + x) * 4;
			int bright = ((x / 4) + (y / 4)) % 2;

			pixels[offset + 0] = bright ? 0x20 : 0xE0;
			pixels[offset + 1] = bright ? 0x80 : 0x40;
			pixels[offset + 2] = bright ? 0xE0 : 0x20;
			pixels[offset + 3] = 0xFF;
		}
	image = uiNewImage(32, 16);
	uiImageAppend(image, pixels, 32, 16, 32 * 4);
	return image;
}

static uiImageView *newPopulatedImageView(uiImageViewContentMode mode)
{
	uiImageView *view;
	uiImage *image;

	view = uiNewImageView();
	uiImageViewSetContentMode(view, mode);
	image = newCheckerImage();
	uiImageViewSetImage(view, image);
	uiFreeImage(image);
	return view;
}

static void imageViewCycleMinimum(uiButton *button, void *data)
{
	static const int values[] = { -1, 0, 48, 160 };
	char text[32];
	int value;

	minimumImageViewStage = (minimumImageViewStage + 1) % 4;
	value = values[minimumImageViewStage];
	uiControlSetMinimumSize(uiControl(minimumImageView), value, value);
	if (value == -1)
		uiLabelSetText(minimumImageViewStatus, "Minimum: default (16 x 16)");
	else {
		snprintf(text, sizeof text, "Minimum: %d x %d", value, value);
		uiLabelSetText(minimumImageViewStatus, text);
	}
}

const char *imageViewLayoutGuide(void)
{
	return
	"1. The empty and populated default views must reserve at least 16 x 16,\n"
	"   but the 32 x 16 image must not set the window's preferred size.\n\n"
	"2. Fit should preserve aspect ratio inside 120 x 80. Center should draw at\n"
	"   1:1 in the same allocation. Resize the window in both directions.\n\n"
	"3. Cycle the first view through default, 0, 48, and 160. Zero may shrink\n"
	"   below 16; default must restore 16 x 16. No constraints may conflict.\n";
}

uiControl *imageViewLayout(void)
{
	uiBox *box;
	uiBox *row;
	uiButton *cycle;
	uiImageView *center;
	uiImageView *empty;
	uiImageView *fit;

	minimumImageViewStage = 0;
	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	row = uiNewHorizontalBox();
	uiBoxSetPadded(row, 1);
	empty = uiNewImageView();
	uiBoxAppend(row, uiControl(empty), 0);
	uiBoxAppend(row, uiControl(uiNewLabel("Empty default")), 0);
	minimumImageView = newPopulatedImageView(uiImageViewContentFit);
	uiBoxAppend(row, uiControl(minimumImageView), 0);
	uiBoxAppend(row, uiControl(uiNewLabel("Populated default")), 0);
	uiBoxAppend(row, uiControl(uiNewLabel("")), 1);
	uiBoxAppend(box, uiControl(row), 0);

	row = uiNewHorizontalBox();
	uiBoxSetPadded(row, 1);
	fit = newPopulatedImageView(uiImageViewContentFit);
	uiControlSetMinimumSize(uiControl(fit), 120, 80);
	uiBoxAppend(row, uiControl(fit), 1);
	center = newPopulatedImageView(uiImageViewContentCenter);
	uiControlSetMinimumSize(uiControl(center), 120, 80);
	uiBoxAppend(row, uiControl(center), 1);
	uiBoxAppend(box, uiControl(row), 1);

	cycle = uiNewButton("Cycle first populated view minimum");
	uiButtonOnClicked(cycle, imageViewCycleMinimum, NULL);
	uiBoxAppend(box, uiControl(cycle), 0);
	minimumImageViewStatus = uiNewLabel("Minimum: default (16 x 16)");
	uiBoxAppend(box, uiControl(minimumImageViewStatus), 0);
	return uiControl(box);
}
