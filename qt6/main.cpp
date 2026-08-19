#include "uipriv_qt.hpp"

#include <QAbstractEventDispatcher>
#include <QEventLoop>
#include <QMetaObject>
#include <QTextBoundaryFinder>
#include <cstdarg>
#include <cstdio>
#include <strings.h>

#include "../common/attrstr.h"

uiInitOptions uiprivOptions;

static QApplication *app;
static bool ownsApp;
static bool stepsQuit;
static std::vector<QTimer *> timers;
static int syntheticArgc = 1;
static char appName[] = "libui";
static char *syntheticArgv[] = { appName, nullptr };

const char *uiInit(uiInitOptions *o)
{
	if (o == nullptr)
		return qtText(QStringLiteral("uiInitOptions must not be NULL"));
	uiprivOptions = *o;
	app = qobject_cast<QApplication *>(QCoreApplication::instance());
	if (QCoreApplication::instance() != nullptr && app == nullptr)
		return qtText(QStringLiteral("the existing QCoreApplication is not a QApplication"));
	if (app == nullptr) {
		app = new QApplication(syntheticArgc, syntheticArgv);
		ownsApp = true;
	} else {
		ownsApp = false;
	}
	app->setQuitOnLastWindowClosed(false);
	stepsQuit = false;
	return nullptr;
}

void uiUninit(void)
{
	for (QTimer *timer : timers)
		delete timer;
	timers.clear();
	uiprivQtUninitMenus();
	if (ownsApp)
		delete app;
	app = nullptr;
	ownsApp = false;
}

void uiFreeInitError(const char *err) { std::free(const_cast<char *>(err)); }
void uiFreeText(char *text) { std::free(text); }

void uiMain(void)
{
	stepsQuit = false;
	app->exec();
}

void uiMainSteps(void) { stepsQuit = false; }

int uiMainStep(int wait)
{
	if (stepsQuit)
		return 0;
	QEventLoop::ProcessEventsFlags flags = QEventLoop::AllEvents;
	if (wait)
		flags |= QEventLoop::WaitForMoreEvents;
	QCoreApplication::processEvents(flags);
	return stepsQuit ? 0 : 1;
}

void uiQuit(void)
{
	stepsQuit = true;
	if (app != nullptr)
		QMetaObject::invokeMethod(app, [] { QCoreApplication::quit(); }, Qt::QueuedConnection);
}

void uiQueueMain(void (*f)(void *), void *data)
{
	if (f == nullptr)
		uiprivUserBug("uiQueueMain() callback must not be NULL");
	QMetaObject::invokeMethod(app, [f, data] { f(data); }, Qt::QueuedConnection);
}

void uiTimer(int milliseconds, int (*f)(void *), void *data)
{
	if (milliseconds <= 0)
		uiprivUserBug("uiTimer() milliseconds must be > 0");
	if (f == nullptr)
		uiprivUserBug("uiTimer() callback must not be NULL");
	QTimer *timer = new QTimer(app);
	timers.push_back(timer);
	QObject::connect(timer, &QTimer::timeout, [timer, f, data] {
		if (!f(data)) {
			timer->stop();
			timers.erase(std::remove(timers.begin(), timers.end(), timer), timers.end());
			timer->deleteLater();
		}
	});
	timer->start(milliseconds);
}

struct AllocHeader { size_t size; };
void *uiprivAlloc(size_t size, const char *)
{
	auto *p = static_cast<AllocHeader *>(std::calloc(1, sizeof(AllocHeader) + size));
	if (p == nullptr)
		std::abort();
	p->size = size;
	return p + 1;
}

void *uiprivRealloc(void *p, size_t size, const char *)
{
	if (p == nullptr)
		return uiprivAlloc(size, nullptr);
	auto *old = static_cast<AllocHeader *>(p) - 1;
	size_t oldSize = old->size;
	auto *out = static_cast<AllocHeader *>(std::realloc(old, sizeof(AllocHeader) + size));
	if (out == nullptr)
		std::abort();
	out->size = size;
	if (size > oldSize)
		std::memset(reinterpret_cast<unsigned char *>(out + 1) + oldSize, 0, size - oldSize);
	return out + 1;
}

void uiprivFree(void *p) { std::free(static_cast<AllocHeader *>(p) - 1); }

void uiprivRealBug(const char *file, const char *line, const char *func,
	const char *prefix, const char *format, va_list ap)
{
	std::fprintf(stderr, "[libui] %s:%s:%s() %s", file, line, func, prefix);
	std::vfprintf(stderr, format, ap);
	std::fputc('\n', stderr);
	std::abort();
}

int uiprivStricmp(const char *a, const char *b) { return QString::compare(qstring(a), qstring(b), Qt::CaseInsensitive); }

int uiprivGraphemesTakesUTF16(void) { return 1; }

uiprivGraphemes *uiprivNewGraphemes(void *s, size_t len)
{
	auto *g = uiprivNew(uiprivGraphemes);
	QString str = QString::fromUtf16(static_cast<const char16_t *>(s), qsizetype(len));
	std::vector<qsizetype> boundaries{0};
	QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, str);
	finder.toStart();
	for (qsizetype p = finder.toNextBoundary(); p >= 0; p = finder.toNextBoundary())
		boundaries.push_back(p);
	if (boundaries.back() != qsizetype(len))
		boundaries.push_back(qsizetype(len));
	g->len = boundaries.size() - 1;
	g->pointsToGraphemes = static_cast<size_t *>(uiprivAlloc((len + 1) * sizeof(size_t), "graphemes points"));
	g->graphemesToPoints = static_cast<size_t *>(uiprivAlloc((g->len + 1) * sizeof(size_t), "graphemes indices"));
	for (size_t i = 0; i < g->len; i++) {
		g->graphemesToPoints[i] = size_t(boundaries[i]);
		for (qsizetype p = boundaries[i]; p < boundaries[i + 1]; p++)
			g->pointsToGraphemes[p] = i;
	}
	g->graphemesToPoints[g->len] = len;
	g->pointsToGraphemes[len] = g->len;
	return g;
}
