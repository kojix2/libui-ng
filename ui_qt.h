// Public helpers for embedding Qt widgets in a libui Qt-backend build.
#ifndef __LIBUI_UI_QT_H__
#define __LIBUI_UI_QT_H__

#ifndef __cplusplus
#error ui_qt.h is a C++-only header
#endif

#include <QWidget>
#include "ui.h"

struct uiQtControl {
	uiControl c;
	uiControl *parent;
	QWidget *widget;
	bool ownsWidget;
};

#define uiQtControl(this) ((struct uiQtControl *) (this))

extern "C" {
_UI_EXTERN uiQtControl *uiQtAllocControl(size_t n, uint32_t typesig,
	const char *typenamestr);
_UI_EXTERN QWidget *uiQtControlWidget(uiQtControl *c);
_UI_EXTERN void uiQtControlSetWidget(uiQtControl *c, QWidget *widget,
	int takeOwnership);
}

#endif
