#include "uipriv_qt.hpp"

#include <QAction>
#include <QApplicationStateChangeEvent>
#include <QDate>
#include <QTime>
#include <ctime>
#include <unordered_map>

#include "../common/controlsigs.h"
#include "../common/table.h"

template<typename T, typename W>
static T *newControl(W *widget, uint32_t sig, const char *name)
{
	auto *c = reinterpret_cast<T *>(uiQtAllocControl(sizeof(T), sig, name));
	uiprivQtInitControl(&c->c, widget, true, sig, name);
	return c;
}

struct uiButton { uiQtControl c; QPushButton *w; void (*clicked)(uiButton *, void *); void *data; };
struct uiCheckbox { uiQtControl c; QCheckBox *w; void (*toggled)(uiCheckbox *, void *); void *data; };
struct uiEntry { uiQtControl c; QLineEdit *w; void (*changed)(uiEntry *, void *); void *data; };
struct uiLabel { uiQtControl c; QLabel *w; };
struct uiSpinbox { uiQtControl c; QSpinBox *w; void (*changed)(uiSpinbox *, void *); void *data; };
struct uiSlider { uiQtControl c; QSlider *w; bool tooltip; void (*changed)(uiSlider *, void *); void *changedData; void (*released)(uiSlider *, void *); void *releasedData; };
struct uiProgressBar { uiQtControl c; QProgressBar *w; int value; };
struct uiSeparator { uiQtControl c; QFrame *w; };
struct uiCombobox { uiQtControl c; QComboBox *w; void (*selected)(uiCombobox *, void *); void *data; };
struct uiEditableCombobox { uiQtControl c; QComboBox *w; void (*changed)(uiEditableCombobox *, void *); void *data; };
struct uiMultilineEntry { uiQtControl c; QPlainTextEdit *w; void (*changed)(uiMultilineEntry *, void *); void *data; };

char *uiButtonText(uiButton *b) { return qtText(b->w->text()); }
void uiButtonSetText(uiButton *b, const char *s) { b->w->setText(qstring(s)); }
void uiButtonOnClicked(uiButton *b, void (*f)(uiButton *, void *), void *d) { b->clicked = f; b->data = d; }
uiButton *uiNewButton(const char *s) {
	auto *b = newControl<uiButton>(new QPushButton(qstring(s)), uiButtonSignature, "uiButton"); b->w = static_cast<QPushButton *>(b->c.widget);
	QObject::connect(b->w, &QPushButton::clicked, [b] { if (b->clicked) b->clicked(b, b->data); }); return b;
}

char *uiCheckboxText(uiCheckbox *c) { return qtText(c->w->text()); }
void uiCheckboxSetText(uiCheckbox *c, const char *s) { c->w->setText(qstring(s)); }
void uiCheckboxOnToggled(uiCheckbox *c, void (*f)(uiCheckbox *, void *), void *d) { c->toggled = f; c->data = d; }
int uiCheckboxChecked(uiCheckbox *c) { return c->w->isChecked(); }
void uiCheckboxSetChecked(uiCheckbox *c, int v) { QSignalBlocker x(c->w); c->w->setChecked(v != 0); }
uiCheckbox *uiNewCheckbox(const char *s) {
	auto *c = newControl<uiCheckbox>(new QCheckBox(qstring(s)), uiCheckboxSignature, "uiCheckbox"); c->w = static_cast<QCheckBox *>(c->c.widget);
	QObject::connect(c->w, &QCheckBox::toggled, [c] { if (c->toggled) c->toggled(c, c->data); }); return c;
}

char *uiEntryText(uiEntry *e) { return qtText(e->w->text()); }
void uiEntrySetText(uiEntry *e, const char *s) { QSignalBlocker x(e->w); e->w->setText(qstring(s)); }
void uiEntryOnChanged(uiEntry *e, void (*f)(uiEntry *, void *), void *d) { e->changed = f; e->data = d; }
int uiEntryReadOnly(uiEntry *e) { return e->w->isReadOnly(); }
void uiEntrySetReadOnly(uiEntry *e, int v) { e->w->setReadOnly(v != 0); }
static uiEntry *newEntry(QLineEdit::EchoMode mode) {
	auto *e = newControl<uiEntry>(new QLineEdit, uiEntrySignature, "uiEntry"); e->w = static_cast<QLineEdit *>(e->c.widget); e->w->setEchoMode(mode);
	QObject::connect(e->w, &QLineEdit::textEdited, [e] { if (e->changed) e->changed(e, e->data); }); return e;
}
uiEntry *uiNewEntry(void) { return newEntry(QLineEdit::Normal); }
uiEntry *uiNewPasswordEntry(void) { return newEntry(QLineEdit::Password); }
uiEntry *uiNewSearchEntry(void) { auto *e = newEntry(QLineEdit::Normal); e->w->setClearButtonEnabled(true); return e; }

char *uiLabelText(uiLabel *l) { return qtText(l->w->text()); }
void uiLabelSetText(uiLabel *l, const char *s) { l->w->setText(qstring(s)); }
uiLabel *uiNewLabel(const char *s) { auto *l = newControl<uiLabel>(new QLabel(qstring(s)), uiLabelSignature, "uiLabel"); l->w = static_cast<QLabel *>(l->c.widget); return l; }

int uiSpinboxValue(uiSpinbox *s) { return s->w->value(); }
void uiSpinboxSetValue(uiSpinbox *s, int v) { QSignalBlocker x(s->w); s->w->setValue(v); }
void uiSpinboxOnChanged(uiSpinbox *s, void (*f)(uiSpinbox *, void *), void *d) { s->changed = f; s->data = d; }
uiSpinbox *uiNewSpinbox(int min, int max) {
	if (min > max)
		std::swap(min, max);
	auto *s = newControl<uiSpinbox>(new QSpinBox, uiSpinboxSignature, "uiSpinbox"); s->w = static_cast<QSpinBox *>(s->c.widget); s->w->setRange(min, max);
	QObject::connect(s->w, &QSpinBox::valueChanged, [s] { if (s->changed) s->changed(s, s->data); }); return s;
}

