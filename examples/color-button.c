#include <stdio.h>
#include <ui.h>

static void updateLabel(uiColorButton *button, uiLabel *label)
{
	double r;
	double g;
	double b;
	double a;
	char text[96];

	uiColorButtonColor(button, &r, &g, &b, &a);
	snprintf(text, sizeof text, "RGBA: %.2f %.2f %.2f %.2f", r, g, b, a);
	uiLabelSetText(label, text);
}

static void onColorChanged(uiColorButton *button, void *data)
{
	updateLabel(button, uiLabel(data));
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
	uiColorButton *button;
	uiLabel *label;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("Color Button", 360, 160, 0);
	uiWindowSetMargined(w, 1);
	uiWindowOnClosing(w, onClosing, NULL);

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	uiWindowSetChild(w, uiControl(box));

	button = uiNewColorButton();
	uiColorButtonSetColor(button, 0.10, 0.45, 0.80, 1.00);
	uiBoxAppend(box, uiControl(button), 0);

	label = uiNewLabel("");
	updateLabel(button, label);
	uiColorButtonOnChanged(button, onColorChanged, label);
	uiBoxAppend(box, uiControl(label), 0);

	uiControlShow(uiControl(w));
	uiMain();
	uiUninit();
	return 0;
}
