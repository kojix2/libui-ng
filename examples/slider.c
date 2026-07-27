#include <stdio.h>
#include <ui.h>

static void onSliderChanged(uiSlider *slider, void *data)
{
	uiLabel *label = uiLabel(data);
	char text[64];

	snprintf(text, sizeof text, "Value: %d", uiSliderValue(slider));
	uiLabelSetText(label, text);
}

static int onClosing(uiWindow *w, void *data)
{
	uiQuit();
	return 1;
}

int main(void)
{
	uiInitOptions o = {0};
	const char *err;
	uiWindow *w;
	uiBox *box;
	uiSlider *slider;
	uiLabel *label;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("Slider", 320, 160, 0);
	uiWindowSetMargined(w, 1);
	uiWindowOnClosing(w, onClosing, NULL);

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	uiWindowSetChild(w, uiControl(box));

	slider = uiNewSlider(0, 100);
	uiSliderSetValue(slider, 25);
	uiBoxAppend(box, uiControl(slider), 0);

	label = uiNewLabel("Value: 25");
	uiSliderOnChanged(slider, onSliderChanged, label);
	uiBoxAppend(box, uiControl(label), 0);

	uiControlShow(uiControl(w));
	uiMain();
	uiUninit();
	return 0;
}
