// 4 september 2015
#include "uipriv_unix.h"

// LONGTERM imitate gnome-calendar's day/month/year entries above the calendar
// LONGTERM allow entering a 24-hour hour in the hour spinbutton and adjust accordingly

#define uiprivDateTimePickerWidgetType (uiprivDateTimePickerWidget_get_type())
#define uiprivDateTimePickerWidget(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), uiprivDateTimePickerWidgetType, uiprivDateTimePickerWidget))

typedef struct uiprivDateTimePickerWidget uiprivDateTimePickerWidget;
typedef struct uiprivDateTimePickerWidgetClass uiprivDateTimePickerWidgetClass;

struct uiprivDateTimePickerWidget {
	GtkMenuButton parent_instance;

	gboolean hasTime;
	gboolean hasDate;

	GtkWidget *popover;
	GtkWidget *box;
	GtkWidget *calendar;
	GtkWidget *timebox;
	GtkWidget *hours;
	GtkWidget *minutes;
	GtkWidget *seconds;
	GtkWidget *ampm;

	gulong calendarBlock;
	gulong hoursBlock;
	gulong minutesBlock;
	gulong secondsBlock;
	gulong ampmBlock;
};

struct uiprivDateTimePickerWidgetClass {
	GtkMenuButtonClass parent_class;
};

G_DEFINE_TYPE(uiprivDateTimePickerWidget, uiprivDateTimePickerWidget, GTK_TYPE_MENU_BUTTON)

static int realSpinValue(GtkSpinButton *spinButton)
{
	GtkAdjustment *adj;

	adj = gtk_spin_button_get_adjustment(spinButton);
	return (int) gtk_adjustment_get_value(adj);
}

static void setRealSpinValue(GtkSpinButton *spinButton, int value, gulong block)
{
	GtkAdjustment *adj;

	g_signal_handler_block(spinButton, block);
	adj = gtk_spin_button_get_adjustment(spinButton);
	gtk_adjustment_set_value(adj, value);
	g_signal_handler_unblock(spinButton, block);
}

static GDateTime *selected(uiprivDateTimePickerWidget *d)
{
	// choose a day for which all times are likely to be valid for the default date in case we're only dealing with time
	guint year = 1970, month = 1, day = 1;
	guint hour = 0, minute = 0, second = 0;

	if (d->hasDate) {
		gtk_calendar_get_date(GTK_CALENDAR(d->calendar), &year, &month, &day);
		month++;		// GtkCalendar/GDateTime differences
	}
	if (d->hasTime) {
		hour = realSpinValue(GTK_SPIN_BUTTON(d->hours));
		if (realSpinValue(GTK_SPIN_BUTTON(d->ampm)) != 0)
			hour += 12;
		minute = realSpinValue(GTK_SPIN_BUTTON(d->minutes));
		second = realSpinValue(GTK_SPIN_BUTTON(d->seconds));
	}
	return g_date_time_new_local(year, month, day, hour, minute, second);
}

static void setLabel(uiprivDateTimePickerWidget *d)
{
	GDateTime *dt;
	char *fmt;
	char *msg;
	gboolean free;

	dt = selected(d);
	free = FALSE;
	if (d->hasDate && d->hasTime) {
		// don't use D_T_FMT; that's too verbose
		fmt = g_strdup_printf("%s %s", nl_langinfo(D_FMT), nl_langinfo(T_FMT));
		free = TRUE;
	} else if (d->hasDate)
		fmt = nl_langinfo(D_FMT);
	else
		fmt = nl_langinfo(T_FMT);
	msg = g_date_time_format(dt, fmt);
	gtk_button_set_label(GTK_BUTTON(d), msg);
	g_free(msg);
	if (free)
		g_free(fmt);
	g_date_time_unref(dt);
}

static int changedSignal;

static void dateTimeChanged(uiprivDateTimePickerWidget *d)
{
	g_signal_emit(d, changedSignal, 0);
	setLabel(d);
}