int uiSliderValue(uiSlider *s) { return s->w->value(); }
void uiSliderSetValue(uiSlider *s, int v) { QSignalBlocker x(s->w); s->w->setValue(v); }
int uiSliderHasToolTip(uiSlider *s) { return s->tooltip; }
void uiSliderSetHasToolTip(uiSlider *s, int v) { s->tooltip = v != 0; }
void uiSliderOnChanged(uiSlider *s, void (*f)(uiSlider *, void *), void *d) { s->changed = f; s->changedData = d; }
void uiSliderOnReleased(uiSlider *s, void (*f)(uiSlider *, void *), void *d) { s->released = f; s->releasedData = d; }
void uiSliderSetRange(uiSlider *s, int min, int max) { if (min > max) std::swap(min, max); QSignalBlocker x(s->w); s->w->setRange(min, max); }
uiSlider *uiNewSlider(int min, int max) {
	if (min > max)
		std::swap(min, max);
	auto *s = newControl<uiSlider>(new QSlider(Qt::Horizontal), uiSliderSignature, "uiSlider"); s->w = static_cast<QSlider *>(s->c.widget); s->tooltip = true; s->w->setRange(min, max);
	QObject::connect(s->w, &QSlider::valueChanged, [s](int v) { if (s->w->isSliderDown() && s->tooltip) QToolTip::showText(QCursor::pos(), QString::number(v), s->w); if (s->changed) s->changed(s, s->changedData); });
	QObject::connect(s->w, &QSlider::sliderReleased, [s] { if (s->released) s->released(s, s->releasedData); }); return s;
}

int uiProgressBarValue(uiProgressBar *p) { return p->value; }
void uiProgressBarSetValue(uiProgressBar *p, int v) { p->value = std::clamp(v, -1, 100); if (p->value < 0) p->w->setRange(0, 0); else { p->w->setRange(0, 100); p->w->setValue(p->value); } }
uiProgressBar *uiNewProgressBar(void) { auto *p = newControl<uiProgressBar>(new QProgressBar, uiProgressBarSignature, "uiProgressBar"); p->w = static_cast<QProgressBar *>(p->c.widget); p->value = 0; p->w->setRange(0, 100); p->w->setTextVisible(false); return p; }

static uiSeparator *newSeparator(QFrame::Shape shape) { auto *s = newControl<uiSeparator>(new QFrame, uiSeparatorSignature, "uiSeparator"); s->w = static_cast<QFrame *>(s->c.widget); s->w->setFrameShape(shape); s->w->setFrameShadow(QFrame::Sunken); return s; }
uiSeparator *uiNewHorizontalSeparator(void) { return newSeparator(QFrame::HLine); }
uiSeparator *uiNewVerticalSeparator(void) { return newSeparator(QFrame::VLine); }

void uiComboboxAppend(uiCombobox *c, const char *s) { QSignalBlocker x(c->w); int selected=c->w->currentIndex(); c->w->addItem(qstring(s)); c->w->setCurrentIndex(selected); }
void uiComboboxInsertAt(uiCombobox *c, int i, const char *s) { QSignalBlocker x(c->w); int selected=c->w->currentIndex(); c->w->insertItem(i, qstring(s)); c->w->setCurrentIndex(selected<0?-1:(selected>=i?selected+1:selected)); }
void uiComboboxDelete(uiCombobox *c, int i) { QSignalBlocker x(c->w); int selected=c->w->currentIndex(); c->w->removeItem(i); c->w->setCurrentIndex(selected==i?-1:(selected>i?selected-1:selected)); }
void uiComboboxClear(uiCombobox *c) { QSignalBlocker x(c->w); c->w->clear(); }
int uiComboboxNumItems(uiCombobox *c) { return c->w->count(); }
int uiComboboxSelected(uiCombobox *c) { return c->w->currentIndex(); }
void uiComboboxSetSelected(uiCombobox *c, int i) { QSignalBlocker x(c->w); c->w->setCurrentIndex(i); }
void uiComboboxOnSelected(uiCombobox *c, void (*f)(uiCombobox *, void *), void *d) { c->selected = f; c->data = d; }
uiCombobox *uiNewCombobox(void) { auto *c = newControl<uiCombobox>(new QComboBox, uiComboboxSignature, "uiCombobox"); c->w = static_cast<QComboBox *>(c->c.widget); c->w->setCurrentIndex(-1); QObject::connect(c->w, &QComboBox::activated, [c] { if (c->selected) c->selected(c, c->data); }); return c; }

void uiEditableComboboxAppend(uiEditableCombobox *c, const char *s) { c->w->addItem(qstring(s)); }
char *uiEditableComboboxText(uiEditableCombobox *c) { return qtText(c->w->currentText()); }
void uiEditableComboboxSetText(uiEditableCombobox *c, const char *s) { QSignalBlocker x(c->w); c->w->setEditText(qstring(s)); }
void uiEditableComboboxOnChanged(uiEditableCombobox *c, void (*f)(uiEditableCombobox *, void *), void *d) { c->changed = f; c->data = d; }
uiEditableCombobox *uiNewEditableCombobox(void) { auto *c = newControl<uiEditableCombobox>(new QComboBox, uiEditableComboboxSignature, "uiEditableCombobox"); c->w = static_cast<QComboBox *>(c->c.widget); c->w->setEditable(true); c->w->setCurrentIndex(-1); QObject::connect(c->w, &QComboBox::editTextChanged, [c] { if (c->changed) c->changed(c, c->data); }); return c; }

char *uiMultilineEntryText(uiMultilineEntry *e) { return qtText(e->w->toPlainText()); }
void uiMultilineEntrySetText(uiMultilineEntry *e, const char *s) { QSignalBlocker x(e->w); e->w->setPlainText(qstring(s)); }
void uiMultilineEntryAppend(uiMultilineEntry *e, const char *s) { QSignalBlocker x(e->w); e->w->moveCursor(QTextCursor::End); e->w->insertPlainText(qstring(s)); }
void uiMultilineEntryOnChanged(uiMultilineEntry *e, void (*f)(uiMultilineEntry *, void *), void *d) { e->changed = f; e->data = d; }
int uiMultilineEntryReadOnly(uiMultilineEntry *e) { return e->w->isReadOnly(); }
void uiMultilineEntrySetReadOnly(uiMultilineEntry *e, int v) { e->w->setReadOnly(v != 0); }
static uiMultilineEntry *newMultiline(bool wrap) { auto *e = newControl<uiMultilineEntry>(new QPlainTextEdit, uiMultilineEntrySignature, "uiMultilineEntry"); e->w = static_cast<QPlainTextEdit *>(e->c.widget); e->w->setLineWrapMode(wrap ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap); QObject::connect(e->w, &QPlainTextEdit::textChanged, [e] { if (e->changed) e->changed(e, e->data); }); return e; }
uiMultilineEntry *uiNewMultilineEntry(void) { return newMultiline(true); }
uiMultilineEntry *uiNewNonWrappingMultilineEntry(void) { return newMultiline(false); }

