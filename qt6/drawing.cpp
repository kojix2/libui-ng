#include "uipriv_qt.hpp"

#include <QLinearGradient>
#include <QRadialGradient>
#include <cmath>

#include "../common/controlsigs.h"

struct uiDrawContext { QPainter *p; };
struct uiDrawPath { QPainterPath path; uiDrawFillMode fill; bool ended; };

static QColor color(double r,double g,double b,double a){return QColor::fromRgbF(r,g,b,a);}
static QBrush brush(const uiDrawBrush*b){if(b->Type==uiDrawBrushTypeLinearGradient){QLinearGradient g(b->X0,b->Y0,b->X1,b->Y1);for(size_t i=0;i<b->NumStops;i++)g.setColorAt(b->Stops[i].Pos,color(b->Stops[i].R,b->Stops[i].G,b->Stops[i].B,b->Stops[i].A));return QBrush(g);}if(b->Type==uiDrawBrushTypeRadialGradient){QRadialGradient g(b->X1,b->Y1,b->OuterRadius,b->X0,b->Y0);for(size_t i=0;i<b->NumStops;i++)g.setColorAt(b->Stops[i].Pos,color(b->Stops[i].R,b->Stops[i].G,b->Stops[i].B,b->Stops[i].A));return QBrush(g);}return QBrush(color(b->R,b->G,b->B,b->A));}
uiDrawPath *uiDrawNewPath(uiDrawFillMode f){auto*p=new uiDrawPath;p->fill=f;p->ended=false;p->path.setFillRule(f==uiDrawFillModeAlternate?Qt::OddEvenFill:Qt::WindingFill);return p;}
void uiDrawFreePath(uiDrawPath*p){delete p;}
void uiDrawPathNewFigure(uiDrawPath*p,double x,double y){p->path.moveTo(x,y);}
static void arc(uiDrawPath*p,double x,double y,double r,double start,double sweep,int negative,bool move){QRectF rect(x-r,y-r,2*r,2*r);double sd=-start*180.0/uiPi;double sp=-sweep*180.0/uiPi;if(negative)sp=-sp;if(move){QPointF pt(x+r*std::cos(start),y+r*std::sin(start));p->path.moveTo(pt);}p->path.arcTo(rect,sd,sp);}
void uiDrawPathNewFigureWithArc(uiDrawPath*p,double x,double y,double r,double start,double sweep,int neg){arc(p,x,y,r,start,sweep,neg,true);}
void uiDrawPathLineTo(uiDrawPath*p,double x,double y){p->path.lineTo(x,y);}
void uiDrawPathArcTo(uiDrawPath*p,double x,double y,double r,double start,double sweep,int neg){arc(p,x,y,r,start,sweep,neg,false);}
void uiDrawPathBezierTo(uiDrawPath*p,double a,double b,double c,double d,double x,double y){p->path.cubicTo(a,b,c,d,x,y);}
void uiDrawPathCloseFigure(uiDrawPath*p){p->path.closeSubpath();}
void uiDrawPathAddRectangle(uiDrawPath*p,double x,double y,double w,double h){p->path.addRect(x,y,w,h);}
int uiDrawPathEnded(uiDrawPath*p){return p->ended;}
void uiDrawPathEnd(uiDrawPath*p){p->ended=true;}
void uiDrawFill(uiDrawContext*c,uiDrawPath*p,uiDrawBrush*b){c->p->fillPath(p->path,brush(b));}
void uiDrawStroke(uiDrawContext*c,uiDrawPath*p,uiDrawBrush*b,uiDrawStrokeParams*s){QPen pen(brush(b),s->Thickness);pen.setCapStyle(s->Cap==uiDrawLineCapRound?Qt::RoundCap:s->Cap==uiDrawLineCapSquare?Qt::SquareCap:Qt::FlatCap);pen.setJoinStyle(s->Join==uiDrawLineJoinRound?Qt::RoundJoin:s->Join==uiDrawLineJoinBevel?Qt::BevelJoin:Qt::MiterJoin);pen.setMiterLimit(s->MiterLimit);if(s->NumDashes){QList<qreal>d;for(size_t i=0;i<s->NumDashes;i++)d<<s->Dashes[i]/s->Thickness;pen.setDashPattern(d);pen.setDashOffset(s->DashPhase/s->Thickness);}c->p->strokePath(p->path,pen);}
void uiDrawTransform(uiDrawContext*c,uiDrawMatrix*m){c->p->setWorldTransform(QTransform(m->M11,m->M12,m->M21,m->M22,m->M31,m->M32),true);}
void uiDrawClip(uiDrawContext*c,uiDrawPath*p){c->p->setClipPath(p->path,Qt::IntersectClip);}
void uiDrawSave(uiDrawContext*c){c->p->save();}
void uiDrawRestore(uiDrawContext*c){c->p->restore();}

