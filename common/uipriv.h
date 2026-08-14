// 6 april 2015
// note: this file should not include ui.h, as the OS-specific ui_*.h files are included between that one and this one in the OS-specific uipriv_*.h* files
#include <stdarg.h>
#include <string.h>
#include "controlsigs.h"
#include "utf.h"

#ifdef __cplusplus
extern "C" {
#endif

// OS-specific init.* or main.* files
extern uiInitOptions uiprivOptions;

// OS-specific alloc.* files
extern void *uiprivAlloc(size_t, const char *);
#define uiprivNew(T) ((T *) uiprivAlloc(sizeof (T), #T))
extern void *uiprivRealloc(void *, size_t, const char *);
extern void uiprivFree(void *);

// debug.c and OS-specific debug.* files
// MSVC 2013 does not provide C99's __func__; use its
// __FUNCTION__ extension instead.
#ifdef _MSC_VER
#define uiprivMacro__func__ __FUNCTION__
#else
#define uiprivMacro__func__ __func__
#endif
extern void uiprivRealBug(const char *file, const char *line, const char *func, const char *prefix, const char *format, va_list ap);
#define uiprivMacro_ns2(s) #s
#define uiprivMacro_ns(s) uiprivMacro_ns2(s)
extern void uiprivDoImplBug(const char *file, const char *line, const char *func, const char *format, ...);
#define uiprivImplBug(...) uiprivDoImplBug(__FILE__, uiprivMacro_ns(__LINE__), uiprivMacro__func__, __VA_ARGS__)
extern void uiprivDoUserBug(const char *file, const char *line, const char *func, const char *format, ...);
#define uiprivUserBug(...) uiprivDoUserBug(__FILE__, uiprivMacro_ns(__LINE__), uiprivMacro__func__, __VA_ARGS__)

// shouldquit.c
extern int uiprivShouldQuit(void);

// areaevents.c
typedef struct uiprivClickCounter uiprivClickCounter;
// Call uiprivClickCounterReset() to initialize a new instance.
struct uiprivClickCounter {
	int curButton;
	int64_t rectX0;
	int64_t rectY0;
	int64_t rectX1;
	int64_t rectY1;
	uintptr_t prevTime;
	int count;
};
extern int uiprivClickCounterClick(uiprivClickCounter *c, int button, int x, int y, uintptr_t time, uintptr_t maxTime, int32_t xdist, int32_t ydist);
extern void uiprivClickCounterReset(uiprivClickCounter *);
extern int uiprivFromScancode(uintptr_t, uiAreaKeyEvent *);

// matrix.c
extern void uiprivFallbackSkew(uiDrawMatrix *, double, double, double, double);
extern void uiprivFallbackTransformSize(uiDrawMatrix *, double *, double *);

// imageview.c
extern void uiprivImageViewComputeRect(double viewWidth, double viewHeight,
	double imageWidth, double imageHeight, uiImageViewContentMode mode,
	double *x, double *y, double *width, double *height);

// imagerep.c
typedef struct uiprivImageRepMatcher uiprivImageRepMatcher;
struct uiprivImageRepMatcher {
	int targetWidth;
	int targetHeight;
	int bestWidth;
	int bestHeight;
	int64_t bestDistance;
	int bestIsLargeEnough;
	int hasBest;
};
extern void uiprivImageRepMatcherInit(uiprivImageRepMatcher *m,
	int targetWidth, int targetHeight);
extern int uiprivImageRepMatcherAdd(uiprivImageRepMatcher *m,
	int width, int height);

// OS-specific text.* files
extern int uiprivStricmp(const char *a, const char *b);

#ifdef __cplusplus
}
#endif
