#include <stdio.h>

#include "qa.h"

static uiSpinbox *stabilityStart;
static uiSpinbox *stabilityEnd;
static uiLabel *stabilityValues;
static int stabilityAlternate;

static void layoutSpinboxSetValues(uiButton *button, void *data)
{
	stabilityAlternate = !stabilityAlternate;
	uiSpinboxSetValue(stabilityStart, stabilityAlternate ? 1000 : 1);
	uiSpinboxSetValue(stabilityEnd, stabilityAlternate ? 1 : 1000);
	uiLabelSetText(stabilityValues,
		stabilityAlternate ? "Start 1000 / End 1" : "Start 1 / End 1000");
}

const char *layoutSpinboxStabilityGuide(void)
{
	return
	"1. Resize the window horizontally. Start and End must keep equal, stable\n"
	"   widths while the blank area at the right receives the extra space.\n\n"
	"2. Alternate focus between both text fields. Their frames must not change\n"
	"   and neither focus ring may overlap the other controls.\n\n"
	"3. Click `Swap values` repeatedly. Values 1 and 1000 must not change the\n"
	"   allocated widths. No Auto Layout warning should be printed.\n";
}

uiControl *layoutSpinboxStability(void)
{
	uiBox *box;
	uiBox *row;
	uiButton *values;

	stabilityAlternate = 0;
	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	row = uiNewHorizontalBox();
	uiBoxSetPadded(row, 1);
	uiBoxAppend(row, uiControl(uiNewLabel("Start")), 0);
	stabilityStart = uiNewSpinbox(1, 1000);
	uiSpinboxSetValue(stabilityStart, 1);
	uiBoxAppend(row, uiControl(stabilityStart), 0);
	uiBoxAppend(row, uiControl(uiNewLabel("End")), 0);
	stabilityEnd = uiNewSpinbox(1, 1000);
	uiSpinboxSetValue(stabilityEnd, 1000);
	uiBoxAppend(row, uiControl(stabilityEnd), 0);
	uiBoxAppend(row, uiControl(uiNewLabel("")), 1);
	uiBoxAppend(box, uiControl(row), 0);

	values = uiNewButton("Swap values");
	uiButtonOnClicked(values, layoutSpinboxSetValues, NULL);
	uiBoxAppend(box, uiControl(values), 0);
	stabilityValues = uiNewLabel("Start 1 / End 1000");
	uiBoxAppend(box, uiControl(stabilityValues), 0);
	return uiControl(box);
}

static uiEntry *minimumEntry;
static uiSpinbox *minimumSpinbox;
static uiLabel *minimumStatus;
static int minimumStage;

static void layoutMinimumCycle(uiButton *button, void *data)
{
	static const int values[] = { -1, 0, 48, 160 };
	char text[32];
	int value;

	minimumStage = (minimumStage + 1) % 4;
	value = values[minimumStage];
	uiControlSetMinimumSize(uiControl(minimumEntry), value, -1);
	uiControlSetMinimumSize(uiControl(minimumSpinbox), value, -1);
	if (value == -1)
		uiLabelSetText(minimumStatus, "Minimum: default (-1)");
	else {
		snprintf(text, sizeof text, "Minimum: %d", value);
		uiLabelSetText(minimumStatus, text);
	}
}

const char *layoutMinimumSizeGuide(void)
{
	return
	"1. Entry was given a 160-point minimum before insertion; Spinbox received\n"
	"   the same minimum after insertion. They should be the same width.\n\n"
	"2. Click `Cycle minimum` through default, 0, 48, and 160. Both controls\n"
	"   must update together without constraint warnings.\n\n"
	"3. Resize narrower and wider after each setting. The explicit minimum is a\n"
	"   lower bound, not a fixed width, and resetting must restore natural size.\n";
}

uiControl *layoutMinimumSize(void)
{
	uiBox *box;
	uiBox *row;
	uiButton *cycle;

	minimumStage = 3;
	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	row = uiNewHorizontalBox();
	uiBoxSetPadded(row, 1);
	minimumEntry = uiNewEntry();
	uiControlSetMinimumSize(uiControl(minimumEntry), 160, -1);
	uiBoxAppend(row, uiControl(minimumEntry), 0);
	minimumSpinbox = uiNewSpinbox(0, 1000);
	uiBoxAppend(row, uiControl(minimumSpinbox), 0);
	uiControlSetMinimumSize(uiControl(minimumSpinbox), 160, -1);
	uiBoxAppend(row, uiControl(uiNewLabel("")), 1);
	uiBoxAppend(box, uiControl(row), 0);

	cycle = uiNewButton("Cycle minimum");
	uiButtonOnClicked(cycle, layoutMinimumCycle, NULL);
	uiBoxAppend(box, uiControl(cycle), 0);
	minimumStatus = uiNewLabel("Minimum: 160");
	uiBoxAppend(box, uiControl(minimumStatus), 0);
	return uiControl(box);
}