struct uiBox { uiQtControl c; QBoxLayout *layout; std::vector<uiControl *> children; bool padded; };
static uiBox *newBox(QBoxLayout::Direction direction) { QWidget *w = new QWidget; auto *b = newControl<uiBox>(w, uiBoxSignature, "uiBox"); b->layout = new QBoxLayout(direction, w); b->layout->setContentsMargins(0, 0, 0, 0); b->layout->setSpacing(0); return b; }
uiBox *uiNewHorizontalBox(void) { return newBox(QBoxLayout::LeftToRight); }
uiBox *uiNewVerticalBox(void) { return newBox(QBoxLayout::TopToBottom); }
void uiBoxAppend(uiBox *b, uiControl *c, int stretchy) { b->children.push_back(c); uiControlSetParent(c, uiControl(b)); b->layout->addWidget(qtWidget(c), stretchy ? 1 : 0); }
int uiBoxNumChildren(uiBox *b) { return int(b->children.size()); }
void uiBoxDelete(uiBox *b, int i) { if (i < 0 || i >= int(b->children.size())) return; uiControl *c = b->children[size_t(i)]; b->layout->removeWidget(qtWidget(c)); qtWidget(c)->setParent(nullptr); uiControlSetParent(c, nullptr); b->children.erase(b->children.begin() + i); }
int uiBoxPadded(uiBox *b) { return b->padded; }
void uiBoxSetPadded(uiBox *b, int v) { b->padded = v != 0; b->layout->setSpacing(b->padded ? uiprivQtSpacing : 0); }

struct uiGroup { uiQtControl c; QGroupBox *w; QVBoxLayout *layout; uiControl *child; bool margined; };
char *uiGroupTitle(uiGroup *g) { return qtText(g->w->title()); }
void uiGroupSetTitle(uiGroup *g, const char *s) { g->w->setTitle(qstring(s)); }
void uiGroupSetChild(uiGroup *g, uiControl *c) { if (g->child) { g->layout->removeWidget(qtWidget(g->child)); qtWidget(g->child)->setParent(nullptr); uiControlSetParent(g->child, nullptr); } g->child = c; if (c) { uiControlSetParent(c, uiControl(g)); g->layout->addWidget(qtWidget(c)); } }
int uiGroupMargined(uiGroup *g) { return g->margined; }
void uiGroupSetMargined(uiGroup *g, int v) { g->margined = v != 0; int m = g->margined ? uiprivQtMargin : 0; g->layout->setContentsMargins(m, m, m, m); }
uiGroup *uiNewGroup(const char *s) { auto *g = newControl<uiGroup>(new QGroupBox(qstring(s)), uiGroupSignature, "uiGroup"); g->w = static_cast<QGroupBox *>(g->c.widget); g->layout = new QVBoxLayout(g->w); g->layout->setContentsMargins(0, 0, 0, 0); return g; }

struct TabPage { QWidget *page; QVBoxLayout *layout; uiControl *child; bool margined; };
struct uiTab { uiQtControl c; QTabWidget *w; std::vector<TabPage> pages; void (*selected)(uiTab *, void *); void *data; };
int uiTabSelected(uiTab *t) { return t->w->currentIndex(); }
void uiTabSetSelected(uiTab *t, int i) { if (i < 0 || i >= t->w->count()) return; QSignalBlocker x(t->w); t->w->setCurrentIndex(i); }
void uiTabOnSelected(uiTab *t, void (*f)(uiTab *, void *), void *d) { t->selected = f; t->data = d; }
void uiTabInsertAt(uiTab *t, const char *name, int i, uiControl *c) { QWidget *p = new QWidget; auto *l = new QVBoxLayout(p); l->setContentsMargins(0, 0, 0, 0); l->addWidget(qtWidget(c)); uiControlSetParent(c, uiControl(t)); i = std::clamp(i, 0, int(t->pages.size())); t->pages.insert(t->pages.begin() + i, {p, l, c, false}); t->w->insertTab(i, p, qstring(name)); }
void uiTabAppend(uiTab *t, const char *name, uiControl *c) { uiTabInsertAt(t, name, int(t->pages.size()), c); }
void uiTabDelete(uiTab *t, int i) { if (i < 0 || i >= int(t->pages.size())) return; auto p = t->pages[size_t(i)]; p.layout->removeWidget(qtWidget(p.child)); qtWidget(p.child)->setParent(nullptr); uiControlSetParent(p.child, nullptr); t->w->removeTab(i); delete p.page; t->pages.erase(t->pages.begin() + i); }
int uiTabNumPages(uiTab *t) { return int(t->pages.size()); }
int uiTabMargined(uiTab *t, int i) { return i >= 0 && i < int(t->pages.size()) ? t->pages[size_t(i)].margined : 0; }
void uiTabSetMargined(uiTab *t, int i, int v) { if (i < 0 || i >= int(t->pages.size())) return; auto &p = t->pages[size_t(i)]; p.margined = v != 0; int m = p.margined ? uiprivQtMargin : 0; p.layout->setContentsMargins(m, m, m, m); }
uiTab *uiNewTab(void) { auto *t = newControl<uiTab>(new QTabWidget, uiTabSignature, "uiTab"); t->w = static_cast<QTabWidget *>(t->c.widget); QObject::connect(t->w, &QTabWidget::currentChanged, [t] { if (t->selected) t->selected(t, t->data); }); return t; }

struct uiForm { uiQtControl c; QFormLayout *layout; std::vector<uiControl *> children; std::vector<QLabel *> labels; bool padded; };
void uiFormAppend(uiForm *f, const char *label, uiControl *c, int) { auto *l = new QLabel(qstring(label)); f->labels.push_back(l); f->children.push_back(c); f->layout->addRow(l, qtWidget(c)); uiControlSetParent(c, uiControl(f)); }
int uiFormNumChildren(uiForm *f) { return int(f->children.size()); }
void uiFormDelete(uiForm *f, int i) { if (i < 0 || i >= int(f->children.size())) return; f->layout->removeRow(i); uiControlSetParent(f->children[size_t(i)], nullptr); qtWidget(f->children[size_t(i)])->setParent(nullptr); f->children.erase(f->children.begin()+i); f->labels.erase(f->labels.begin()+i); }
int uiFormPadded(uiForm *f) { return f->padded; }
void uiFormSetPadded(uiForm *f, int v) { f->padded = v != 0; f->layout->setSpacing(f->padded ? uiprivQtSpacing : 0); }
uiForm *uiNewForm(void) { QWidget *w = new QWidget; auto *f = newControl<uiForm>(w, uiFormSignature, "uiForm"); f->layout = new QFormLayout(w); f->layout->setContentsMargins(0,0,0,0); f->layout->setSpacing(0); return f; }