struct uiArea;
class AreaCanvas final:public QWidget{public:uiArea*owner=nullptr;protected:void paintEvent(QPaintEvent*)override;void mousePressEvent(QMouseEvent*)override;void mouseReleaseEvent(QMouseEvent*)override;void mouseMoveEvent(QMouseEvent*)override;void enterEvent(QEnterEvent*)override;void leaveEvent(QEvent*)override;void keyPressEvent(QKeyEvent*)override;void keyReleaseEvent(QKeyEvent*)override;};
struct uiArea {uiQtControl c;AreaCanvas*canvas;QScrollArea*scroll;uiAreaHandler*h;bool scrolling;};
void AreaCanvas::paintEvent(QPaintEvent*e){QPainter p(this);p.setRenderHint(QPainter::Antialiasing);uiDrawContext c{&p};QRect r=e->rect();uiAreaDrawParams d{&c,double(width()),double(height()),double(r.x()),double(r.y()),double(r.width()),double(r.height())};owner->h->Draw(owner->h,owner,&d);}
static uiModifiers mods(Qt::KeyboardModifiers m){uiModifiers o=0;if(m&Qt::ControlModifier)o|=uiModifierCtrl;if(m&Qt::AltModifier)o|=uiModifierAlt;if(m&Qt::ShiftModifier)o|=uiModifierShift;if(m&Qt::MetaModifier)o|=uiModifierSuper;return o;}
static int button(Qt::MouseButton b){if(b==Qt::LeftButton)return 1;if(b==Qt::MiddleButton)return 2;if(b==Qt::RightButton)return 3;return 0;}
static void mouse(AreaCanvas*w,QMouseEvent*e,bool down,bool up){uiAreaMouseEvent m{};m.X=e->position().x();m.Y=e->position().y();m.AreaWidth=w->width();m.AreaHeight=w->height();m.Down=down?button(e->button()):0;m.Up=up?button(e->button()):0;m.Count=1;m.Modifiers=mods(e->modifiers());for(int i=1;i<=3;i++){Qt::MouseButton qb=i==1?Qt::LeftButton:i==2?Qt::MiddleButton:Qt::RightButton;if((e->buttons()&qb)&&button(e->button())!=i)m.Held1To64|=uint64_t(1)<<(i-1);}w->owner->h->MouseEvent(w->owner->h,w->owner,&m);}
void AreaCanvas::mousePressEvent(QMouseEvent*e){setFocus();mouse(this,e,true,false);}void AreaCanvas::mouseReleaseEvent(QMouseEvent*e){mouse(this,e,false,true);}void AreaCanvas::mouseMoveEvent(QMouseEvent*e){mouse(this,e,false,false);}void AreaCanvas::enterEvent(QEnterEvent*){owner->h->MouseCrossed(owner->h,owner,0);}void AreaCanvas::leaveEvent(QEvent*){owner->h->MouseCrossed(owner->h,owner,1);}
static void key(AreaCanvas*w,QKeyEvent*e,bool up){uiAreaKeyEvent k{};QString t=e->text();if(t.size()==1&&t[0].unicode()<128)k.Key=char(t[0].unicode());k.Modifier=mods(e->modifiers());k.Modifiers=mods(e->modifiers());k.Up=up;w->owner->h->KeyEvent(w->owner->h,w->owner,&k);}
void AreaCanvas::keyPressEvent(QKeyEvent*e){key(this,e,false);}void AreaCanvas::keyReleaseEvent(QKeyEvent*e){key(this,e,true);}
static uiArea*newArea(uiAreaHandler*h,bool scrolling,int width,int height){auto*a=reinterpret_cast<uiArea*>(uiQtAllocControl(sizeof(uiArea),uiAreaSignature,"uiArea"));a->h=h;a->scrolling=scrolling;a->canvas=new AreaCanvas;a->canvas->owner=a;a->canvas->setFocusPolicy(Qt::StrongFocus);a->canvas->setMouseTracking(true);QWidget*outer=a->canvas;if(scrolling){a->scroll=new QScrollArea;a->scroll->setWidget(a->canvas);a->scroll->setWidgetResizable(false);a->canvas->resize(width,height);outer=a->scroll;}uiprivQtInitControl(&a->c,outer,true,uiAreaSignature,"uiArea");return a;}
uiArea*uiNewArea(uiAreaHandler*h){return newArea(h,false,0,0);}uiArea*uiNewScrollingArea(uiAreaHandler*h,int w,int hgt){return newArea(h,true,w,hgt);}
void uiAreaSetSize(uiArea*a,int w,int h){a->canvas->resize(w,h);}
void uiAreaQueueRedrawAll(uiArea*a){a->canvas->update();}
void uiAreaScrollTo(uiArea*a,double x,double y,double w,double h){if(a->scroll)a->scroll->ensureVisible(int(x+w/2),int(y+h/2),int(w/2),int(h/2));}
void uiAreaBeginUserWindowMove(uiArea*a){if(auto*w=a->canvas->window()->windowHandle())w->startSystemMove();}
void uiAreaBeginUserWindowResize(uiArea*a,uiWindowResizeEdge e){static const Qt::Edge one[]={Qt::LeftEdge,Qt::TopEdge,Qt::RightEdge,Qt::BottomEdge};Qt::Edges edges;if(e<4)edges=one[e];else if(e==uiWindowResizeEdgeTopLeft)edges=Qt::TopEdge|Qt::LeftEdge;else if(e==uiWindowResizeEdgeTopRight)edges=Qt::TopEdge|Qt::RightEdge;else if(e==uiWindowResizeEdgeBottomLeft)edges=Qt::BottomEdge|Qt::LeftEdge;else edges=Qt::BottomEdge|Qt::RightEdge;if(auto*w=a->canvas->window()->windowHandle())w->startSystemResize(edges);}

