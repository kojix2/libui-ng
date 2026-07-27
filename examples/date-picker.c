#include <stdio.h>
#include <time.h>
#include <ui.h>

static void updateLabel(uiDateTimePicker *picker, uiLabel *label)
{
	struct tm timebuf;
	char text[64];

	uiDateTimePickerTime(picker, &timebuf);
	strftime(text, sizeof text, "Date: %x", &timebuf);
	uiLabelSetText(label, text);
}

static void onDateChanged(uiDateTimePicker *picker, void *data)
{
	updateLabel(picker, uiLabel(data));
}

static int onClosing(uiWindow *w, void *data)
{
	uiQuit();
	return 1;
}

int main(void)
{
	uiInitOptions o = {0};
	const char *err;
	uiWindow *w;
	uiBox *box;
	uiDateTimePicker *picker;
	uiLabel *label;
	time_t now;
	struct tm timebuf;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("Date Picker", 320, 160, 0);
	uiWindowSetMargined(w, 1);
	uiWindowOnClosing(w, onClosing, NULL);

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	uiWindowSetChild(w, uiControl(box));

	picker = uiNewDatePicker();
	now = time(NULL);
	timebuf = *localtime(&now);
	uiDateTimePickerSetTime(picker, &timebuf);
	uiBoxAppend(box, uiControl(picker), 0);

	label = uiNewLabel("");
	updateLabel(picker, label);
	uiDateTimePickerOnChanged(picker, onDateChanged, label);
	uiBoxAppend(box, uiControl(label), 0);

	uiControlShow(uiControl(w));
	uiMain();
	uiUninit();
	return 0;
}
