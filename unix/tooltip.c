#include "uipriv_unix.h"

void uiControlSetTooltip(uiControl *c, const char *tooltip)
{
	GtkWidget *widget;

	widget = GTK_WIDGET(uiControlHandle(c));
	if (c->TypeSignature == uiSliderSignature) {
		uiSlider *s = uiSlider(c);

		if (tooltip != NULL)
			uiprivSliderSetControlTooltip(s, 1);
		gtk_widget_set_tooltip_text(widget, tooltip);
		if (tooltip == NULL)
			uiprivSliderSetControlTooltip(s, 0);
		return;
	}

	gtk_widget_set_tooltip_text(widget, tooltip);
}
