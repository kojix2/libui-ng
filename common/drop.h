#ifndef UIPRIV_DROP_H
#define UIPRIV_DROP_H

#include "../ui.h"

#ifdef __cplusplus
extern "C" {
#endif

// Limits apply to normalized payload bytes (including terminators).
#define uiprivDropMaxBytes ((size_t) 64 * 1024 * 1024)
#define uiprivDropMaxFiles ((size_t) 65536)

typedef struct uiprivDropRegistration uiprivDropRegistration;
typedef struct uiprivDropEndpoint uiprivDropEndpoint;
struct uiprivDropEndpoint {
	uiControl *owner;
	unsigned int refs;
	uint64_t generation;
	uiprivDropRegistration *current;
	void *native;
	uiDropStatus (*enable)(uiprivDropEndpoint *, int);
	void (*cancel)(uiprivDropEndpoint *);
	void (*dispose)(uiprivDropEndpoint *);
};

struct uiDropInfo {
	unsigned int formats;
	uiDropFormat format;
	double x, y;
};

struct uiDropData {
	uiDropFormat format;
	char *text;
	size_t length;
	char **files;
	size_t count;
	size_t bytes;
};

extern uiDropStatus uiprivDropValidate(unsigned int, const uiDropHandler *, void *);
extern uiprivDropEndpoint *uiprivDropNew(uiControl *);
extern void uiprivDropRetain(uiprivDropEndpoint *);
extern void uiprivDropRelease(uiprivDropEndpoint *);
extern void uiprivDropDestroy(uiprivDropEndpoint *);
extern uiDropStatus uiprivDropSetHandler(uiprivDropEndpoint *, unsigned int,
	const uiDropHandler *, void *);
extern int uiprivDropActive(uiprivDropEndpoint *, uint64_t);
extern uiDropInfo uiprivDropMakeInfo(uiprivDropEndpoint *, unsigned int, double, double);
extern uiDropAction uiprivDropOver(uiprivDropEndpoint *, const uiDropInfo *, int);
extern void uiprivDropLeave(uiprivDropEndpoint *, uint64_t);
extern uiDropResult uiprivDropDeliver(uiprivDropEndpoint *, uint64_t,
	const uiDropInfo *, const uiDropData *);
extern int uiprivDropSetText(uiDropData *, const char *, size_t);
extern int uiprivDropAddFile(uiDropData *, const char *, size_t);
extern void uiprivDropDataClear(uiDropData *);

#ifdef __cplusplus
}
#endif
#endif