struct GridItem { uiControl *c; int left, top, xspan, yspan; };
struct uiGrid { uiQtControl c; QGridLayout *layout; std::vector<GridItem> items; bool padded; };
static Qt::Alignment alignment(uiAlign h, uiAlign v) { Qt::Alignment a; if (h == uiAlignStart) a |= Qt::AlignLeft; else if (h == uiAlignCenter) a |= Qt::AlignHCenter; else if (h == uiAlignEnd) a |= Qt::AlignRight; if (v == uiAlignStart) a |= Qt::AlignTop; else if (v == uiAlignCenter) a |= Qt::AlignVCenter; else if (v == uiAlignEnd) a |= Qt::AlignBottom; return a; }
void uiGridAppend(uiGrid *g, uiControl *c, int left, int top, int xs, int ys, int he, uiAlign ha, int ve, uiAlign va) { g->items.push_back({c,left,top,xs,ys}); int qleft=left+1024,qtop=top+1024; g->layout->addWidget(qtWidget(c), qtop, qleft, ys, xs, alignment(ha,va)); if (he) g->layout->setColumnStretch(qleft,1); if (ve) g->layout->setRowStretch(qtop,1); uiControlSetParent(c, uiControl(g)); }
void uiGridInsertAt(uiGrid *g, uiControl *c, uiControl *existing, uiAt at, int xs, int ys, int he, uiAlign ha, int ve, uiAlign va) { auto it = std::find_if(g->items.begin(), g->items.end(), [existing](const GridItem &i){return i.c==existing;}); if (it == g->items.end()) return; int l=it->left, t=it->top; if(at==uiAtLeading) l--; else if(at==uiAtTrailing) l+=it->xspan; else if(at==uiAtTop) t--; else t+=it->yspan; uiGridAppend(g,c,l,t,xs,ys,he,ha,ve,va); }
int uiGridPadded(uiGrid *g) { return g->padded; }
void uiGridSetPadded(uiGrid *g, int v) { g->padded=v!=0; g->layout->setSpacing(g->padded ? uiprivQtSpacing : 0); }
uiGrid *uiNewGrid(void) { QWidget *w=new QWidget; auto *g=newControl<uiGrid>(w,uiGridSignature,"uiGrid"); g->layout=new QGridLayout(w); g->layout->setContentsMargins(0,0,0,0); g->layout->setSpacing(0); return g; }

struct uiRadioButtons { uiQtControl c; QWidget *w; QVBoxLayout *layout; QButtonGroup *group; std::vector<QRadioButton *> buttons; void (*selected)(uiRadioButtons *, void *); void *data; };
void uiRadioButtonsAppend(uiRadioButtons *r, const char *s) { auto *b=new QRadioButton(qstring(s)); r->buttons.push_back(b); r->group->addButton(b, int(r->buttons.size()-1)); r->layout->addWidget(b); }
int uiRadioButtonsSelected(uiRadioButtons *r) { return r->group->checkedId(); }
void uiRadioButtonsSetSelected(uiRadioButtons *r, int i) { QSignalBlocker x(r->group); if(i<0) { r->group->setExclusive(false); if(auto *b=r->group->checkedButton()) b->setChecked(false); r->group->setExclusive(true); } else if(i<int(r->buttons.size())) r->buttons[size_t(i)]->setChecked(true); }
void uiRadioButtonsOnSelected(uiRadioButtons *r, void (*f)(uiRadioButtons *, void *), void *d) { r->selected=f; r->data=d; }
uiRadioButtons *uiNewRadioButtons(void) { QWidget *w=new QWidget; auto *r=newControl<uiRadioButtons>(w,uiRadioButtonsSignature,"uiRadioButtons"); r->w=w; r->layout=new QVBoxLayout(w); r->layout->setContentsMargins(0,0,0,0); r->group=new QButtonGroup(w); QObject::connect(r->group,&QButtonGroup::idClicked,[r]{if(r->selected)r->selected(r,r->data);}); return r; }

struct uiDateTimePicker { uiQtControl c; QDateTimeEdit *w; void (*changed)(uiDateTimePicker *, void *); void *data; };
void uiDateTimePickerTime(uiDateTimePicker *d, struct tm *out) { std::memset(out,0,sizeof(*out)); QDateTime x=d->w->dateTime(); out->tm_year=x.date().year()-1900; out->tm_mon=x.date().month()-1; out->tm_mday=x.date().day(); out->tm_hour=x.time().hour(); out->tm_min=x.time().minute(); out->tm_sec=x.time().second(); out->tm_isdst=-1; std::mktime(out); }
void uiDateTimePickerSetTime(uiDateTimePicker *d, const struct tm *in) { QSignalBlocker x(d->w); d->w->setDateTime(QDateTime(QDate(in->tm_year+1900,in->tm_mon+1,in->tm_mday),QTime(in->tm_hour,in->tm_min,in->tm_sec))); }
void uiDateTimePickerOnChanged(uiDateTimePicker *d, void (*f)(uiDateTimePicker *, void *), void *p) { d->changed=f; d->data=p; }
static uiDateTimePicker *newDateTime(const QString &format) { auto *d=newControl<uiDateTimePicker>(new QDateTimeEdit(QDateTime::currentDateTime()),uiDateTimePickerSignature,"uiDateTimePicker"); d->w=static_cast<QDateTimeEdit *>(d->c.widget); d->w->setCalendarPopup(true); d->w->setDisplayFormat(format); QObject::connect(d->w,&QDateTimeEdit::dateTimeChanged,[d]{if(d->changed)d->changed(d,d->data);}); return d; }
uiDateTimePicker *uiNewDateTimePicker(void) { return newDateTime(QStringLiteral("yyyy-MM-dd HH:mm:ss")); }
uiDateTimePicker *uiNewDatePicker(void) { return newDateTime(QStringLiteral("yyyy-MM-dd")); }
uiDateTimePicker *uiNewTimePicker(void) { return newDateTime(QStringLiteral("HH:mm:ss")); }

