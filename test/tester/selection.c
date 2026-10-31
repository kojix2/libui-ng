#include "test.h"

static uiLabel *cboxLbl;
static uiCombobox *cbox;
static uiEditableCombobox *editable;
static uiRadioButtons *rb;

static void updateCB()
{
	char str[128];

	snprintf(str, sizeof str, "Combobox items: %d", uiComboboxNumItems(cbox));
	uiLabelSetText(cboxLbl, str);
}

static void appendCBRB(uiButton *b, void *data)
{
	uiComboboxAppend(cbox, "New Item");
	uiEditableComboboxAppend(editable, "New Item");
	uiRadioButtonsAppend(rb, "New Item");
	updateCB();
}

static void insertCB(uiButton *b, void *data)
{
	uiComboboxInsertAt(cbox, 0, "Inserted item");
	updateCB();
}

static void deleteCB(uiButton *b, void *data)
{
	uiComboboxDelete(cbox, 0);
	updateCB();
}

static void clearCB(uiButton *b, void *data)
{
	uiComboboxClear(cbox);
	updateCB();
}

static void onCBChanged(uiCombobox *c, void *data)
{
	printf("%s combobox changed to %d\n",
		(char *) data,
		(int) uiComboboxSelected(c));
	uiEditableComboboxSetText(editable, "changed");
}

static void onECBChanged(uiEditableCombobox *c, void *data)
{
	char *t;

	t = uiEditableComboboxText(c);
	printf("%s combobox changed to %s\n",
		(char *) data,
		t);
	uiFreeText(t);
}

static void onRBSelected(uiRadioButtons *r, void *data)
{
	printf("radio buttons %d\n", uiRadioButtonsSelected(r));
}

static void selectSecond(uiButton *b, void *data)
{
	// TODO combobox, editable
	uiRadioButtonsSetSelected(rb, 1);
}

static void selectNone(uiButton *b, void *data)
{
	// TODO combobox, editable
	uiRadioButtonsSetSelected(rb, -1);
}

uiBox *makeSelection(void)
{
	uiBox *page, *hbox;
	uiButton *b;
	uiRadioButtons *independent;

	page = newVerticalBox();

	hbox = newHorizontalBox();
	cboxLbl = uiNewLabel("0");
	uiBoxAppend(hbox, uiControl(cboxLbl), 0);
	cbox = uiNewCombobox();
	uiComboboxAppend(cbox, "Item 1");
	uiComboboxAppend(cbox, "Item 2");
	uiComboboxAppend(cbox, "Item 3");
	uiComboboxOnSelected(cbox, onCBChanged, "noneditable");
	updateCB();
	uiBoxAppend(hbox, uiControl(cbox), 1);
	uiBoxAppend(page, uiControl(hbox), 0);

	editable = uiNewEditableCombobox();
	uiEditableComboboxAppend(editable, "Editable Item 1");
	uiEditableComboboxAppend(editable, "Editable Item 2");
	uiEditableComboboxAppend(editable, "Editable Item 3");
	uiEditableComboboxOnChanged(editable, onECBChanged, "editable");
	uiBoxAppend(page, uiControl(editable), 0);

	uiBoxAppend(page, uiControl(uiNewLabel("Radio group controlled by the buttons below")), 0);
	rb = uiNewRadioButtons();
	uiRadioButtonsAppend(rb, "Item 1");
	uiRadioButtonsAppend(rb, "Item 2");
	uiRadioButtonsAppend(rb, "Item 3");
	uiRadioButtonsOnSelected(rb, onRBSelected, NULL);
	uiBoxAppend(page, uiControl(rb), 0);

	hbox = newHorizontalBox();
	b = uiNewButton("Append");
	uiButtonOnClicked(b, appendCBRB, NULL);
	uiBoxAppend(hbox, uiControl(b), 0);
	b = uiNewButton("Insert");
	uiButtonOnClicked(b, insertCB, NULL);
	uiBoxAppend(hbox, uiControl(b), 0);
	b = uiNewButton("Delete");
	uiButtonOnClicked(b, deleteCB, NULL);
	uiBoxAppend(hbox, uiControl(b), 0);
	b = uiNewButton("Clear");
	uiButtonOnClicked(b, clearCB, NULL);
	uiBoxAppend(hbox, uiControl(b), 0);
	b = uiNewButton("Second");
	uiButtonOnClicked(b, selectSecond, NULL);
	uiBoxAppend(hbox, uiControl(b), 0);
	b = uiNewButton("None");
	uiButtonOnClicked(b, selectNone, NULL);
	uiBoxAppend(hbox, uiControl(b), 0);
	uiBoxAppend(page, uiControl(hbox), 0);

	uiBoxAppend(page, uiControl(uiNewHorizontalSeparator()), 0);

	uiBoxAppend(page, uiControl(uiNewLabel("Independent group 1")), 0);
	independent = uiNewRadioButtons();
	uiRadioButtonsAppend(independent, "Item 1");
	uiRadioButtonsAppend(independent, "Item 2");
	uiRadioButtonsAppend(independent, "Item 3");
	uiBoxAppend(page, uiControl(independent), 0);

	uiBoxAppend(page, uiControl(uiNewLabel("Independent group 2")), 0);
	independent = uiNewRadioButtons();
	uiRadioButtonsAppend(independent, "Item A");
	uiRadioButtonsAppend(independent, "Item B");
	uiBoxAppend(page, uiControl(independent), 0);

	return page;
}
