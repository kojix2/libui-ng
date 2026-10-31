#include "test.h"

#define SHED(method, Method) \
	static void method ## Control(uiButton *b, void *data) \
	{ \
		uiControl ## Method(uiControl(data)); \
	}
SHED(show, Show)
SHED(enable, Enable)
SHED(disable, Disable)

uiBox *makeControlState(void)
{
	uiBox *page, *hbox, *nestedBox, *innerhbox, *innerhbox2, *innerhbox3;
	uiTab *disabledTab;
	uiButton *button, *button2;

	page = newVerticalBox();

	nestedBox = newHorizontalBox();
	innerhbox = newHorizontalBox();
	uiBoxAppend(innerhbox, uiControl(uiNewButton("These")), 0);
	button = uiNewButton("buttons");
	uiControlDisable(uiControl(button));
	uiBoxAppend(innerhbox, uiControl(button), 0);
	uiBoxAppend(nestedBox, uiControl(innerhbox), 0);
	innerhbox = newHorizontalBox();
	uiBoxAppend(innerhbox, uiControl(uiNewButton("are")), 0);
	innerhbox2 = newHorizontalBox();
	button = uiNewButton("in");
	uiControlDisable(uiControl(button));
	uiBoxAppend(innerhbox2, uiControl(button), 0);
	uiBoxAppend(innerhbox, uiControl(innerhbox2), 0);
	uiBoxAppend(nestedBox, uiControl(innerhbox), 0);
	innerhbox = newHorizontalBox();
	innerhbox2 = newHorizontalBox();
	uiBoxAppend(innerhbox2, uiControl(uiNewButton("nested")), 0);
	innerhbox3 = newHorizontalBox();
	button = uiNewButton("boxes");
	uiControlDisable(uiControl(button));
	uiBoxAppend(innerhbox3, uiControl(button), 0);
	uiBoxAppend(innerhbox2, uiControl(innerhbox3), 0);
	uiBoxAppend(innerhbox, uiControl(innerhbox2), 0);
	uiBoxAppend(nestedBox, uiControl(innerhbox), 0);
	uiBoxAppend(page, uiControl(nestedBox), 0);

	hbox = newHorizontalBox();
	button = uiNewButton("Enable Nested Box");
	uiButtonOnClicked(button, enableControl, nestedBox);
	uiBoxAppend(hbox, uiControl(button), 0);
	button = uiNewButton("Disable Nested Box");
	uiButtonOnClicked(button, disableControl, nestedBox);
	uiBoxAppend(hbox, uiControl(button), 0);
	uiBoxAppend(page, uiControl(hbox), 0);

	disabledTab = newTab();
	uiTabAppend(disabledTab, "Disabled", uiControl(uiNewButton("Button")));
	uiTabAppend(disabledTab, "Tab", uiControl(uiNewLabel("Label")));
	uiControlDisable(uiControl(disabledTab));
	uiBoxAppend(page, uiControl(disabledTab), 1);

	hbox = newHorizontalBox();
	button = uiNewButton("Show Button 2");
	button2 = uiNewButton("Button 2");
	uiButtonOnClicked(button, showControl, button2);
	uiControlHide(uiControl(button2));
	uiBoxAppend(hbox, uiControl(button), 1);
	uiBoxAppend(hbox, uiControl(button2), 0);
	uiBoxAppend(page, uiControl(hbox), 0);

	return page;
}