struct uiFontButton { uiQtControl c; QPushButton *w; QFont font; void (*changed)(uiFontButton *, void *); void *data; };
static void updateFontButton(uiFontButton *b) { b->w->setText(QStringLiteral("%1 %2").arg(b->font.family()).arg(b->font.pointSizeF())); }
void uiFontButtonFont(uiFontButton *b, uiFontDescriptor *d) { d->Family=qtText(b->font.family()); d->Size=b->font.pointSizeF(); d->Weight=uiTextWeight(std::clamp(b->font.weight()*10,0,1000)); d->Italic=b->font.italic()?uiTextItalicItalic:uiTextItalicNormal; d->Stretch=uiTextStretchNormal; }
void uiFontButtonOnChanged(uiFontButton *b, void (*f)(uiFontButton *, void *), void *d) { b->changed=f; b->data=d; }
uiFontButton *uiNewFontButton(void) { auto *b=newControl<uiFontButton>(new QPushButton,uiFontButtonSignature,"uiFontButton"); b->w=static_cast<QPushButton *>(b->c.widget); b->font=QApplication::font(); updateFontButton(b); QObject::connect(b->w,&QPushButton::clicked,[b]{bool ok=false; QFont f=QFontDialog::getFont(&ok,b->font,b->w); if(ok){b->font=f;updateFontButton(b);if(b->changed)b->changed(b,b->data);}}); return b; }
void uiFreeFontButtonFont(uiFontDescriptor *d) { uiFreeText(d->Family); }
void uiLoadControlFont(uiFontDescriptor *d) { QFont f=QApplication::font(); d->Family=qtText(f.family()); d->Size=f.pointSizeF(); d->Weight=uiTextWeight(f.weight()*10); d->Italic=f.italic()?uiTextItalicItalic:uiTextItalicNormal; d->Stretch=uiTextStretchNormal; }
void uiFreeFontDescriptor(uiFontDescriptor *d) { uiFreeText(d->Family); }

struct uiColorButton { uiQtControl c; QPushButton *w; QColor color; void (*changed)(uiColorButton *, void *); void *data; };
static void updateColorButton(uiColorButton *b) { b->w->setText(b->color.name(QColor::HexArgb)); b->w->setStyleSheet(QStringLiteral("background-color:%1").arg(b->color.name(QColor::HexArgb))); }
void uiColorButtonColor(uiColorButton *b,double *r,double *g,double *bl,double *a){*r=b->color.redF();*g=b->color.greenF();*bl=b->color.blueF();*a=b->color.alphaF();}
void uiColorButtonSetColor(uiColorButton *b,double r,double g,double bl,double a){b->color.setRgbF(r,g,bl,a);updateColorButton(b);}
void uiColorButtonOnChanged(uiColorButton *b,void(*f)(uiColorButton*,void*),void*d){b->changed=f;b->data=d;}
uiColorButton *uiNewColorButton(void){auto*b=newControl<uiColorButton>(new QPushButton,uiColorButtonSignature,"uiColorButton");b->w=static_cast<QPushButton*>(b->c.widget);b->color=Qt::black;updateColorButton(b);QObject::connect(b->w,&QPushButton::clicked,[b]{QColor c=QColorDialog::getColor(b->color,b->w,QString(),QColorDialog::ShowAlphaChannel);if(c.isValid()){b->color=c;updateColorButton(b);if(b->changed)b->changed(b,b->data);}});return b;}

struct uiWindow;
class QtWindow final : public QMainWindow {
public:
	uiWindow *owner = nullptr;
protected:
	void closeEvent(QCloseEvent *e) override;
	void moveEvent(QMoveEvent *e) override;
	void resizeEvent(QResizeEvent *e) override;
	void focusInEvent(QFocusEvent *e) override;
	void focusOutEvent(QFocusEvent *e) override;
};
struct uiWindow { uiQtControl c; QtWindow *w; QWidget *host; QVBoxLayout *layout; uiControl *child; bool margined; bool resizeable; bool programMove; bool programResize; int contentWidth; int contentHeight; void (*positionChanged)(uiWindow *,void *); void *positionData; void (*sizeChanged)(uiWindow *,void *); void *sizeData; int (*closing)(uiWindow *,void *); void *closingData; void (*focusChanged)(uiWindow *,void *); void *focusData; };
void QtWindow::closeEvent(QCloseEvent *e){if(owner->closing && !owner->closing(owner,owner->closingData)){e->ignore();return;}e->accept();}
void QtWindow::moveEvent(QMoveEvent *e){QMainWindow::moveEvent(e);if(!owner->programMove&&owner->positionChanged)owner->positionChanged(owner,owner->positionData);owner->programMove=false;}
void QtWindow::resizeEvent(QResizeEvent *e){QMainWindow::resizeEvent(e);if(!owner->programResize){owner->contentWidth=centralWidget()->width();owner->contentHeight=centralWidget()->height();if(owner->sizeChanged)owner->sizeChanged(owner,owner->sizeData);}owner->programResize=false;}
void QtWindow::focusInEvent(QFocusEvent *e){QMainWindow::focusInEvent(e);if(owner->focusChanged)owner->focusChanged(owner,owner->focusData);}
void QtWindow::focusOutEvent(QFocusEvent *e){QMainWindow::focusOutEvent(e);if(owner->focusChanged)owner->focusChanged(owner,owner->focusData);}

struct uiMenuItem { QString name; bool checkable=false; bool separator=false; bool quit=false; bool enabled=true; bool checked=false; void (*clicked)(uiMenuItem *,uiWindow *,void *)=nullptr; void *data=nullptr; std::vector<QPointer<QAction>> actions; };
struct uiMenu { QString name; std::vector<uiMenuItem *> items; };
static std::vector<uiMenu *> menus;
void uiprivQtUninitMenus(void){for(auto*m:menus){for(auto*i:m->items)delete i;delete m;}menus.clear();}
static uiMenuItem *appendMenuItem(uiMenu *m,const char *name,bool check=false,bool quit=false){auto*i=new uiMenuItem;i->name=qstring(name);i->checkable=check;i->quit=quit;m->items.push_back(i);return i;}
uiMenu *uiNewMenu(const char *name){auto*m=new uiMenu;m->name=qstring(name);menus.push_back(m);return m;}
uiMenuItem *uiMenuAppendItem(uiMenu*m,const char*n){return appendMenuItem(m,n);}
uiMenuItem *uiMenuAppendCheckItem(uiMenu*m,const char*n){return appendMenuItem(m,n,true);}
uiMenuItem *uiMenuAppendQuitItem(uiMenu*m){return appendMenuItem(m,"Quit",false,true);}
uiMenuItem *uiMenuAppendPreferencesItem(uiMenu*m){return appendMenuItem(m,"Preferences");}
uiMenuItem *uiMenuAppendAboutItem(uiMenu*m){return appendMenuItem(m,"About");}
void uiMenuAppendSeparator(uiMenu*m){auto*i=appendMenuItem(m,"");i->separator=true;}
void uiMenuItemEnable(uiMenuItem*i){i->enabled=true;for(auto&a:i->actions)if(a)a->setEnabled(true);}
void uiMenuItemDisable(uiMenuItem*i){i->enabled=false;for(auto&a:i->actions)if(a)a->setEnabled(false);}
void uiMenuItemOnClicked(uiMenuItem*i,void(*f)(uiMenuItem*,uiWindow*,void*),void*d){i->clicked=f;i->data=d;}
int uiMenuItemChecked(uiMenuItem*i){return i->checked;}
void uiMenuItemSetChecked(uiMenuItem*i,int v){i->checked=v!=0;for(auto&a:i->actions)if(a){QSignalBlocker x(a);a->setChecked(i->checked);}}
static void buildMenus(uiWindow*w){for(uiMenu*m:menus){QMenu*q=w->w->menuBar()->addMenu(m->name);for(uiMenuItem*i:m->items){if(i->separator){q->addSeparator();continue;}QAction*a=q->addAction(i->name);a->setCheckable(i->checkable);a->setChecked(i->checked);a->setEnabled(i->enabled);i->actions.push_back(a);QObject::connect(a,&QAction::triggered,[i,w](bool checked){if(i->checkable)i->checked=checked;if(i->quit){if(uiprivShouldQuit())uiQuit();return;}if(i->clicked)i->clicked(i,w,i->data);});}}}

