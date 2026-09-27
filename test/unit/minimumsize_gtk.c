#include <stdio.h>
#include <gtk/gtk.h>
#include "../../ui.h"
#include "../../ui_unix.h"

typedef struct externalControl externalControl;
struct externalControl {
	uiUnixControl c;
	GtkWidget *widget;
};
#define externalControl(this) ((externalControl *) (this))
#define externalControlSignature 0x4D534558
uiUnixControlAllDefaults(externalControl)

static int minimumWidth(GtkWidget *widget)
{
	int minimum, natural;

	gtk_widget_get_preferred_width(widget, &minimum, &natural);
	return minimum;
}

static int minimumHeight(GtkWidget *widget)
{
	int minimum, natural;

	gtk_widget_get_preferred_height(widget, &minimum, &natural);
	return minimum;
}

static int testBox(void)
{
	uiInitOptions options = {0};
	uiWindow *window;
	uiBox *box;
	uiEntry *first, *second;
	GtkWidget *rawFirst, *rawBox;
	int defaultSecondWidth, structuralWidth;
	int result = 1;

	if (uiInit(&options) != NULL)
		return 1;
	// Treat invalid GTK object access as a test failure, even when GTK would
	// otherwise only print a critical and keep running.
	g_log_set_always_fatal(G_LOG_LEVEL_CRITICAL);
	first = uiNewEntry();
	rawFirst = GTK_WIDGET(uiControlHandle(uiControl(first)));
	uiControlSetMinimumSize(uiControl(first), 40, -1);
	uiControlSetMinimumSize(uiControl(first), -1, -1);
	if (!GTK_IS_WIDGET(rawFirst) || gtk_widget_get_parent(rawFirst) != NULL) {
		fprintf(stderr, "pre-insertion minimum-size reset lost its widget\n");
		goto cleanupFirst;
	}

	window = uiNewWindow("minimum-size test", 320, 100, 0);
	box = uiNewHorizontalBox();
	second = uiNewEntry();
	uiBoxAppend(box, uiControl(first), 1);
	uiBoxAppend(box, uiControl(second), 1);
	uiWindowSetChild(window, uiControl(box));
	uiControlShow(uiControl(window));
	uiMainSteps();
	uiMainStep(0);

	defaultSecondWidth = minimumWidth(GTK_WIDGET(uiControlHandle(uiControl(second))));
	uiControlSetMinimumSize(uiControl(first), 300, -1);
	if (minimumWidth(GTK_WIDGET(uiControlHandle(uiControl(second)))) < 300) {
		fprintf(stderr, "stretchy size group did not follow the wrapper\n");
		goto cleanupWindow;
	}
	uiControlSetMinimumSize(uiControl(first), -1, -1);
	if (minimumWidth(GTK_WIDGET(uiControlHandle(uiControl(second)))) != defaultSecondWidth) {
		fprintf(stderr, "stretchy size group did not follow the reset\n");
		goto cleanupWindow;
	}
	uiControlSetMinimumSize(uiControl(first), 300, -1);
	uiBoxDelete(box, 0);
	uiControlDestroy(uiControl(first));
	if (minimumWidth(GTK_WIDGET(uiControlHandle(uiControl(second)))) != defaultSecondWidth) {
		fprintf(stderr, "deleted stretchy child remained in the size group\n");
		goto cleanupWindow;
	}

	rawBox = GTK_WIDGET(uiControlHandle(uiControl(box)));
	structuralWidth = minimumWidth(rawBox);
	uiControlSetMinimumSize(uiControl(box), 0, -1);
	if (structuralWidth <= 0 || gtk_widget_get_parent(rawBox) == NULL ||
		minimumWidth(gtk_widget_get_parent(rawBox)) < structuralWidth) {
		fprintf(stderr, "container minimum became smaller than its child\n");
		goto cleanupWindow;
	}
	uiControlSetMinimumSize(uiControl(box), -1, -1);
	result = 0;

cleanupWindow:
	uiControlDestroy(uiControl(window));
	uiUninit();
	return result;
cleanupFirst:
	uiControlDestroy(uiControl(first));
	uiUninit();
	return result;
}

