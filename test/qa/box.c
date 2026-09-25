#include "qa.h"

static void boxToggleControl(uiCheckbox *c, void *data)
{
	uiControl *control = data;

	if (uiCheckboxChecked(c))
		uiControlShow(control);
	else
		uiControlHide(control);
}

static void boxTogglePadding(uiCheckbox *c, void *data)
{
	uiBoxSetPadded((uiBox *) data, uiCheckboxChecked(c));
}

const char *boxNonStretchyNestedGuide(void)
{
	return
	"1.\tResize the window vertically. All controls in the left pane should keep\n"
	"\ttheir natural height and remain packed at the top, with blank space below.\n"
	"\tIn particular, the three labels in the nested row must not stretch taller.\n"
	"\n"
	"2.\tThe buttons `Stretchy A` and `Stretchy B` should have equal widths and\n"
	"\ttogether fill the available width. Resize the window horizontally and\n"
	"\tconfirm that they continue to share the extra width equally.\n"
	"\n"
	"3.\tToggle `Show nested row` off and on. The row should disappear and return\n"
	"\twithout leaving a gap. Toggle `Pad nested row`; only the spacing between\n"
	"\tthe three labels should change.\n"
	"\n"
	"4.\tTurn off both `Show stretchy A` and `Show stretchy B`, then turn them on\n"
	"\tagain. The row should collapse cleanly when both are hidden, and the two\n"
	"\tbuttons should return with equal widths and normal padding.\n"
	;
}

uiControl *boxNonStretchyNested(void)
{
	uiBox *box;
	uiBox *nestedRow;
	uiBox *stretchyRow;
	uiBox *options;
	uiButton *stretchyA;
	uiButton *stretchyB;
	uiCheckbox *showNested;
	uiCheckbox *padNested;
	uiCheckbox *showStretchyA;
	uiCheckbox *showStretchyB;

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	uiBoxAppend(box, uiControl(uiNewLabel("Above the nested row")), 0);

	nestedRow = uiNewHorizontalBox();
	uiBoxSetPadded(nestedRow, 1);
	uiBoxAppend(nestedRow, uiControl(uiNewLabel("Nested")), 0);
	uiBoxAppend(nestedRow, uiControl(uiNewLabel("non-stretchy")), 0);
	uiBoxAppend(nestedRow, uiControl(uiNewLabel("row")), 0);
	uiBoxAppend(box, uiControl(nestedRow), 0);

	uiBoxAppend(box, uiControl(uiNewLabel("Below the nested row")), 0);

	stretchyRow = uiNewHorizontalBox();
	uiBoxSetPadded(stretchyRow, 1);
	stretchyA = uiNewButton("Stretchy A");
	stretchyB = uiNewButton("Stretchy B");
	uiBoxAppend(stretchyRow, uiControl(stretchyA), 1);
	uiBoxAppend(stretchyRow, uiControl(stretchyB), 1);
	uiBoxAppend(box, uiControl(stretchyRow), 0);

	options = uiNewVerticalBox();
	showNested = uiNewCheckbox("Show nested row");
	uiCheckboxSetChecked(showNested, 1);
	uiCheckboxOnToggled(showNested, boxToggleControl, nestedRow);
	uiBoxAppend(options, uiControl(showNested), 0);

	padNested = uiNewCheckbox("Pad nested row");
	uiCheckboxSetChecked(padNested, 1);
	uiCheckboxOnToggled(padNested, boxTogglePadding, nestedRow);
	uiBoxAppend(options, uiControl(padNested), 0);

	showStretchyA = uiNewCheckbox("Show stretchy A");
	uiCheckboxSetChecked(showStretchyA, 1);
	uiCheckboxOnToggled(showStretchyA, boxToggleControl, stretchyA);
	uiBoxAppend(options, uiControl(showStretchyA), 0);

	showStretchyB = uiNewCheckbox("Show stretchy B");
	uiCheckboxSetChecked(showStretchyB, 1);
	uiCheckboxOnToggled(showStretchyB, boxToggleControl, stretchyB);
	uiBoxAppend(options, uiControl(showStretchyB), 0);
	uiBoxAppend(box, uiControl(options), 0);

	return uiControl(box);
}