char *uiWindowTitle(uiWindow*w){return qtText(w->w->windowTitle());}
void uiWindowSetTitle(uiWindow*w,const char*s){w->w->setWindowTitle(qstring(s));}
void uiWindowPosition(uiWindow*w,int*x,int*y){*x=w->w->x();*y=w->w->y();}
void uiWindowSetPosition(uiWindow*w,int x,int y){w->programMove=true;w->w->move(x,y);}
void uiWindowOnPositionChanged(uiWindow*w,void(*f)(uiWindow*,void*),void*d){w->positionChanged=f;w->positionData=d;}
void uiWindowContentSize(uiWindow*w,int*width,int*height){*width=w->contentWidth;*height=w->contentHeight;}
void uiWindowSetContentSize(uiWindow*w,int width,int height){w->contentWidth=width;w->contentHeight=height;w->programResize=true;QSize extra=w->w->size()-w->w->centralWidget()->size();w->w->resize(width+extra.width(),height+extra.height());}
int uiWindowFullscreen(uiWindow*w){return w->w->isFullScreen();}
void uiWindowSetFullscreen(uiWindow*w,int v){if(v)w->w->showFullScreen();else w->w->showNormal();}
void uiWindowOnContentSizeChanged(uiWindow*w,void(*f)(uiWindow*,void*),void*d){w->sizeChanged=f;w->sizeData=d;}
void uiWindowOnClosing(uiWindow*w,int(*f)(uiWindow*,void*),void*d){w->closing=f;w->closingData=d;}
void uiWindowOnFocusChanged(uiWindow*w,void(*f)(uiWindow*,void*),void*d){w->focusChanged=f;w->focusData=d;}
int uiWindowFocused(uiWindow*w){return w->w->isActiveWindow();}
int uiWindowBorderless(uiWindow*w){return w->w->windowFlags().testFlag(Qt::FramelessWindowHint);}
void uiWindowSetBorderless(uiWindow*w,int v){w->w->setWindowFlag(Qt::FramelessWindowHint,v!=0);}
void uiWindowSetChild(uiWindow*w,uiControl*c){if(w->child){w->layout->removeWidget(qtWidget(w->child));qtWidget(w->child)->setParent(nullptr);uiControlSetParent(w->child,nullptr);}w->child=c;if(c){w->layout->addWidget(qtWidget(c));uiControlSetParent(c,uiControl(w));}}
int uiWindowMargined(uiWindow*w){return w->margined;}
void uiWindowSetMargined(uiWindow*w,int v){w->margined=v!=0;int m=w->margined?uiprivQtMargin:0;w->layout->setContentsMargins(m,m,m,m);}
int uiWindowResizeable(uiWindow*w){return w->resizeable;}
void uiWindowSetResizeable(uiWindow*w,int v){w->resizeable=v!=0;if(w->resizeable){w->w->setMinimumSize(0,0);w->w->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);}else w->w->setFixedSize(w->w->size());}
uiWindow *uiNewWindow(const char*title,int width,int height,int hasMenubar){auto*qw=new QtWindow;auto*w=newControl<uiWindow>(qw,uiWindowSignature,"uiWindow");w->w=qw;qw->owner=w;w->resizeable=true;w->contentWidth=width;w->contentHeight=height;w->host=new QWidget;w->layout=new QVBoxLayout(w->host);w->layout->setContentsMargins(0,0,0,0);qw->setCentralWidget(w->host);qw->setWindowTitle(qstring(title));if(hasMenubar)buildMenus(w);qw->resize(width,height);w->c.c.Toplevel=[](uiControl*)->int{return 1;};w->c.c.SetParent=[](uiControl*,uiControl*){uiUserBugCannotSetParentOnToplevel("uiWindow");};return w;}

static QWidget *parentWidget(uiWindow*w){return w?w->w:nullptr;}
char *uiOpenFile(uiWindow*w){QString s=QFileDialog::getOpenFileName(parentWidget(w));return s.isEmpty()?nullptr:qtText(s);}
char *uiOpenFolder(uiWindow*w){QString s=QFileDialog::getExistingDirectory(parentWidget(w));return s.isEmpty()?nullptr:qtText(s);}
char *uiSaveFile(uiWindow*w){QString s=QFileDialog::getSaveFileName(parentWidget(w));return s.isEmpty()?nullptr:qtText(s);}
void uiMsgBox(uiWindow*w,const char*title,const char*description){QMessageBox::information(parentWidget(w),qstring(title),qstring(description));}
void uiMsgBoxError(uiWindow*w,const char*title,const char*description){QMessageBox::critical(parentWidget(w),qstring(title),qstring(description));}