static uiGroup *radioGroup(const char *title, int count)
{
	uiGroup *group;
	uiRadioButtons *radio;

	group = uiNewGroup(title);
	uiGroupSetMargined(group, 1);
	radio = uiNewRadioButtons();
	if (count >= 1)
		uiRadioButtonsAppend(radio, "Short");
	if (count >= 2) {
		uiRadioButtonsAppend(radio, "A much longer radio-button title");
		uiRadioButtonsAppend(radio, "Medium title");
	}
	uiGroupSetChild(group, uiControl(radio));
	return group;
}

const char *layoutRadioButtonsGuide(void)
{
	return
	"1. Empty, one-item, and multi-item groups should fit their visible content.\n"
	"   The multi-item group must use the longest title without clipping.\n\n"
	"2. Resize both axes. The trailing blank area should absorb horizontal\n"
	"   surplus and the radio buttons should remain packed at the top.\n\n"
	"3. Select each item. Selection must not change group geometry.\n";
}

uiControl *layoutRadioButtons(void)
{
	uiBox *box;
	uiBox *row;

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	row = uiNewHorizontalBox();
	uiBoxSetPadded(row, 1);
	uiBoxAppend(row, uiControl(radioGroup("Empty", 0)), 0);
	uiBoxAppend(row, uiControl(radioGroup("One item", 1)), 0);
	uiBoxAppend(row, uiControl(radioGroup("Several items", 2)), 0);
	uiBoxAppend(row, uiControl(uiNewLabel("")), 1);
	uiBoxAppend(box, uiControl(row), 0);
	uiBoxAppend(box, uiControl(radioGroup("Vertical non-stretchy", 2)), 0);
	uiBoxAppend(box, uiControl(uiNewLabel("")), 1);
	return uiControl(box);
}

static uiBox *compactContents(const char *prefix, int stretchy)
{
	uiBox *box;
	uiBox *row;

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	uiBoxAppend(box, uiControl(uiNewLabel(prefix)), 0);
	row = uiNewHorizontalBox();
	uiBoxSetPadded(row, 1);
	uiBoxAppend(row, uiControl(uiNewButton("One")), stretchy);
	uiBoxAppend(row, uiControl(uiNewButton("Two")), stretchy);
	uiBoxAppend(box, uiControl(row), stretchy);
	return box;
}

static uiGroup *layoutGroup(const char *title, int stretchy)
{
	uiGroup *group;

	group = uiNewGroup(title);
	uiGroupSetMargined(group, 1);
	uiGroupSetChild(group, uiControl(compactContents("Group child", stretchy)));
	return group;
}

static uiTab *layoutTab(int stretchy)
{
	uiTab *tab;

	tab = uiNewTab();
	uiTabAppend(tab, "First", uiControl(compactContents("First page", stretchy)));
	uiTabAppend(tab, "Second", uiControl(compactContents("Second page", stretchy)));
	uiTabSetMargined(tab, 0, 1);
	uiTabSetMargined(tab, 1, 1);
	return tab;
}

const char *layoutContainersGuide(void)
{
	return
	"1. The direct Group and Tab should keep compact children packed at their\n"
	"   leading/top edges. Switch both tab pages and resize the window.\n\n"
	"2. In the nested row, Group and Tab are non-stretchy; the blank label at\n"
	"   the right receives horizontal surplus. Their internal layout must match\n"
	"   the direct versions.\n\n"
	"3. In the final row, the Group and Tab share horizontal surplus. Their\n"
	"   stretchy buttons should expand only inside their own container.\n";
}