static gint hoursSpinboxInput(GtkSpinButton *sb, gpointer ptr, gpointer data)
{
	double *out = (double *) ptr;
	const gchar *text;
	int value;

	text = gtk_entry_get_text(GTK_ENTRY(sb));
	value = (int) g_strtod(text, NULL);
	if (value < 0 || value > 12)
		return GTK_INPUT_ERROR;
	if (value == 12)		// 12 to the user is 0 internally
		value = 0;
	*out = (double) value;
	return TRUE;
}

static gboolean hoursSpinboxOutput(GtkSpinButton *sb, gpointer data)
{
	gchar *text;
	int value;

	value = realSpinValue(sb);
	if (value == 0)		// 0 internally is 12 to the user
		value = 12;
	text = g_strdup_printf("%d", value);
	gtk_entry_set_text(GTK_ENTRY(sb), text);
	g_free(text);
	return TRUE;
}

static gboolean zeroPadSpinbox(GtkSpinButton *sb, gpointer data)
{
	gchar *text;
	int value;

	value = realSpinValue(sb);
	text = g_strdup_printf("%02d", value);
	gtk_entry_set_text(GTK_ENTRY(sb), text);
	g_free(text);
	return TRUE;
}

// this is really hacky but we can't use GtkCombobox here :(
static gint ampmSpinboxInput(GtkSpinButton *sb, gpointer ptr, gpointer data)
{
	double *out = (double *) ptr;
	const gchar *text;
	char firstAM, firstPM;

	text = gtk_entry_get_text(GTK_ENTRY(sb));
	// LONGTERM don't use ASCII here for case insensitivity
	firstAM = g_ascii_tolower(nl_langinfo(AM_STR)[0]);
	firstPM = g_ascii_tolower(nl_langinfo(PM_STR)[0]);
	for (; *text != '\0'; text++)
		if (g_ascii_tolower(*text) == firstAM) {
			*out = 0;
			return TRUE;
		} else if (g_ascii_tolower(*text) == firstPM) {
			*out = 1;
			return TRUE;
		}
	return GTK_INPUT_ERROR;
}

static gboolean ampmSpinboxOutput(GtkSpinButton *sb, gpointer data)
{
	int value;

	value = gtk_spin_button_get_value_as_int(sb);
	if (value == 0)
		gtk_entry_set_text(GTK_ENTRY(sb), nl_langinfo(AM_STR));
	else
		gtk_entry_set_text(GTK_ENTRY(sb), nl_langinfo(PM_STR));
	return TRUE;
}

static void spinboxChanged(GtkSpinButton *sb, gpointer data)
{
	uiprivDateTimePickerWidget *d = uiprivDateTimePickerWidget(data);

	dateTimeChanged(d);
}

static GtkWidget *newSpinbox(uiprivDateTimePickerWidget *d, int min, int max, gint (*input)(GtkSpinButton *, gpointer, gpointer), gboolean (*output)(GtkSpinButton *, gpointer), gulong *block)
{
	GtkWidget *sb;

	sb = gtk_spin_button_new_with_range(min, max, 1);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(sb), 0);
	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(sb), TRUE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(sb), GTK_ORIENTATION_VERTICAL);
	*block = g_signal_connect(sb, "value-changed", G_CALLBACK(spinboxChanged), d);
	if (input != NULL)
		g_signal_connect(sb, "input", G_CALLBACK(input), NULL);
	if (output != NULL)
		g_signal_connect(sb, "output", G_CALLBACK(output), NULL);
	return sb;
}

static void dateChanged(GtkCalendar *c, gpointer data)
{
	uiprivDateTimePickerWidget *d = uiprivDateTimePickerWidget(data);

	dateTimeChanged(d);
}

static void setDateOnly(uiprivDateTimePickerWidget *d)
{
	d->hasTime = FALSE;
	gtk_container_remove(GTK_CONTAINER(d->box), d->timebox);
}

static void setTimeOnly(uiprivDateTimePickerWidget *d)
{
	d->hasDate = FALSE;
	gtk_container_remove(GTK_CONTAINER(d->box), d->calendar);
}