static QTransform transform(const uiDrawMatrix *m){return QTransform(m->M11,m->M12,m->M21,m->M22,m->M31,m->M32);}
static void multiply(uiDrawMatrix*d,const uiDrawMatrix*s){uiDrawMatrix a=*d;d->M11=s->M11*a.M11+s->M21*a.M12;d->M12=s->M12*a.M11+s->M22*a.M12;d->M21=s->M11*a.M21+s->M21*a.M22;d->M22=s->M12*a.M21+s->M22*a.M22;d->M31=s->M11*a.M31+s->M21*a.M32+s->M31;d->M32=s->M12*a.M31+s->M22*a.M32+s->M32;}
void uiDrawMatrixTranslate(uiDrawMatrix*m,double x,double y){uiDrawMatrix t{1,0,0,1,x,y};multiply(m,&t);}
void uiDrawMatrixScale(uiDrawMatrix*m,double cx,double cy,double x,double y){uiDrawMatrix t{ x,0,0,y,cx-cx*x,cy-cy*y};multiply(m,&t);}
void uiDrawMatrixRotate(uiDrawMatrix*m,double x,double y,double a){double c=std::cos(a),s=std::sin(a);uiDrawMatrix t{c,s,-s,c,-x*c+y*s+x,-x*s-y*c+y};multiply(m,&t);}
void uiDrawMatrixSkew(uiDrawMatrix*m,double x,double y,double xa,double ya){uiprivFallbackSkew(m,x,y,xa,ya);}
void uiDrawMatrixMultiply(uiDrawMatrix*d,uiDrawMatrix*s){multiply(d,s);}
int uiDrawMatrixInvertible(uiDrawMatrix*m){return transform(m).isInvertible();}
int uiDrawMatrixInvert(uiDrawMatrix*m){bool ok;QTransform t=transform(m).inverted(&ok);if(ok){m->M11=t.m11();m->M12=t.m12();m->M21=t.m21();m->M22=t.m22();m->M31=t.dx();m->M32=t.dy();}return ok;}
void uiDrawMatrixTransformPoint(uiDrawMatrix*m,double*x,double*y){QPointF p=transform(m).map(QPointF(*x,*y));*x=p.x();*y=p.y();}
void uiDrawMatrixTransformSize(uiDrawMatrix*m,double*x,double*y){uiprivFallbackTransformSize(m,x,y);}

struct uiImage { double width,height; std::vector<QImage> reps; };
uiImage*uiNewImage(double w,double h){auto*i=new uiImage;i->width=w;i->height=h;return i;}
void uiFreeImage(uiImage*i){delete i;}
void uiImageAppend(uiImage*i,void*pixels,int w,int h,int stride){QImage src(static_cast<uchar*>(pixels),w,h,stride,QImage::Format_RGBA8888_Premultiplied);i->reps.push_back(src.copy());}

