// 14 august 2015
#import "uipriv_darwin.h"

struct uiDateTimePicker {
	uiDarwinControl c;
	NSDatePicker *dp;
	void (*onChanged)(uiDateTimePicker *, void *);
	void *onChangedData;
};

@interface uiprivDatePicker : NSDatePicker {
	uiDateTimePicker *picker;
}
- (id)initWithElements:(NSDatePickerElementFlags)elements uiDateTimePicker:(uiDateTimePicker *)d;
- (IBAction)onChanged:(id)sender;
@end

@implementation uiprivDatePicker

- (id)initWithElements:(NSDatePickerElementFlags)elements uiDateTimePicker:(uiDateTimePicker *)d
{
	self = [super initWithFrame:NSZeroRect];
	if (self) {
		self->picker = d;

		[self setDateValue:[NSDate date]];
		[self setBordered:NO];
		[self setBezeled:YES];
		[self setDrawsBackground:YES];
		[self setDatePickerStyle:NSTextFieldAndStepperDatePickerStyle];
		[self setDatePickerElements:elements];
		[self setDatePickerMode:NSSingleDateMode];

		[self setTarget:self];
		[self setAction:@selector(onChanged:)];
	}
	return self;
}

- (IBAction)onChanged:(id)sender
{
	uiDateTimePicker *d;

	d = self->picker;
	if (d == NULL)
		return;
	(*(d->onChanged))(d, d->onChangedData);
}
@end

uiDarwinControlAllDefaultsExceptDestroy(uiDateTimePicker, dp)

static void uiDateTimePickerDestroy(uiControl *c)
{
	uiDateTimePicker *d = uiDateTimePicker(c);

	[d->dp release];
	uiFreeControl(uiControl(d));
}

static void defaultOnChanged(uiDateTimePicker *d, void *data)
{
	// do nothing
}

void uiDateTimePickerTime(uiDateTimePicker *d, struct tm *time)
{
	time_t t;
	struct tm tmbuf;
	NSDate *date;

	date = [d->dp dateValue];
	t = (time_t) [date timeIntervalSince1970];

	// Copy time to minimize a race condition
	// time.h functions use global non-thread-safe data
	tmbuf = *localtime(&t);
	memcpy(time, &tmbuf, sizeof (struct tm));
}

void uiDateTimePickerSetTime(uiDateTimePicker *d, const struct tm *time)
{
	time_t t;
	struct tm tmbuf;

	// Copy time because mktime() modifies its argument
	memcpy(&tmbuf, time, sizeof (struct tm));
	t = mktime(&tmbuf);

	[d->dp setDateValue:[NSDate dateWithTimeIntervalSince1970:t]];
}

void uiDateTimePickerOnChanged(uiDateTimePicker *d, void (*f)(uiDateTimePicker *, void *), void *data)
{
	d->onChanged = f;
	d->onChangedData = data;
}

static uiDateTimePicker *finishNewDateTimePicker(NSDatePickerElementFlags elements)
{
	uiDateTimePicker *d;

	uiDarwinNewControl(uiDateTimePicker, d);

	d->dp = [[uiprivDatePicker alloc] initWithElements:elements uiDateTimePicker:d];
	uiDarwinSetControlFont(d->dp, NSRegularControlSize);

	uiDateTimePickerOnChanged(d, defaultOnChanged, NULL);

	return d;
}

uiDateTimePicker *uiNewDateTimePicker(void)
{
	return finishNewDateTimePicker(NSYearMonthDayDatePickerElementFlag | NSHourMinuteSecondDatePickerElementFlag);
}

uiDateTimePicker *uiNewDatePicker(void)
{
	return finishNewDateTimePicker(NSYearMonthDayDatePickerElementFlag);
}

uiDateTimePicker *uiNewTimePicker(void)
{
	return finishNewDateTimePicker(NSHourMinuteSecondDatePickerElementFlag);
}
