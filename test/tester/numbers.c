// 19 may 2015
#include "test.h"

static uiSpinbox *spinbox;
static uiSlider *slider;
static uiProgressBar *pbar;
static uiProgressBar *pbar2;
static uiCheckbox *checkbox;
static uiSpinbox *spinboxFrom;
static uiSpinbox *spinboxTo;


#define CHANGED(what) \
	static void on ## what ## Changed(ui ## what *this, void *data) \
	{ \
		int value; \
		printf("on %s changed\n", #what); \
		value = ui ## what ## Value(this); \
		uiSpinboxSetValue(spinbox, value); \
		uiSliderSetValue(slider, value); \
		uiProgressBarSetValue(pbar, value); \
	}
CHANGED(Spinbox)
CHANGED(Slider)

#define SETTOO(what, name, n) \
	static void set ## what ## Too ## name(uiButton *this, void *data) \
	{ \
		ui ## what ## SetValue(ui ## what(data), n); \
	}
SETTOO(Spinbox, Low, -80)
SETTOO(Spinbox, High, 80)
SETTOO(Slider, Low, -80)
SETTOO(Slider, High, 80)

static void sliderEnableToolTip(uiButton *b, void *data)
{
	uiSliderSetHasToolTip(uiSlider(data), 1);
	uiCheckboxSetChecked(checkbox, uiSliderHasToolTip(uiSlider(data)));
}

static void sliderDisableToolTip(uiButton *b, void *data)
{
	uiSliderSetHasToolTip(uiSlider(data), 0);
	uiCheckboxSetChecked(checkbox, uiSliderHasToolTip(uiSlider(data)));
}

static void setSliderRange(uiSpinbox *spinbox, void *data)
{
	uiSlider *s = data;

	uiSliderSetRange(s, uiSpinboxValue(spinboxFrom), uiSpinboxValue(spinboxTo));
}

static void onRangeSliderChanged(uiSlider *s, void *data)
{
	char str[128];
	uiLabel *lbl = data;

	snprintf(str, sizeof str, "%d", uiSliderValue(s));
	uiLabelSetText(lbl, str);
}

static void onSliderReleased(uiSlider *s, void *data){
	int value;

	printf("on Slider released\n");
	value = uiSliderValue(s);
	uiProgressBarSetValue(pbar2, value);
}

static void setIndeterminate(uiButton *b, void *data)
{
	uiProgressBar *p = uiProgressBar(data);
	int value;

	value = uiProgressBarValue(p);
	if (value == -1)
		value = 50;
	else
		value = -1;
	uiProgressBarSetValue(p, value);
}

uiBox *makeNumbers(void)
{
	uiBox *page, *hbox;
	uiSpinbox *xsb;
	uiSlider *xsl, *toolTipSlider;
	uiButton *b;
	uiLabel *lbl;
	uiProgressBar *p;

	page = newVerticalBox();
	uiBoxAppend(page, uiControl(uiNewLabel("Synchronized spinbox, slider and progress (0-100)")), 0);

	spinbox = uiNewSpinbox(0, 100);
	uiSpinboxOnChanged(spinbox, onSpinboxChanged, NULL);
	uiBoxAppend(page, uiControl(spinbox), 0);

	slider = uiNewSlider(0, 100);
	uiSliderOnChanged(slider, onSliderChanged, NULL);
	uiSliderOnReleased(slider, onSliderReleased, NULL);
	uiBoxAppend(page, uiControl(slider), 0);

	uiBoxAppend(page, uiControl(uiNewLabel("Progress on change")), 0);
	pbar = uiNewProgressBar();
	uiBoxAppend(page, uiControl(pbar), 0);

	uiBoxAppend(page, uiControl(uiNewLabel("Progress on slider release")), 0);
	pbar2 = uiNewProgressBar();
	uiBoxAppend(page, uiControl(pbar2), 0);

	uiBoxAppend(page, uiControl(uiNewLabel("Separate slider for tooltip testing")), 0);
	hbox = newHorizontalBox();
	toolTipSlider = uiNewSlider(0, 100);
	uiBoxAppend(hbox, uiControl(toolTipSlider), 1);
	b = uiNewButton("Enable ToolTip");
	uiButtonOnClicked(b, sliderEnableToolTip, toolTipSlider);
	uiBoxAppend(hbox, uiControl(b), 0);
	b = uiNewButton("Disable ToolTip");
	uiButtonOnClicked(b, sliderDisableToolTip, toolTipSlider);
	uiBoxAppend(hbox, uiControl(b), 0);
	checkbox = uiNewCheckbox("Has ToolTip");
	uiControlDisable(uiControl(checkbox));
	uiCheckboxSetChecked(checkbox, uiSliderHasToolTip(toolTipSlider));
	uiBoxAppend(hbox, uiControl(checkbox), 0);
	uiBoxAppend(page, uiControl(hbox), 0);

	uiBoxAppend(page, uiControl(uiNewHorizontalSeparator()), 0);

	uiBoxAppend(page, uiControl(uiNewLabel("Out-of-range values (-40 to 40)")), 0);
	hbox = newHorizontalBox();
	xsb = uiNewSpinbox(-40, 40);
	uiBoxAppend(hbox, uiControl(xsb), 0);
	b = uiNewButton("Bad Low");
	uiButtonOnClicked(b, setSpinboxTooLow, xsb);
	uiBoxAppend(hbox, uiControl(b), 0);
	b = uiNewButton("Bad High");
	uiButtonOnClicked(b, setSpinboxTooHigh, xsb);
	uiBoxAppend(hbox, uiControl(b), 0);
	uiBoxAppend(page, uiControl(hbox), 0);

	hbox = newHorizontalBox();
	xsl = uiNewSlider(-40, 40);
	uiBoxAppend(hbox, uiControl(xsl), 1);
	b = uiNewButton("Bad Low");
	uiButtonOnClicked(b, setSliderTooLow, xsl);
	uiBoxAppend(hbox, uiControl(b), 0);
	b = uiNewButton("Bad High");
	uiButtonOnClicked(b, setSliderTooHigh, xsl);
	uiBoxAppend(hbox, uiControl(b), 0);
	uiBoxAppend(page, uiControl(hbox), 0);

	uiBoxAppend(page, uiControl(uiNewHorizontalSeparator()), 0);

	uiBoxAppend(page, uiControl(uiNewLabel("Dynamic range: minimum / slider / maximum")), 0);
	lbl = uiNewLabel("100");
	uiBoxAppend(page, uiControl(lbl), 0);
	hbox = newHorizontalBox();
	spinboxFrom = uiNewSpinbox(0, 1000);
	uiSpinboxSetValue(spinboxFrom, 100);
	uiBoxAppend(hbox, uiControl(spinboxFrom), 1);
	xsl = uiNewSlider(100, 200);
	uiSpinboxOnChanged(spinboxFrom, setSliderRange, xsl);
	uiBoxAppend(hbox, uiControl(xsl), 1);
	uiSliderOnChanged(xsl, onRangeSliderChanged, lbl);
	spinboxTo = uiNewSpinbox(0, 1000);
	uiSpinboxSetValue(spinboxTo, 200);
	uiSpinboxOnChanged(spinboxTo, setSliderRange, xsl);
	uiBoxAppend(hbox, uiControl(spinboxTo), 1);
	uiBoxAppend(page, uiControl(hbox), 0);

	uiBoxAppend(page, uiControl(uiNewHorizontalSeparator()), 0);

	uiBoxAppend(page, uiControl(uiNewLabel("Indeterminate progress")), 0);
	p = uiNewProgressBar();
	uiProgressBarSetValue(p, 50);
	uiBoxAppend(page, uiControl(p), 0);
	b = uiNewButton("Toggle Indeterminate");
	uiButtonOnClicked(b, setIndeterminate, p);
	uiBoxAppend(page, uiControl(b), 0);

	return page;
}