class QtTableModel;
struct uiTableModel { uiTableModelHandler *mh; std::vector<QtTableModel*> views; };
enum class TableColumnType {Text,Image,ImageText,Checkbox,CheckboxText,Progress,Button};
struct TableColumn {QString name;TableColumnType type;int a=-1,b=-1,c=-1,d=-1;uiSortIndicator sort=uiSortIndicatorNone;};
struct uiTable {uiQtControl c;QTableView*w;QtTableModel*view;uiTableModel*model;std::vector<TableColumn>columns;uiTableSelectionMode selectionMode=uiTableSelectionModeZeroOrOne;void(*rowClicked)(uiTable*,int,void*)=nullptr;void*rowData=nullptr;void(*rowDouble)(uiTable*,int,void*)=nullptr;void*doubleData=nullptr;void(*headerClicked)(uiTable*,int,void*)=nullptr;void*headerData=nullptr;void(*selectionChanged)(uiTable*,void*)=nullptr;void*selectionData=nullptr;};
class QtTableModel final:public QAbstractTableModel{public:uiTable*t;explicit QtTableModel(uiTable*x):QAbstractTableModel(x->w),t(x){}int rowCount(const QModelIndex&p={})const override{return p.isValid()?0:uiprivTableModelNumRows(t->model);}int columnCount(const QModelIndex&p={})const override{return p.isValid()?0:int(t->columns.size());}QVariant headerData(int s,Qt::Orientation o,int role)const override{if(o==Qt::Horizontal&&role==Qt::DisplayRole&&s<int(t->columns.size()))return t->columns[size_t(s)].name;return{};}QVariant data(const QModelIndex&i,int role)const override{if(!i.isValid())return{};const auto&c=t->columns[size_t(i.column())];int mc=c.a;if(c.type==TableColumnType::ImageText)mc=c.b;if(c.type==TableColumnType::CheckboxText)mc=c.c;uiTableValue*v=uiprivTableModelCellValue(t->model,i.row(),mc);if(!v)return{};QVariant out;if((c.type==TableColumnType::Checkbox||c.type==TableColumnType::CheckboxText)&&role==Qt::CheckStateRole)out=uiTableValueInt(v)?Qt::Checked:Qt::Unchecked;else if(role==Qt::DisplayRole||role==Qt::EditRole){uiTableValueType ty=uiTableValueGetType(v);if(ty==uiTableValueTypeString)out=qstring(uiTableValueString(v));else if(ty==uiTableValueTypeInt)out=uiTableValueInt(v);}uiFreeTableValue(v);return out;}Qt::ItemFlags flags(const QModelIndex&i)const override{auto f=QAbstractTableModel::flags(i);const auto&c=t->columns[size_t(i.column())];int edit=c.b;if(c.type==TableColumnType::Checkbox||c.type==TableColumnType::CheckboxText){f|=Qt::ItemIsUserCheckable;edit=c.b;}if((c.type==TableColumnType::Text||c.type==TableColumnType::ImageText||c.type==TableColumnType::CheckboxText)&&uiprivTableModelCellEditable(t->model,i.row(),edit))f|=Qt::ItemIsEditable;return f;}bool setData(const QModelIndex&i,const QVariant&v,int role)override{const auto&c=t->columns[size_t(i.column())];if(role==Qt::CheckStateRole){uiTableValue*x=uiNewTableValueInt(v.toInt()==Qt::Checked);uiprivTableModelSetCellValue(t->model,i.row(),c.a,x);uiFreeTableValue(x);return true;}if(role==Qt::EditRole){QByteArray b=v.toString().toUtf8();uiTableValue*x=uiNewTableValueString(b.constData());int mc=c.type==TableColumnType::ImageText?c.b:c.type==TableColumnType::CheckboxText?c.c:c.a;uiprivTableModelSetCellValue(t->model,i.row(),mc,x);uiFreeTableValue(x);return true;}return false;}void reset(){beginResetModel();endResetModel();}void inserted(int r){beginInsertRows({},r,r);endInsertRows();}void changed(int r){emit dataChanged(index(r,0),index(r,columnCount()-1));}void removed(int r){beginRemoveRows({},r,r);endRemoveRows();}};
uiTableModel *uiNewTableModel(uiTableModelHandler*mh){auto*m=new uiTableModel;m->mh=mh;return m;}
void uiFreeTableModel(uiTableModel*m){if(!m->views.empty())uiprivUserBug("You cannot free a uiTableModel while uiTables are using it.");delete m;}
uiTableModelHandler *uiprivTableModelHandler(uiTableModel*m){return m->mh;}
void uiTableModelRowInserted(uiTableModel*m,int r){for(auto*v:m->views)v->inserted(r);}
void uiTableModelRowChanged(uiTableModel*m,int r){for(auto*v:m->views)v->changed(r);}
void uiTableModelRowDeleted(uiTableModel*m,int r){for(auto*v:m->views)v->removed(r);}
static void addColumn(uiTable*t,const char*n,TableColumnType ty,int a,int b=-1,int c=-1,int d=-1){t->columns.push_back({qstring(n),ty,a,b,c,d});t->view->reset();}
void uiTableAppendTextColumn(uiTable*t,const char*n,int a,int b,uiTableTextColumnOptionalParams*){addColumn(t,n,TableColumnType::Text,a,b);}
void uiTableAppendImageColumn(uiTable*t,const char*n,int a){addColumn(t,n,TableColumnType::Image,a);}
void uiTableAppendImageTextColumn(uiTable*t,const char*n,int a,int b,int c,uiTableTextColumnOptionalParams*){addColumn(t,n,TableColumnType::ImageText,a,b,c);}
void uiTableAppendCheckboxColumn(uiTable*t,const char*n,int a,int b){addColumn(t,n,TableColumnType::Checkbox,a,b);}
void uiTableAppendCheckboxTextColumn(uiTable*t,const char*n,int a,int b,int c,int d,uiTableTextColumnOptionalParams*){addColumn(t,n,TableColumnType::CheckboxText,a,b,c,d);}
void uiTableAppendProgressBarColumn(uiTable*t,const char*n,int a){addColumn(t,n,TableColumnType::Progress,a);}
void uiTableAppendButtonColumn(uiTable*t,const char*n,int a,int b){addColumn(t,n,TableColumnType::Button,a,b);}
int uiTableHeaderVisible(uiTable*t){return !t->w->horizontalHeader()->isHidden();}void uiTableHeaderSetVisible(uiTable*t,int v){t->w->horizontalHeader()->setVisible(v!=0);}
uiTable*uiNewTable(uiTableParams*p){auto*t=newControl<uiTable>(new QTableView,uiTableSignature,"uiTable");t->w=static_cast<QTableView*>(t->c.widget);t->model=p->Model;t->view=new QtTableModel(t);t->w->setModel(t->view);t->w->setSelectionBehavior(QAbstractItemView::SelectRows);t->model->views.push_back(t->view);t->c.c.Destroy=[](uiControl*c){auto*t=uiTable(c);t->model->views.erase(std::remove(t->model->views.begin(),t->model->views.end(),t->view),t->model->views.end());uiprivQtDetach(c);delete t->w;t->c.widget=nullptr;uiFreeControl(c);};QObject::connect(t->w,&QTableView::clicked,[t](const QModelIndex&i){if(t->rowClicked)t->rowClicked(t,i.row(),t->rowData);});QObject::connect(t->w,&QTableView::doubleClicked,[t](const QModelIndex&i){if(t->rowDouble)t->rowDouble(t,i.row(),t->doubleData);});QObject::connect(t->w->horizontalHeader(),&QHeaderView::sectionClicked,[t](int c){if(t->headerClicked)t->headerClicked(t,c,t->headerData);});QObject::connect(t->w->selectionModel(),&QItemSelectionModel::selectionChanged,[t]{if(t->selectionChanged)t->selectionChanged(t,t->selectionData);});return t;}
void uiTableOnRowClicked(uiTable*t,void(*f)(uiTable*,int,void*),void*d){t->rowClicked=f;t->rowData=d;}void uiTableOnRowDoubleClicked(uiTable*t,void(*f)(uiTable*,int,void*),void*d){t->rowDouble=f;t->doubleData=d;}
void uiTableHeaderSetSortIndicator(uiTable*t,int c,uiSortIndicator i){if(c<0||c>=int(t->columns.size()))return;t->columns[size_t(c)].sort=i;if(i==uiSortIndicatorNone)t->w->setSortingEnabled(false);else{t->w->setSortingEnabled(true);t->w->horizontalHeader()->setSortIndicator(c,i==uiSortIndicatorAscending?Qt::AscendingOrder:Qt::DescendingOrder);}}
uiSortIndicator uiTableHeaderSortIndicator(uiTable*t,int c){return c>=0&&c<int(t->columns.size())?t->columns[size_t(c)].sort:uiSortIndicator(uiSortIndicatorNone);}
void uiTableHeaderOnClicked(uiTable*t,void(*f)(uiTable*,int,void*),void*d){t->headerClicked=f;t->headerData=d;}int uiTableColumnWidth(uiTable*t,int c){return t->w->columnWidth(c);}void uiTableColumnSetWidth(uiTable*t,int c,int w){if(w<0)t->w->resizeColumnToContents(c);else t->w->setColumnWidth(c,w);}
uiTableSelectionMode uiTableGetSelectionMode(uiTable*t){return t->selectionMode;}void uiTableSetSelectionMode(uiTable*t,uiTableSelectionMode m){t->selectionMode=m;t->w->clearSelection();t->w->setSelectionMode(m==uiTableSelectionModeNone?QAbstractItemView::NoSelection:m==uiTableSelectionModeZeroOrMany?QAbstractItemView::ExtendedSelection:m==uiTableSelectionModeOne?QAbstractItemView::SingleSelection:QAbstractItemView::SingleSelection);}
void uiTableOnSelectionChanged(uiTable*t,void(*f)(uiTable*,void*),void*d){t->selectionChanged=f;t->selectionData=d;}
uiTableSelection*uiTableGetSelection(uiTable*t){QModelIndexList rows=t->w->selectionModel()->selectedRows();auto*s=static_cast<uiTableSelection*>(uiprivAlloc(sizeof(uiTableSelection),"uiTableSelection"));s->NumRows=rows.size();s->Rows=s->NumRows?static_cast<int*>(uiprivAlloc(size_t(s->NumRows)*sizeof(int),"selection rows")):nullptr;for(int i=0;i<s->NumRows;i++)s->Rows[i]=rows[i].row();return s;}
void uiTableSetSelection(uiTable*t,uiTableSelection*s){uiprivValidateTableSelection(t->model,s);QSignalBlocker b(t->w->selectionModel());t->w->clearSelection();for(int i=0;i<s->NumRows;i++)t->w->selectionModel()->select(t->view->index(s->Rows[i],0),QItemSelectionModel::Select|QItemSelectionModel::Rows);}