uiControl *layoutContainers(void)
{
	uiBox *box;
	uiBox *direct;
	uiBox *expanding;
	uiBox *nested;

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	direct = uiNewHorizontalBox();
	uiBoxSetPadded(direct, 1);
	uiBoxAppend(direct, uiControl(layoutGroup("Direct group", 0)), 0);
	uiBoxAppend(direct, uiControl(layoutTab(0)), 0);
	uiBoxAppend(box, uiControl(direct), 0);

	nested = uiNewHorizontalBox();
	uiBoxSetPadded(nested, 1);
	uiBoxAppend(nested, uiControl(layoutGroup("Nested group", 0)), 0);
	uiBoxAppend(nested, uiControl(layoutTab(0)), 0);
	uiBoxAppend(nested, uiControl(uiNewLabel("")), 1);
	uiBoxAppend(box, uiControl(nested), 0);

	expanding = uiNewHorizontalBox();
	uiBoxSetPadded(expanding, 1);
	uiBoxAppend(expanding, uiControl(layoutGroup("Stretchy group", 1)), 1);
	uiBoxAppend(expanding, uiControl(layoutTab(1)), 1);
	uiBoxAppend(box, uiControl(expanding), 0);
	uiBoxAppend(box, uiControl(uiNewLabel("")), 1);
	return uiControl(box);
}

static void layoutToggleControl(uiCheckbox *checkbox, void *data)
{
	uiControl *control = data;

	if (uiCheckboxChecked(checkbox))
		uiControlShow(control);
	else
		uiControlHide(control);
}

const char *layoutGridGuide(void)
{
	return
	"1. The Entry column should receive horizontal surplus; the label and\n"
	"   Spinbox remain compact. The spanning button fills both columns.\n\n"
	"2. Resize both axes, then hide and show the spanning button. The grid must\n"
	"   rebuild without stale gaps, overlap, or a geometry jump in other rows.\n\n"
	"3. No Grid row is vertically expanding. The stretchy blank area below the\n"
	"   Grid must receive vertical surplus; no control row may become tall.\n";
}

uiControl *layoutGrid(void)
{
	uiBox *box;
	uiButton *spanning;
	uiCheckbox *visible;
	uiGrid *grid;

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	grid = uiNewGrid();
	uiGridSetPadded(grid, 1);
	uiGridAppend(grid, uiControl(uiNewLabel("Expandable entry")),
		0, 0, 1, 1, 0, uiAlignEnd, 0, uiAlignCenter);
	uiGridAppend(grid, uiControl(uiNewEntry()),
		1, 0, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);
	uiGridAppend(grid, uiControl(uiNewLabel("Compact spinbox")),
		0, 1, 1, 1, 0, uiAlignEnd, 0, uiAlignCenter);
	uiGridAppend(grid, uiControl(uiNewSpinbox(0, 1000)),
		1, 1, 1, 1, 0, uiAlignStart, 0, uiAlignCenter);
	spanning = uiNewButton("Two-column spanning control");
	uiGridAppend(grid, uiControl(spanning),
		0, 2, 2, 1, 1, uiAlignFill, 0, uiAlignCenter);
	uiBoxAppend(box, uiControl(grid), 0);
	visible = uiNewCheckbox("Show spanning control");
	uiCheckboxSetChecked(visible, 1);
	uiCheckboxOnToggled(visible, layoutToggleControl, spanning);
	uiBoxAppend(box, uiControl(visible), 0);
	uiBoxAppend(box, uiControl(uiNewLabel("")), 1);
	return uiControl(box);
}

static void layoutTallSpinbox(uiCheckbox *checkbox, void *data)
{
	uiControlSetMinimumSize(data, -1,
		uiCheckboxChecked(checkbox) ? 80 : -1);
}

const char *layoutSpinboxBaselineGuide(void)
{
	return
	"1. At natural height, Label, Entry, and Spinbox text baselines should align.\n\n"
	"2. Toggle the 80-point Spinbox height. Its field and stepper must remain at\n"
	"   natural height and stay vertically centered; neither child may stretch.\n\n"
	"3. The tall row's label baseline must follow the Spinbox field baseline.\n"
	"   Repeated toggles must restore the original row without warnings.\n";
}

uiControl *layoutSpinboxBaseline(void)
{
	uiBox *box;
	uiCheckbox *tall;
	uiForm *form;
	uiSpinbox *spinbox;

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	form = uiNewForm();
	uiFormSetPadded(form, 1);
	uiFormAppend(form, "Entry baseline", uiControl(uiNewEntry()), 0);
	uiFormAppend(form, "Natural Spinbox", uiControl(uiNewSpinbox(0, 1000)), 0);
	spinbox = uiNewSpinbox(0, 1000);
	uiFormAppend(form, "Variable-height Spinbox", uiControl(spinbox), 0);
	uiBoxAppend(box, uiControl(form), 0);
	tall = uiNewCheckbox("Use 80-point Spinbox minimum height");
	uiCheckboxOnToggled(tall, layoutTallSpinbox, spinbox);
	uiBoxAppend(box, uiControl(tall), 0);
	uiBoxAppend(box, uiControl(uiNewLabel("")), 1);
	return uiControl(box);
}
