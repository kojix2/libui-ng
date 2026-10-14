#include "../../ui.h"

int main(void)
{
	uiInitOptions options = { 0 };
	uiImageView *imageView;
	uiLabel *label;
	const char *error;

	error = uiInit(&options);
	if (error != NULL) {
		uiFreeInitError(error);
		return 1;
	}
	label = uiNewLabel("status");
	uiControlSetTooltip(uiControl(label), "label tooltip");
	uiControlDestroy(uiControl(label));
	imageView = uiNewImageView();
	uiControlSetTooltip(uiControl(imageView), "image view tooltip");
	uiControlDestroy(uiControl(imageView));
	uiUninit();
	return 0;
}