static void uiprivDateTimePickerWidget_setTime(uiprivDateTimePickerWidget *d, GDateTime *dt)
{
	gint year, month, day;
	gint hour;

	// notice how we block signals from firing
	if (d->hasDate) {
		g_date_time_get_ymd(dt, &year, &month, &day);
		month--;			// GDateTime/GtkCalendar differences
		g_signal_handler_block(d->calendar, d->calendarBlock);
		gtk_calendar_select_month(GTK_CALENDAR(d->calendar), month, year);
		gtk_calendar_select_day(GTK_CALENDAR(d->calendar), day);
		g_signal_handler_unblock(d->calendar, d->calendarBlock);
	}
	if (d->hasTime) {
		hour = g_date_time_get_hour(dt);
		if (hour >= 12) {
			hour -= 12;
			setRealSpinValue(GTK_SPIN_BUTTON(d->ampm), 1, d->ampmBlock);
		} else
			setRealSpinValue(GTK_SPIN_BUTTON(d->ampm), 0, d->ampmBlock);
		setRealSpinValue(GTK_SPIN_BUTTON(d->hours), hour, d->hoursBlock);
		setRealSpinValue(GTK_SPIN_BUTTON(d->minutes), g_date_time_get_minute(dt), d->minutesBlock);
		setRealSpinValue(GTK_SPIN_BUTTON(d->seconds), g_date_time_get_seconds(dt), d->secondsBlock);
	}
	g_date_time_unref(dt);
}

static void uiprivDateTimePickerWidget_init(uiprivDateTimePickerWidget *d)
{
	d->popover = gtk_popover_new(GTK_WIDGET(d));
	gtk_popover_set_position(GTK_POPOVER(d->popover), GTK_POS_BOTTOM);
	gtk_menu_button_set_popover(GTK_MENU_BUTTON(d), d->popover);
	gtk_container_set_border_width(GTK_CONTAINER(d->popover), 12);

	d->box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_container_add(GTK_CONTAINER(d->popover), d->box);

	d->calendar = gtk_calendar_new();
	d->calendarBlock = g_signal_connect(d->calendar, "day-selected", G_CALLBACK(dateChanged), d);
	gtk_container_add(GTK_CONTAINER(d->box), d->calendar);

	d->timebox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_set_valign(d->timebox, GTK_ALIGN_CENTER);
	gtk_container_add(GTK_CONTAINER(d->box), d->timebox);

	d->hours = newSpinbox(d, 0, 11, hoursSpinboxInput, hoursSpinboxOutput, &(d->hoursBlock));
	gtk_container_add(GTK_CONTAINER(d->timebox), d->hours);

	gtk_container_add(GTK_CONTAINER(d->timebox),
		gtk_label_new(":"));

	d->minutes = newSpinbox(d, 0, 59, NULL, zeroPadSpinbox, &(d->minutesBlock));
	gtk_container_add(GTK_CONTAINER(d->timebox), d->minutes);

	gtk_container_add(GTK_CONTAINER(d->timebox),
		gtk_label_new(":"));

	d->seconds = newSpinbox(d, 0, 59, NULL, zeroPadSpinbox, &(d->secondsBlock));
	gtk_container_add(GTK_CONTAINER(d->timebox), d->seconds);

	d->ampm = newSpinbox(d, 0, 1, ampmSpinboxInput, ampmSpinboxOutput, &(d->ampmBlock));
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(d->ampm), FALSE);
	gtk_widget_set_valign(d->ampm, GTK_ALIGN_CENTER);
	gtk_container_add(GTK_CONTAINER(d->timebox), d->ampm);

	gtk_widget_show_all(d->box);

	d->hasTime = TRUE;
	d->hasDate = TRUE;

	// set the current date/time
	uiprivDateTimePickerWidget_setTime(d, g_date_time_new_now_local());
}

static void uiprivDateTimePickerWidget_class_init(uiprivDateTimePickerWidgetClass *class)
{
	changedSignal = g_signal_new("changed",
		G_TYPE_FROM_CLASS(class),
		G_SIGNAL_RUN_LAST,
		0,
		NULL, NULL, NULL,
		G_TYPE_NONE,
		0);
}

