// 11 june 2015
#include "uipriv_unix.h"

struct uiLabel {
	uiUnixControl c;
	GtkWidget *widget;
	GtkLabel *label;
};

uiUnixControlAllDefaults(uiLabel)

char *uiLabelText(uiLabel *l)
{
	return uiUnixStrdupText(gtk_label_get_text(l->label));
}

void uiLabelSetText(uiLabel *l, const char *text)
{
	gtk_label_set_text(l->label, text);
}

uiLabel *uiNewLabel(const char *text)
{
	uiLabel *l;

	uiUnixNewControl(uiLabel, l);

	l->widget = gtk_label_new(text);
	l->label = GTK_LABEL(l->widget);

	gtk_label_set_xalign(l->label, 0);
	gtk_label_set_yalign(l->label, 0);

	return l;
}
