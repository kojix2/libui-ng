#include <string.h>

#include "qa.h"

const char *labelMultiLineGuide(void) {
	return
	"1.\tThe first line should read `Single line test`.\n"
	"\n"
	"2.\tNext are four invisible horizontal boxes.\n"
	"\tThe first three should each have the width of the line `Long line`.\n"
	"\tThe fourth box should take up the remaining space and consist of three\n"
	"\tlines reading `Padding`.\n"
	"\n"
	"3.\tThe next line should read `Multi line height test` and should have a\n"
	"\tsimilar distance to the four boxes as the first line `Single line test`."
	;
}

uiControl *labelMultiLine(void)
{
	uiBox *vbox;
	uiBox *hbox;
	uiLabel *label;

	vbox = uiNewVerticalBox();
	uiBoxSetPadded(vbox, 1);

	label = uiNewLabel("Single line test");
	uiBoxAppend(vbox, uiControl(label), 0);

	hbox = uiNewHorizontalBox();
	uiBoxSetPadded(hbox, 1);
	uiBoxAppend(vbox, uiControl(hbox), 0);

	label = uiNewLabel("Long line\nShort\nShort");
	uiBoxAppend(hbox, uiControl(label), 0);

	label = uiNewLabel("Short\nLong line\nShort");
	uiBoxAppend(hbox, uiControl(label), 0);

	label = uiNewLabel("Short\nShort\nLong line");
	uiBoxAppend(hbox, uiControl(label), 0);

	label = uiNewLabel("Padding\nPadding\nPadding");
	uiBoxAppend(hbox, uiControl(label), 1);

	label = uiNewLabel("Multi line height test");
	uiBoxAppend(vbox, uiControl(label), 0);

	return uiControl(vbox);
}

const char *labelVerticalAlignmentGuide(void) {
	return
	"1.\tIn the first row, both labels should be vertically centered beside\n"
	"\tthe spinbox and combobox.\n"
	"\n"
	"2.\tIn the second row, `Centered label` should be vertically centered\n"
	"\tbeside the tall multiline entry.\n"
	"\n"
	"3.\tIn the third row, the complete `Multiline\\nlabel` text block should\n"
	"\tbe vertically centered beside the tall multiline entry.\n"
	"\n"
	"4.\tIn the fourth row, click `Toggle lines`. The label should switch\n"
	"\tbetween centered single-line and centered multiline text.\n"
	"\n"
	"5.\tResize the window vertically. Every label text block should remain\n"
	"\tvertically centered in its row."
	;
}

static uiMultilineEntry *newAlignmentTestEntry(void)
{
	uiMultilineEntry *entry;

	entry = uiNewMultilineEntry();
	uiMultilineEntrySetText(entry, "Tall control\nwith multiple lines");
	uiMultilineEntrySetReadOnly(entry, 1);
	return entry;
}

static void toggleLabelLines(uiButton *button, void *data)
{
	uiLabel *label = data;
	char *text;

	text = uiLabelText(label);
	if (strchr(text, '\n') == NULL)
		uiLabelSetText(label, "Two centered\nlines");
	else
		uiLabelSetText(label, "One centered line");
	uiFreeText(text);
}

uiControl *labelVerticalAlignment(void)
{
	uiBox *vbox;
	uiBox *row;
	uiButton *button;
	uiCombobox *combobox;
	uiLabel *label;

	vbox = uiNewVerticalBox();
	uiBoxSetPadded(vbox, 1);

	row = uiNewHorizontalBox();
	uiBoxSetPadded(row, 1);
	uiBoxAppend(row, uiControl(uiNewLabel("Spinbox")), 0);
	uiBoxAppend(row, uiControl(uiNewSpinbox(0, 100)), 0);
	uiBoxAppend(row, uiControl(uiNewLabel("Combobox")), 0);
	combobox = uiNewCombobox();
	uiComboboxAppend(combobox, "Item");
	uiComboboxSetSelected(combobox, 0);
	uiBoxAppend(row, uiControl(combobox), 1);
	uiBoxAppend(vbox, uiControl(row), 0);

	row = uiNewHorizontalBox();
	uiBoxSetPadded(row, 1);
	uiBoxAppend(row, uiControl(uiNewLabel("Centered label")), 0);
	uiBoxAppend(row, uiControl(newAlignmentTestEntry()), 1);
	uiBoxAppend(vbox, uiControl(row), 1);

	row = uiNewHorizontalBox();
	uiBoxSetPadded(row, 1);
	uiBoxAppend(row, uiControl(uiNewLabel("Multiline\nlabel")), 0);
	uiBoxAppend(row, uiControl(newAlignmentTestEntry()), 1);
	uiBoxAppend(vbox, uiControl(row), 1);

	row = uiNewHorizontalBox();
	uiBoxSetPadded(row, 1);
	label = uiNewLabel("One centered line");
	uiBoxAppend(row, uiControl(label), 0);
	uiBoxAppend(row, uiControl(newAlignmentTestEntry()), 1);
	button = uiNewButton("Toggle lines");
	uiButtonOnClicked(button, toggleLabelLines, label);
	uiBoxAppend(row, uiControl(button), 0);
	uiBoxAppend(vbox, uiControl(row), 1);

	return uiControl(vbox);
}
