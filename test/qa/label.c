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

const char *labelCompactGroupGuide(void) {
	return
	"1.\tThe group on the left should be as tall as the text on the right.\n"
	"\n"
	"2.\tThe label row and separator should follow the checkbox closely.\n"
	"\tAny spare height in the group should be below the entry, not between\n"
	"\tthe controls.\n"
	"\n"
	"3.\tResize the window horizontally. The group should take the available\n"
	"\twidth while the label on its right keeps its natural width."
	;
}

uiControl *labelCompactGroup(void)
{
	uiBox *columns;
	uiBox *inner;
	uiBox *row;
	uiGroup *group;

	columns = uiNewHorizontalBox();
	uiBoxSetPadded(columns, 1);
	group = uiNewGroup("Controls");
	uiGroupSetMargined(group, 1);
	uiBoxAppend(columns, uiControl(group), 1);

	inner = uiNewVerticalBox();
	uiBoxSetPadded(inner, 1);
	uiGroupSetChild(group, uiControl(inner));
	uiBoxAppend(inner, uiControl(uiNewButton("Button")), 0);
	uiBoxAppend(inner, uiControl(uiNewCheckbox("Checkbox")), 0);

	row = uiNewHorizontalBox();
	uiBoxSetPadded(row, 1);
	uiBoxAppend(row, uiControl(uiNewLabel("First label")), 0);
	uiBoxAppend(row, uiControl(uiNewLabel("Second label")), 0);
	uiBoxAppend(inner, uiControl(row), 0);
	uiBoxAppend(inner, uiControl(uiNewHorizontalSeparator()), 0);
	uiBoxAppend(inner, uiControl(uiNewEntry()), 0);

	uiBoxAppend(columns, uiControl(uiNewLabel(
		"Tall neighbor\nOne\nTwo\nThree\nFour\nFive\nSix\nSeven\nEight\nNine\nTen\n"
		"Eleven\nTwelve\nThirteen\nFourteen\nFifteen\nSixteen\nSeventeen\nEighteen\nNineteen\nTwenty")), 0);
	return uiControl(columns);
}

const char *labelFontSizeGuide(void) {
	return
	"1.\tThe first four labels should appear at 8, default, 18, and 32 point\n"
	"\tsizes. None of the text should be clipped.\n"
	"\n"
	"2.\tUse the buttons to change the final multiline label between 10 and\n"
	"\t24 points, then restore the default size. Its complete text block\n"
	"\tshould remain vertically centered beside the entry.\n"
	"\n"
	"3.\tResize the window. All labels should retain their selected sizes and\n"
	"\tthe layout should remain stable."
	;
}

static void setLabelSize10(uiButton *button, void *data)
{
	uiLabelSetFontSize(uiLabel(data), 10);
}

static void setLabelSize24(uiButton *button, void *data)
{
	uiLabelSetFontSize(uiLabel(data), 24);
}

static void resetLabelSize(uiButton *button, void *data)
{
	uiLabelResetFontSize(uiLabel(data));
}

uiControl *labelFontSize(void)
{
	uiBox *vbox;
	uiBox *row;
	uiButton *button;
	uiLabel *label;

	vbox = uiNewVerticalBox();
	uiBoxSetPadded(vbox, 1);

	label = uiNewLabel("8 point label");
	uiLabelSetFontSize(label, 8);
	uiBoxAppend(vbox, uiControl(label), 0);

	uiBoxAppend(vbox, uiControl(uiNewLabel("Default size label")), 0);

	label = uiNewLabel("18 point label");
	uiLabelSetFontSize(label, 18);
	uiBoxAppend(vbox, uiControl(label), 0);

	label = uiNewLabel("32 point label");
	uiLabelSetFontSize(label, 32);
	uiBoxAppend(vbox, uiControl(label), 0);

	row = uiNewHorizontalBox();
	uiBoxSetPadded(row, 1);
	label = uiNewLabel("Dynamic multiline\nlabel");
	uiBoxAppend(row, uiControl(label), 0);
	uiBoxAppend(row, uiControl(newAlignmentTestEntry()), 1);
	uiBoxAppend(vbox, uiControl(row), 1);

	row = uiNewHorizontalBox();
	uiBoxSetPadded(row, 1);
	button = uiNewButton("10 pt");
	uiButtonOnClicked(button, setLabelSize10, label);
	uiBoxAppend(row, uiControl(button), 0);
	button = uiNewButton("24 pt");
	uiButtonOnClicked(button, setLabelSize24, label);
	uiBoxAppend(row, uiControl(button), 0);
	button = uiNewButton("Default");
	uiButtonOnClicked(button, resetLabelSize, label);
	uiBoxAppend(row, uiControl(button), 0);
	uiBoxAppend(vbox, uiControl(row), 0);

	return uiControl(vbox);
}