struct uiDateTimePicker {
	uiUnixControl c;
	GtkWidget *widget;
	uiprivDateTimePickerWidget *d;
	void (*onChanged)(uiDateTimePicker *, void *);
	void *onChangedData;
	gulong setBlock;
};

uiUnixControlAllDefaultsExceptDestroy(uiDateTimePicker)

static void uiDateTimePickerDestroy(uiControl *c)
{
	uiDateTimePicker *d = uiDateTimePicker(c);

	if (d->setBlock != 0) {
		g_signal_handler_disconnect(d->d, d->setBlock);
		d->setBlock = 0;
	}
	g_object_unref(d->widget);
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
	GDateTime *dt;

	dt = selected(d->d);
	t = g_date_time_to_unix(dt);
	g_date_time_unref(dt);

	// Copy time to minimize a race condition
	// time.h functions use global non-thread-safe data
	tmbuf = *localtime(&t);
	memcpy(time, &tmbuf, sizeof (struct tm));
}

void uiDateTimePickerSetTime(uiDateTimePicker *d, const struct tm *time)
{
	time_t t;
	struct tm tmbuf;

	// TODO find a better way to avoid this; possibly by removing the signal entirely, or the call to dateTimeChanged() (most likely both)
	g_signal_handler_block(d->d, d->setBlock);

	// Copy time because mktime() modifies its argument
	memcpy(&tmbuf, time, sizeof (struct tm));
	t = mktime(&tmbuf);

	uiprivDateTimePickerWidget_setTime(d->d, g_date_time_new_from_unix_local(t));
	dateTimeChanged(d->d);

	g_signal_handler_unblock(d->d, d->setBlock);
}

void uiDateTimePickerOnChanged(uiDateTimePicker *d, void (*f)(uiDateTimePicker *, void *), void *data)
{
	d->onChanged = f;
	d->onChangedData = data;
}

static void onChanged(uiprivDateTimePickerWidget *d, gpointer data)
{
	uiDateTimePicker *c;

	c = uiDateTimePicker(data);
	(*(c->onChanged))(c, c->onChangedData);
}

static GtkWidget *newDTP(void)
{
	GtkWidget *w;

	w = GTK_WIDGET(g_object_new(uiprivDateTimePickerWidgetType, "label", "", NULL));
	setLabel(uiprivDateTimePickerWidget(w));
	return w;
}

static GtkWidget *newDP(void)
{
	GtkWidget *w;

	w = GTK_WIDGET(g_object_new(uiprivDateTimePickerWidgetType, "label", "", NULL));
	setDateOnly(uiprivDateTimePickerWidget(w));
	setLabel(uiprivDateTimePickerWidget(w));
	return w;
}

static GtkWidget *newTP(void)
{
	GtkWidget *w;

	w = GTK_WIDGET(g_object_new(uiprivDateTimePickerWidgetType, "label", "", NULL));
	setTimeOnly(uiprivDateTimePickerWidget(w));
	setLabel(uiprivDateTimePickerWidget(w));
	return w;
}

uiDateTimePicker *finishNewDateTimePicker(GtkWidget *(*fn)(void))
{
	uiDateTimePicker *d;

	uiUnixNewControl(uiDateTimePicker, d);

	d->widget = (*fn)();
	d->d = uiprivDateTimePickerWidget(d->widget);
	d->setBlock = g_signal_connect(d->widget, "changed", G_CALLBACK(onChanged), d);
	uiDateTimePickerOnChanged(d, defaultOnChanged, NULL);

	return d;
}

uiDateTimePicker *uiNewDateTimePicker(void)
{
	return finishNewDateTimePicker(newDTP);
}

uiDateTimePicker *uiNewDatePicker(void)
{
	return finishNewDateTimePicker(newDP);
}

uiDateTimePicker *uiNewTimePicker(void)
{
	return finishNewDateTimePicker(newTP);
}
