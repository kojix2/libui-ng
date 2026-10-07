// 6 december 2015
#include <float.h>
#include "uipriv_unix.h"

struct uiMultilineEntry {
	uiUnixControl c;
	GtkWidget *widget;
	GtkContainer *scontainer;
	GtkScrolledWindow *sw;
	GtkWidget *textviewWidget;
	GtkTextView *textview;
	GtkTextBuffer *textbuf;
	GtkCssProvider *fontProvider;
	double fontSize;
	double defaultFontSize;
	void (*onChanged)(uiMultilineEntry *, void *);
	void *onChangedData;
	gulong onChangedSignal;
};

static void uiMultilineEntryDestroy(uiControl *c)
{
	uiMultilineEntry *e = uiMultilineEntry(c);

	g_object_unref(e->widget);
	g_object_unref(e->fontProvider);
	uiFreeControl(c);
}

uiUnixControlAllDefaultsExceptDestroy(uiMultilineEntry)

static void onChanged(GtkTextBuffer *textbuf, gpointer data)
{
	uiMultilineEntry *e = uiMultilineEntry(data);

	if (!uiprivUserCallbackEnter(uiControl(e)))
		return;
	(*(e->onChanged))(e, e->onChangedData);
	uiprivUserCallbackLeave();
}

static void defaultOnChanged(uiMultilineEntry *e, void *data)
{
	// do nothing
}

char *uiMultilineEntryText(uiMultilineEntry *e)
{
	GtkTextIter start, end;

	gtk_text_buffer_get_start_iter(e->textbuf, &start);
	gtk_text_buffer_get_end_iter(e->textbuf, &end);

	return gtk_text_buffer_get_text(e->textbuf, &start, &end, TRUE);
}

void uiMultilineEntrySetText(uiMultilineEntry *e, const char *text)
{
	// we need to inhibit sending of ::changed because this WILL send a ::changed otherwise
	g_signal_handler_block(e->textbuf, e->onChangedSignal);
	gtk_text_buffer_set_text(e->textbuf, text, -1);
	g_signal_handler_unblock(e->textbuf, e->onChangedSignal);
}

void uiMultilineEntryAppend(uiMultilineEntry *e, const char *text)
{
	GtkTextIter end;

	gtk_text_buffer_get_end_iter(e->textbuf, &end);
	// we need to inhibit sending of ::changed because this WILL send a ::changed otherwise
	g_signal_handler_block(e->textbuf, e->onChangedSignal);
	gtk_text_buffer_insert(e->textbuf, &end, text, -1);
	g_signal_handler_unblock(e->textbuf, e->onChangedSignal);
}

void uiMultilineEntryOnChanged(uiMultilineEntry *e, void (*f)(uiMultilineEntry *e, void *data), void *data)
{
	e->onChanged = f;
	e->onChangedData = data;
}

static void setMultilineEntryFontSize(uiMultilineEntry *e, double size)
{
	char number[G_ASCII_DTOSTR_BUF_SIZE];
	char css[128];

	g_ascii_dtostr(number, sizeof number, size);
	g_snprintf(css, sizeof css,
		"textview { font-size: %spt; }", number);
	gtk_css_provider_load_from_data(e->fontProvider, css, -1, NULL);
	e->fontSize = size;
	gtk_widget_queue_resize(e->textviewWidget);
}

double uiMultilineEntryFontSize(uiMultilineEntry *e)
{
	return e->fontSize;
}

void uiMultilineEntrySetFontSize(uiMultilineEntry *e, double size)
{
	if (!(size > 0) || size > DBL_MAX ||
		size > ((double) G_MAXINT / PANGO_SCALE)) {
		uiprivUserBug(
			"uiMultilineEntrySetFontSize() size must be finite, positive, "
			"and representable by Pango.");
		return;
	}

	setMultilineEntryFontSize(e, size);
}

void uiMultilineEntryResetFontSize(uiMultilineEntry *e)
{
	setMultilineEntryFontSize(e, e->defaultFontSize);
}

int uiMultilineEntryReadOnly(uiMultilineEntry *e)
{
	return gtk_text_view_get_editable(e->textview) == FALSE;
}

void uiMultilineEntrySetReadOnly(uiMultilineEntry *e, int readonly)
{
	gboolean editable;

	editable = TRUE;
	if (readonly)
		editable = FALSE;
	gtk_text_view_set_editable(e->textview, editable);
}

static double multilineEntryDefaultFontSize(uiMultilineEntry *e)
{
	PangoContext *context;
	const PangoFontDescription *fontdesc;
	double size;

	context = gtk_widget_get_pango_context(e->textviewWidget);
	fontdesc = pango_context_get_font_description(context);
	size = pango_units_to_double(
		pango_font_description_get_size(fontdesc));
	if (pango_font_description_get_size_is_absolute(fontdesc)) {
		double resolution;

		resolution = pango_cairo_context_get_resolution(context);
		if (resolution <= 0)
			resolution = 96;
		size *= 72.0 / resolution;
	}
	return size;
}

static uiMultilineEntry *finishMultilineEntry(GtkPolicyType hpolicy, GtkWrapMode wrapMode)
{
	uiMultilineEntry *e;

	uiUnixNewControl(uiMultilineEntry, e);

	e->widget = gtk_scrolled_window_new(NULL, NULL);
	e->scontainer = GTK_CONTAINER(e->widget);
	e->sw = GTK_SCROLLED_WINDOW(e->widget);
	gtk_scrolled_window_set_policy(e->sw,
		hpolicy,
		GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(e->sw, GTK_SHADOW_IN);

	e->textviewWidget = gtk_text_view_new();
	e->textview = GTK_TEXT_VIEW(e->textviewWidget);
	gtk_text_view_set_wrap_mode(e->textview, wrapMode);
	e->fontSize = multilineEntryDefaultFontSize(e);
	e->defaultFontSize = e->fontSize;
	e->fontProvider = gtk_css_provider_new();
	gtk_style_context_add_provider(
		gtk_widget_get_style_context(e->textviewWidget),
		GTK_STYLE_PROVIDER(e->fontProvider),
		GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	gtk_container_add(e->scontainer, e->textviewWidget);
	// and make the text view visible; only the scrolled window's visibility is controlled by libui
	gtk_widget_show(e->textviewWidget);

	e->textbuf = gtk_text_view_get_buffer(e->textview);

	e->onChangedSignal = g_signal_connect(e->textbuf, "changed", G_CALLBACK(onChanged), e);
	uiMultilineEntryOnChanged(e, defaultOnChanged, NULL);

	return e;
}

uiMultilineEntry *uiNewMultilineEntry(void)
{
	return finishMultilineEntry(GTK_POLICY_NEVER, GTK_WRAP_WORD_CHAR);
}

uiMultilineEntry *uiNewNonWrappingMultilineEntry(void)
{
	return finishMultilineEntry(GTK_POLICY_AUTOMATIC, GTK_WRAP_NONE);
}