struct uiDrawTextLayout { QTextDocument doc; };
static QFont font(const uiFontDescriptor*f){QFont q(qstring(f->Family));q.setPointSizeF(f->Size);q.setWeight(QFont::Weight(std::clamp(int(f->Weight/10),1,1000)));q.setItalic(f->Italic!=uiTextItalicNormal);return q;}
struct TextAttrContext { QTextDocument *doc; QByteArray utf8; };
static uiForEach applyFeature(const uiOpenTypeFeatures*,char a,char b,char c,char d,uint32_t value,void*data){auto*f=static_cast<QFont*>(data);uint32_t tag=(uint32_t(uint8_t(a))<<24)|(uint32_t(uint8_t(b))<<16)|(uint32_t(uint8_t(c))<<8)|uint8_t(d);auto qtTag=QFont::Tag::fromValue(tag);if(qtTag)f->setFeature(*qtTag,value);return uiForEachContinue;}
static uiForEach applyTextAttribute(const uiAttributedString*,const uiAttribute*a,size_t start,size_t end,void*data){auto*c=static_cast<TextAttrContext*>(data);int s=QString::fromUtf8(c->utf8.constData(),qsizetype(start)).size();int e=QString::fromUtf8(c->utf8.constData(),qsizetype(end)).size();QTextCursor cursor(c->doc);cursor.setPosition(s);cursor.setPosition(e,QTextCursor::KeepAnchor);QTextCharFormat f;double r,g,b,alpha;switch(uiAttributeGetType(a)){case uiAttributeTypeFamily:f.setFontFamilies({qstring(uiAttributeFamily(a))});break;case uiAttributeTypeSize:f.setFontPointSize(uiAttributeSize(a));break;case uiAttributeTypeWeight:f.setFontWeight(std::clamp(int(uiAttributeWeight(a)/10),1,1000));break;case uiAttributeTypeItalic:f.setFontItalic(uiAttributeItalic(a)!=uiTextItalicNormal);break;case uiAttributeTypeStretch:{static const int stretches[]={50,62,75,87,100,112,125,150,200};f.setFontStretch(stretches[uiAttributeStretch(a)]);break;}case uiAttributeTypeColor:uiAttributeColor(a,&r,&g,&b,&alpha);f.setForeground(color(r,g,b,alpha));break;case uiAttributeTypeBackground:uiAttributeColor(a,&r,&g,&b,&alpha);f.setBackground(color(r,g,b,alpha));break;case uiAttributeTypeUnderline:{uiUnderline u=uiAttributeUnderline(a);f.setFontUnderline(u!=uiUnderlineNone);if(u==uiUnderlineDouble)f.setUnderlineStyle(QTextCharFormat::DashUnderline);else if(u==uiUnderlineSuggestion)f.setUnderlineStyle(QTextCharFormat::WaveUnderline);break;}case uiAttributeTypeUnderlineColor:{uiUnderlineColor u;uiAttributeUnderlineColor(a,&u,&r,&g,&b,&alpha);if(u==uiUnderlineColorCustom)f.setUnderlineColor(color(r,g,b,alpha));else if(u==uiUnderlineColorGrammar)f.setUnderlineColor(Qt::green);else f.setUnderlineColor(Qt::red);break;}case uiAttributeTypeFeatures:{QFont q=cursor.charFormat().font();uiOpenTypeFeaturesForEach(uiAttributeFeatures(a),applyFeature,&q);f.setFont(q,QTextCharFormat::FontPropertiesSpecifiedOnly);break;}}cursor.mergeCharFormat(f);return uiForEachContinue;}
uiDrawTextLayout*uiDrawNewTextLayout(uiDrawTextLayoutParams*p){auto*t=new uiDrawTextLayout;t->doc.setDocumentMargin(0);t->doc.setDefaultFont(font(p->DefaultFont));QByteArray utf8(uiAttributedStringString(p->String));t->doc.setPlainText(QString::fromUtf8(utf8));TextAttrContext context{&t->doc,utf8};uiAttributedStringForEachAttribute(p->String,applyTextAttribute,&context);if(p->Width>=0)t->doc.setTextWidth(p->Width);QTextOption o=t->doc.defaultTextOption();o.setAlignment(p->Align==uiDrawTextAlignCenter?Qt::AlignHCenter:p->Align==uiDrawTextAlignRight?Qt::AlignRight:Qt::AlignLeft);t->doc.setDefaultTextOption(o);return t;}
void uiDrawFreeTextLayout(uiDrawTextLayout*t){delete t;}
void uiDrawText(uiDrawContext*c,uiDrawTextLayout*t,double x,double y){c->p->save();c->p->translate(x,y);t->doc.drawContents(c->p);c->p->restore();}
void uiDrawTextLayoutExtents(uiDrawTextLayout*t,double*w,double*h){QSizeF s=t->doc.size();*w=s.width();*h=s.height();}
