#include "test.h"

uiBox *makeDateTime(void)
{
	uiBox *page;

	page = newVerticalBox();
	uiBoxAppend(page, uiControl(uiNewLabel("Date and time")), 0);
	uiBoxAppend(page, uiControl(uiNewDateTimePicker()), 0);
	uiBoxAppend(page, uiControl(uiNewLabel("Date only")), 0);
	uiBoxAppend(page, uiControl(uiNewDatePicker()), 0);
	uiBoxAppend(page, uiControl(uiNewLabel("Time only")), 0);
	uiBoxAppend(page, uiControl(uiNewTimePicker()), 0);
	return page;
}