static int testForm(void)
{
	uiInitOptions options = {0};
	uiWindow *window;
	uiForm *form;
	uiMultilineEntry *first, *second;
	GtkWidget *rawFirst, *label;
	int defaultSecondHeight;
	int result = 1;

	if (uiInit(&options) != NULL)
		return 1;
	window = uiNewWindow("minimum-size form test", 320, 180, 0);
	form = uiNewForm();
	uiWindowSetChild(window, uiControl(form));
	first = uiNewMultilineEntry();
	second = uiNewMultilineEntry();
	rawFirst = GTK_WIDGET(uiControlHandle(uiControl(first)));
	uiControlShow(uiControl(first));
	uiControlSetMinimumSize(uiControl(first), -1, 40);
	if (!uiControlVisible(uiControl(first))) {
		fprintf(stderr, "parentless visible control became hidden by its wrapper\n");
		uiControlDestroy(uiControl(first));
		uiControlDestroy(uiControl(second));
		goto cleanupWindow;
	}
	uiFormAppend(form, "First", uiControl(first), 1);
	uiFormAppend(form, "Second", uiControl(second), 1);
	label = gtk_grid_get_child_at(GTK_GRID(uiControlHandle(uiControl(form))), 0, 0);
	if (!GTK_IS_SCROLLED_WINDOW(rawFirst) || label == NULL ||
		gtk_widget_get_valign(label) != GTK_ALIGN_START) {
		fprintf(stderr, "scrolled control's form label is not top-aligned\n");
		goto cleanupWindow;
	}
	uiControlShow(uiControl(window));
	uiMainSteps();
	uiMainStep(0);
	defaultSecondHeight = minimumHeight(GTK_WIDGET(uiControlHandle(uiControl(second))));
	uiControlSetMinimumSize(uiControl(first), -1, 400);
	if (minimumHeight(GTK_WIDGET(uiControlHandle(uiControl(second)))) < 400) {
		fprintf(stderr, "form size group did not follow the wrapper\n");
		goto cleanupWindow;
	}
	uiControlSetMinimumSize(uiControl(first), -1, -1);
	if (minimumHeight(GTK_WIDGET(uiControlHandle(uiControl(second)))) != defaultSecondHeight) {
		fprintf(stderr, "form size group did not follow the reset\n");
		goto cleanupWindow;
	}
	uiControlSetMinimumSize(uiControl(first), -1, 400);
	uiFormDelete(form, 0);
	uiControlDestroy(uiControl(first));
	if (minimumHeight(GTK_WIDGET(uiControlHandle(uiControl(second)))) != defaultSecondHeight) {
		fprintf(stderr, "deleted form child remained in the size group\n");
		goto cleanupWindow;
	}
	result = 0;

cleanupWindow:
	uiControlDestroy(uiControl(window));
	uiUninit();
	return result;
}

static int testExternalControl(void)
{
	uiInitOptions options = {0};
	uiWindow *window;
	uiBox *box;
	externalControl *custom;

	if (uiInit(&options) != NULL)
		return 1;
	window = uiNewWindow("external control test", 240, 80, 0);
	box = uiNewHorizontalBox();
	uiUnixNewControl(externalControl, custom);
	custom->widget = gtk_button_new_with_label("custom");
	uiControlShow(uiControl(custom));
	uiControlSetMinimumSize(uiControl(custom), 40, -1);
	uiBoxAppend(box, uiControl(custom), 0);
	uiWindowSetChild(window, uiControl(box));
	uiControlShow(uiControl(window));
	uiControlHide(uiControl(custom));
	uiControlShow(uiControl(custom));
	uiControlSetMinimumSize(uiControl(custom), -1, -1);
	uiControlDestroy(uiControl(window));
	uiUninit();
	return 0;
}

int main(void)
{
	if (testBox() != 0)
		return 1;
	if (testForm() != 0)
		return 1;
	return testExternalControl();
}
