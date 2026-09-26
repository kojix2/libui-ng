#include "qa.h"

static void sliderDescriptionToggled(uiCheckbox *checkbox, void *data)
{
	uiSlider *slider = uiSlider(data);

	if (uiCheckboxChecked(checkbox))
		uiControlSetTooltip(uiControl(slider), "Descriptive tooltip for uiSlider");
	else
		uiControlSetTooltip(uiControl(slider), NULL);
}

const char *tooltipTestGuide(void)
{
	return
	"1.\tMove the pointer to the text entry and see its tooltip.\n"
	"\n"
	"2.\tMove the pointer to the checkbox and see its tooltip.\n"
	"\n"
	"3.\tMove the pointer to both radio buttons and see their tooltip. "
	"The tooltip was set before the items were appended.\n"
	"\n"
	"4.\tMove the pointer across the spinbox entry and arrow buttons and see its tooltip.\n"
	"\n"
	"5.\tMove the pointer to the combobox and editable combobox and see their tooltips.\n"
	"\n"
	"6.\tWith Use descriptive slider tooltip checked, move the pointer to the "
	"slider and see Descriptive tooltip for uiSlider.\n"
	"\n"
	"7.\tUncheck it, then hover or drag the slider and see its numeric value tooltip.\n"
	"\n"
	"8.\tSwitch away from this test and back. No tooltip should remain and no error "
	"should occur while the controls are destroyed.\n";
}

uiControl *tooltipTest(void)
{
	uiBox *vbox;
	uiCheckbox *check;
	uiCombobox *combo;
	uiEditableCombobox *editable;
	uiEntry *entry;
	uiRadioButtons *radio;
	uiSlider *slider;
	uiSpinbox *spin;

	vbox = uiNewVerticalBox();
	uiBoxSetPadded(vbox, 1);

	entry = uiNewEntry();
	uiControlSetTooltip(uiControl(entry), "Tooltip for uiEntry");
	uiBoxAppend(vbox, uiControl(entry), 0);

	check = uiNewCheckbox("uiCheckbox");
	uiControlSetTooltip(uiControl(check), "Tooltip for uiCheckbox");
	uiBoxAppend(vbox, uiControl(check), 0);

	radio = uiNewRadioButtons();
	uiControlSetTooltip(uiControl(radio), "Tooltip for uiRadioButtons");
	uiRadioButtonsAppend(radio, "Item 1");
	uiRadioButtonsAppend(radio, "Item 2");
	uiBoxAppend(vbox, uiControl(radio), 0);

	spin = uiNewSpinbox(0, 100);
	uiControlSetTooltip(uiControl(spin), "Tooltip for uiSpinbox");
	uiBoxAppend(vbox, uiControl(spin), 0);

	combo = uiNewCombobox();
	uiComboboxAppend(combo, "Item 1");
	uiComboboxAppend(combo, "Item 2");
	uiControlSetTooltip(uiControl(combo), "Tooltip for uiCombobox");
	uiBoxAppend(vbox, uiControl(combo), 0);

	editable = uiNewEditableCombobox();
	uiEditableComboboxAppend(editable, "Item 1");
	uiEditableComboboxAppend(editable, "Item 2");
	uiControlSetTooltip(uiControl(editable), "Tooltip for uiEditableCombobox");
	uiBoxAppend(vbox, uiControl(editable), 0);

	slider = uiNewSlider(0, 100);
	uiSliderSetValue(slider, 50);
	uiControlSetTooltip(uiControl(slider), "Descriptive tooltip for uiSlider");
	uiBoxAppend(vbox, uiControl(slider), 0);

	check = uiNewCheckbox("Use descriptive slider tooltip");
	uiCheckboxSetChecked(check, 1);
	uiCheckboxOnToggled(check, sliderDescriptionToggled, slider);
	uiBoxAppend(vbox, uiControl(check), 0);

	return uiControl(vbox);
}
