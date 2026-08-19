#include "uipriv_qt.hpp"

static void qtDestroy(uiControl *c)
{
	auto *qc = uiQtControl(c);
	uiprivQtDetach(c);
	if (qc->ownsWidget)
		delete qc->widget;
	qc->widget = nullptr;
	uiFreeControl(c);
}

static uintptr_t qtHandle(uiControl *c) { return reinterpret_cast<uintptr_t>(qtWidget(c)); }
static uiControl *qtParent(uiControl *c) { return uiQtControl(c)->parent; }
static void qtSetParent(uiControl *c, uiControl *parent)
{
	uiControlVerifySetParent(c, parent);
	uiQtControl(c)->parent = parent;
}
static int qtToplevel(uiControl *) { return 0; }
static int qtVisible(uiControl *c) { return !qtWidget(c)->isHidden(); }
static void qtShow(uiControl *c) { qtWidget(c)->show(); }
static void qtHide(uiControl *c) { qtWidget(c)->hide(); }
static int qtEnabled(uiControl *c) { return qtWidget(c)->isEnabled(); }
static void qtEnable(uiControl *c) { qtWidget(c)->setEnabled(true); }
static void qtDisable(uiControl *c) { qtWidget(c)->setEnabled(false); }

void uiprivQtInitControl(uiQtControl *c, QWidget *widget, bool owns,
	uint32_t, const char *, bool toplevel)
{
	c->parent = nullptr;
	c->widget = widget;
	c->ownsWidget = owns;
	c->c.Destroy = qtDestroy;
	c->c.Handle = qtHandle;
	c->c.Parent = qtParent;
	c->c.SetParent = qtSetParent;
	c->c.Toplevel = toplevel ? [](uiControl *) -> int { return 1; } : qtToplevel;
	c->c.Visible = qtVisible;
	c->c.Show = qtShow;
	c->c.Hide = qtHide;
	c->c.Enabled = qtEnabled;
	c->c.Enable = qtEnable;
	c->c.Disable = qtDisable;
}

uiQtControl *uiQtAllocControl(size_t n, uint32_t typesig, const char *name)
{
	auto *c = uiQtControl(uiAllocControl(n, uiprivQtControlSignature, typesig, name));
	uiprivQtInitControl(c, nullptr, false, typesig, name);
	return c;
}

QWidget *uiQtControlWidget(uiQtControl *c) { return c->widget; }

void uiQtControlSetWidget(uiQtControl *c, QWidget *widget, int takeOwnership)
{
	c->widget = widget;
	c->ownsWidget = takeOwnership != 0;
}

void uiprivQtDetach(uiControl *c)
{
	QWidget *w = qtWidget(c);
	if (w != nullptr && w->parentWidget() != nullptr)
		w->setParent(nullptr);
	uiQtControl(c)->parent = nullptr;
}
