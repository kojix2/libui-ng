#include "qa.h"

const char *tooltipTestGuide() {
	return
	"1.\tMove the mouse cursor to a text entry and see `Tooltip for uiEntry` pop up.\n"
	"\n"
	"2.\tMove the mouse cursor to a checkbox and see `Tooltip for uiCheckbox` pop up.\n"
	"\n"
	"3.\tMove the mouse cursor to each radio buttion and see `Tooltip for uiRadioButtons` pop up.\n"
	"\n"
	"4.\tMove the mouse cursor to a spinbox and see `Tooltip for uiSpinbox` pop up.\n"
	"\n"
	"5.\tMove the mouse cursor to a combobox and see `Tooltip for uiCombobox` pop up.\n"
	"\n"
	"6.\tMove the mouse cursor to an editable combobox and see `Tooltip for uiEditableCombobox` pop up.\n"
	;
}

uiControl* tooltipTest()
{
	uiBox *vbox;

	vbox = uiNewVerticalBox();
	uiBoxSetPadded(vbox, 1);

	uiEntry *entry = uiNewEntry();
	uiControlSetTooltip(uiControl(entry), "Tooltip for uiEntry");
	uiBoxAppend(vbox, uiControl(entry), 0);

	uiCheckbox *check = uiNewCheckbox("uiCheckbox");
	uiControlSetTooltip(uiControl(check), "Tooltip for uiCheckbox");
	uiBoxAppend(vbox, uiControl(check), 0);

	uiRadioButtons *radio = uiNewRadioButtons();
	uiRadioButtonsAppend(radio, "Item 1");
	uiRadioButtonsAppend(radio, "Item 2");
	uiControlSetTooltip(uiControl(radio), "Tooltip for uiRadioButtons");
	uiBoxAppend(vbox, uiControl(radio), 0);

	uiSpinbox *spin = uiNewSpinbox(0, 100);
	uiControlSetTooltip(uiControl(spin), "Tooltip for uiSpinbox");
	uiBoxAppend(vbox, uiControl(spin), 0);

	uiCombobox *combo = uiNewCombobox();
	uiComboboxAppend(combo, "Item 1");
	uiComboboxAppend(combo, "Item 2");
	uiControlSetTooltip(uiControl(combo), "Tooltip for uiCombobox");
	uiBoxAppend(vbox, uiControl(combo), 0);

	uiEditableCombobox *combo2 = uiNewEditableCombobox();
	uiEditableComboboxAppend(combo2, "Item 1");
	uiEditableComboboxAppend(combo2, "Item 2");
	uiControlSetTooltip(uiControl(combo2), "Tooltip for uiEditableCombobox");
	uiBoxAppend(vbox, uiControl(combo2), 0);

	return uiControl(vbox);
}
