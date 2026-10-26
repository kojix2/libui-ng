#include "../ui.h"
#include "uipriv.h"

static int canFocus(uiControl *c)
{
	uiControl *ancestor;

	if (c == NULL)
		uiprivUserBug("focus functions cannot be called with NULL");
	if (uiprivControlDestroyPending(c))
		return 0;

	// Only input controls may delegate to their internal native widgets.
	// In particular, never descend into separately owned child uiControls.
	switch (c->TypeSignature) {
	case uiAreaSignature:
	case uiButtonSignature:
	case uiCheckboxSignature:
	case uiColorButtonSignature:
	case uiComboboxSignature:
	case uiDateTimePickerSignature:
	case uiEditableComboboxSignature:
	case uiEntrySignature:
	case uiFontButtonSignature:
	case uiMultilineEntrySignature:
	case uiRadioButtonsSignature:
	case uiSliderSignature:
	case uiSpinboxSignature:
	case uiTableSignature:
		break;
	default:
		return 0;
	}

	for (ancestor = c; ancestor != NULL; ancestor = uiControlParent(ancestor)) {
		if (!uiControlVisible(ancestor) || !uiControlEnabled(ancestor))
			return 0;
		if (uiControlToplevel(ancestor))
			return ancestor->TypeSignature == uiWindowSignature;
	}
	return 0;
}

void uiControlSetFocus(uiControl *c)
{
	if (!canFocus(c) || uiprivControlFocused(c))
		return;
	// Moving focus can synchronously notify user code (e.g. commit a table
	// edit). Keep native widgets alive until the focus operation has returned.
	if (!uiprivUserCallbackEnter(c))
		return;
	uiprivControlSetFocus(c);
	uiprivUserCallbackLeave();
}

int uiControlFocused(uiControl *c)
{
	if (!canFocus(c))
		return 0;
	return uiprivControlFocused(c);
}
