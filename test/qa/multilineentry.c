#include <stdio.h>

#include "qa.h"

struct multilineEntryFontSizeState {
	uiMultilineEntry *wrapping;
	uiMultilineEntry *nonwrapping;
	uiLabel *changed;
	int changeCount;
};

static struct multilineEntryFontSizeState fontSizeState;

static void updateChangeCount(void)
{
	char text[32];

	snprintf(text, sizeof text, "OnChanged count: %d",
		fontSizeState.changeCount);
	uiLabelSetText(fontSizeState.changed, text);
}

static void onMultilineEntryChanged(uiMultilineEntry *e, void *data)
{
	fontSizeState.changeCount++;
	updateChangeCount();
}

static void setFontSize(double size)
{
	uiMultilineEntrySetFontSize(fontSizeState.wrapping, size);
	uiMultilineEntrySetFontSize(fontSizeState.nonwrapping, size);
}

static void set8Point(uiButton *button, void *data)
{
	setFontSize(8);
}

static void set18Point(uiButton *button, void *data)
{
	setFontSize(18);
}

static void set32Point(uiButton *button, void *data)
{
	setFontSize(32);
}

static void resetFontSize(uiButton *button, void *data)
{
	uiMultilineEntryResetFontSize(fontSizeState.wrapping);
	uiMultilineEntryResetFontSize(fontSizeState.nonwrapping);
}

static void appendText(uiButton *button, void *data)
{
	uiMultilineEntryAppend(fontSizeState.wrapping, "\nAppended text");
	uiMultilineEntryAppend(fontSizeState.nonwrapping, "\nAppended text");
}

static void setText(uiButton *button, void *data)
{
	const char *text = "Text replaced after changing the font size. "
		"This sentence is long enough to demonstrate wrapping behavior.";

	uiMultilineEntrySetText(fontSizeState.wrapping, text);
	uiMultilineEntrySetText(fontSizeState.nonwrapping, text);
}

static void setReadOnly(uiCheckbox *checkbox, void *data)
{
	int readonly;

	readonly = uiCheckboxChecked(checkbox);
	uiMultilineEntrySetReadOnly(fontSizeState.wrapping, readonly);
	uiMultilineEntrySetReadOnly(fontSizeState.nonwrapping, readonly);
}

static uiButton *fontSizeButton(const char *text,
	void (*callback)(uiButton *, void *))
{
	uiButton *button;

	button = uiNewButton(text);
	uiButtonOnClicked(button, callback, NULL);
	return button;
}

const char *multilineEntryFontSizeGuide(void)
{
	return
	"1.\tUse the 8 pt, 18 pt, and 32 pt buttons. Existing text in both\n"
	"\tentries should visibly change to the same requested size.\n"
	"2.\tThe first entry should wrap; the second should scroll horizontally.\n"
	"3.\tType into both entries after changing size. New text should match.\n"
	"4.\tAppend text and Set text should preserve the chosen font size.\n"
	"5.\tDefault should restore the original native size and remain safe\n"
	"\twhen clicked repeatedly.\n"
	"6.\tFont-size, append, and set-text buttons must not increment the\n"
	"\tOnChanged count. Direct typing should increment it.\n"
	"7.\tRead only should prevent editing without preventing size changes.\n"
	"8.\tSelection, scrolling, colors, and light/dark theme should remain\n"
	"\tnative and usable at every size.";
}

uiControl *multilineEntryFontSize(void)
{
	uiBox *box;
	uiBox *buttons;
	uiCheckbox *readonly;
	const char *text = "The quick brown fox jumps over the lazy dog. "
		"This sentence demonstrates multiline wrapping and scrolling.";

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	buttons = uiNewHorizontalBox();
	uiBoxSetPadded(buttons, 1);
	uiBoxAppend(buttons, uiControl(fontSizeButton("8 pt", set8Point)), 0);
	uiBoxAppend(buttons, uiControl(fontSizeButton("18 pt", set18Point)), 0);
	uiBoxAppend(buttons, uiControl(fontSizeButton("32 pt", set32Point)), 0);
	uiBoxAppend(buttons, uiControl(fontSizeButton("Default", resetFontSize)), 0);
	uiBoxAppend(buttons, uiControl(fontSizeButton("Append text", appendText)), 0);
	uiBoxAppend(buttons, uiControl(fontSizeButton("Set text", setText)), 0);
	uiBoxAppend(box, uiControl(buttons), 0);

	fontSizeState.wrapping = uiNewMultilineEntry();
	fontSizeState.nonwrapping = uiNewNonWrappingMultilineEntry();
	uiMultilineEntrySetText(fontSizeState.wrapping, text);
	uiMultilineEntrySetText(fontSizeState.nonwrapping, text);
	uiMultilineEntryOnChanged(fontSizeState.wrapping,
		onMultilineEntryChanged, NULL);
	uiMultilineEntryOnChanged(fontSizeState.nonwrapping,
		onMultilineEntryChanged, NULL);
	uiBoxAppend(box, uiControl(fontSizeState.wrapping), 1);
	uiBoxAppend(box, uiControl(fontSizeState.nonwrapping), 1);

	fontSizeState.changeCount = 0;
	fontSizeState.changed = uiNewLabel("");
	updateChangeCount();
	uiBoxAppend(box, uiControl(fontSizeState.changed), 0);
	readonly = uiNewCheckbox("Read only");
	uiCheckboxOnToggled(readonly, setReadOnly, NULL);
	uiBoxAppend(box, uiControl(readonly), 0);

	return uiControl(box);
}
