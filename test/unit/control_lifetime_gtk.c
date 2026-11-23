#include <stdio.h>
#include <gtk/gtk.h>
#include "../../ui.h"
#include "../../ui_unix.h"

/* Exercise the public macros used by third-party controls as well. */
typedef struct externalControl externalControl;
struct externalControl {
	uiUnixControl c;
	GtkWidget *widget;
};
#define externalControl(this) ((externalControl *) (this))
#define externalControlSignature 0x45585443
uiUnixControlAllDefaults(externalControl)

static uiControl *newExternal(void)
{
	externalControl *c;

	uiUnixNewControl(externalControl, c);
	c->widget = gtk_button_new();
	return uiControl(c);
}

static int numColumns(uiTableModelHandler *handler, uiTableModel *model)
{
	return 1;
}

static uiTableValueType columnType(uiTableModelHandler *handler, uiTableModel *model, int column)
{
	return uiTableValueTypeString;
}

static int numRows(uiTableModelHandler *handler, uiTableModel *model)
{
	return 0;
}

static uiTableValue *cellValue(uiTableModelHandler *handler, uiTableModel *model, int row, int column)
{
	return uiNewTableValueString("");
}

static void setCellValue(uiTableModelHandler *handler, uiTableModel *model, int row, int column, const uiTableValue *value)
{
}

static void finalized(gpointer data, GObject *object)
{
	int *count = data;

	(*count)++;
}

enum destroyMode {
	destroyUnparented,
	destroyDetached,
	destroyMinimumSize,
	destroyWithParent,
	destroyRetained,
	destroyModeCount
};

static int checkDestroy(uiControl *control, const char *name, enum destroyMode mode)
{
	GtkWidget *widget = GTK_WIDGET((void *) uiControlHandle(control));
	uiBox *box = NULL;
	int count = 0;

	g_object_weak_ref(G_OBJECT(widget), finalized, &count);
	if (mode == destroyDetached || mode == destroyWithParent) {
		box = uiNewVerticalBox();
		uiBoxAppend(box, control, 0);
		if (mode == destroyDetached)
			uiBoxDelete(box, 0);
	} else if (mode == destroyMinimumSize) {
		uiControlSetMinimumSize(control, 30, 20);
	} else if (mode == destroyRetained) {
		g_object_ref(widget);
	}
	if (mode != destroyWithParent)
		uiControlDestroy(control);
	if (box != NULL)
		uiControlDestroy(uiControl(box));
	if (mode == destroyRetained) {
		if (count != 0) {
			fprintf(stderr, "%s: widget finalized with an external reference\n", name);
			return 1;
		}
		g_object_unref(widget);
	}
	if (count != 1) {
		fprintf(stderr, "%s mode %d: widget finalized %d times\n", name, mode, count);
		return 1;
	}
	return 0;
}

#define CHECK(name, expression) do { \
	for (enum destroyMode mode = 0; mode < destroyModeCount; mode++) \
		if (checkDestroy(uiControl(expression), name, mode)) \
			return 1; \
} while (0)

int main(void)
{
	uiInitOptions options = {0};
	uiAreaHandler areaHandler = {0};
	uiTableModelHandler tableHandler = {numColumns, columnType, numRows, cellValue, setCellValue};
	uiTableModel *model;
	uiTableParams tableParams;
	const char *error;

	error = uiInit(&options);
	if (error != NULL) {
		fprintf(stderr, "uiInit: %s\n", error);
		uiFreeInitError(error);
		return 1;
	}
	/* Warnings expose floating finalization; criticals catch bad references. */
	g_log_set_always_fatal(G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL);
	CHECK("external", newExternal());
	CHECK("button", uiNewButton("button"));
	CHECK("font button", uiNewFontButton());
	CHECK("entry", uiNewEntry());
	CHECK("label", uiNewLabel("label"));
	CHECK("checkbox", uiNewCheckbox("check"));
	CHECK("combobox", uiNewCombobox());
	CHECK("color button", uiNewColorButton());
	CHECK("spinbox", uiNewSpinbox(0, 10));
	CHECK("slider", uiNewSlider(0, 10));
	CHECK("multiline entry", uiNewMultilineEntry());
	CHECK("box", uiNewVerticalBox());
	CHECK("group", uiNewGroup("group"));
	CHECK("form", uiNewForm());
	CHECK("grid", uiNewGrid());
	CHECK("tab", uiNewTab());
	CHECK("progress bar", uiNewProgressBar());
	CHECK("radio buttons", uiNewRadioButtons());
	CHECK("date time picker", uiNewDateTimePicker());
	CHECK("area", uiNewArea(&areaHandler));
	model = uiNewTableModel(&tableHandler);
	tableParams.Model = model;
	tableParams.RowBackgroundColorModelColumn = -1;
	CHECK("table", uiNewTable(&tableParams));
	uiFreeTableModel(model);
	uiUninit();
	return 0;
}
