#include <stdlib.h>
#include "../ui.h"
#include "uipriv.h"
#include "drop.h"

struct uiprivDropRegistration {
	uiDropHandler handler;
	void *data;
	unsigned int formats;
	unsigned int refs;
};

static void releaseRegistration(uiprivDropRegistration *r)
{
	if (r == NULL || --r->refs != 0)
		return;
	if (r->handler.ReleaseUserData != NULL) {
		uiprivUserCallbackEnter(NULL);
		r->handler.ReleaseUserData(r->data);
		uiprivUserCallbackLeave();
	}
	uiprivFree(r);
}

uiDropStatus uiprivDropValidate(unsigned int formats, const uiDropHandler *h, void *data)
{
	if (h == NULL)
		return formats == 0 && data == NULL ? uiDropStatusOK : uiDropStatusInvalidArgument;
	if (formats == 0 || (formats & ~(uiDropFormatText | uiDropFormatFiles)) != 0 ||
		h->Size < offsetof(uiDropHandler, Drop) + sizeof h->Drop || h->Drop == NULL)
		return uiDropStatusInvalidArgument;
	return uiDropStatusOK;
}

uiprivDropEndpoint *uiprivDropNew(uiControl *owner)
{
	uiprivDropEndpoint *e = uiprivNew(uiprivDropEndpoint);
	e->owner = owner;
	e->refs = 1;
	e->generation = 1;
	return e;
}

void uiprivDropRetain(uiprivDropEndpoint *e) { e->refs++; }

void uiprivDropRelease(uiprivDropEndpoint *e)
{
	if (--e->refs != 0)
		return;
	if (e->dispose != NULL)
		e->dispose(e);
	uiprivFree(e);
}

uiDropStatus uiprivDropSetHandler(uiprivDropEndpoint *e, unsigned int formats,
	const uiDropHandler *h, void *data)
{
	uiprivDropRegistration *r = NULL, *old;
	uiDropStatus status = uiprivDropValidate(formats, h, data);
	if (status != uiDropStatusOK)
		return status;
	if (e->owner == NULL || uiprivControlDestroyPending(e->owner))
		return uiDropStatusUnavailable;
	if (h != NULL) {
		r = uiprivNew(uiprivDropRegistration);
		r->handler.Size = sizeof r->handler;
		r->handler.Drop = h->Drop;
#define COPY_FIELD(f) if (h->Size >= offsetof(uiDropHandler, f) + sizeof h->f) r->handler.f = h->f
		COPY_FIELD(Over);
		COPY_FIELD(Leave);
		COPY_FIELD(ReleaseUserData);
#undef COPY_FIELD
		r->formats = formats;
		r->data = data;
		r->refs = 1;
	}
	uiprivDropRetain(e);
	uiprivUserCallbackEnter(NULL);
	old = e->current;
	if ((old == NULL) != (r == NULL) && e->enable != NULL) {
		status = e->enable(e, r != NULL);
		if (status != uiDropStatusOK) {
			if (r != NULL)
				uiprivFree(r); // Failed registrations never own userdata.
			goto done;
		}
	}
	e->generation++;
	e->current = r;
	if (e->cancel != NULL)
		e->cancel(e);
	releaseRegistration(old);
done:
	uiprivDropRelease(e);
	uiprivUserCallbackLeave();
	return status;
}

void uiprivDropDestroy(uiprivDropEndpoint *e)
{
	uiprivDropRegistration *old;
	if (e == NULL)
		return;
	uiprivUserCallbackEnter(NULL);
	old = e->current;
	e->current = NULL;
	e->owner = NULL;
	e->generation++;
	if (e->enable != NULL)
		e->enable(e, 0);
	if (e->cancel != NULL)
		e->cancel(e);
	releaseRegistration(old);
	uiprivDropRelease(e);
	uiprivUserCallbackLeave();
}

int uiprivDropActive(uiprivDropEndpoint *e, uint64_t generation)
{
	return e != NULL && e->owner != NULL && e->current != NULL &&
		e->generation == generation && !uiprivControlDestroyPending(e->owner) &&
		uiControlEnabledToUser(e->owner);
}

uiDropInfo uiprivDropMakeInfo(uiprivDropEndpoint *e, unsigned int formats, double x, double y)
{
	uiDropInfo info;
	unsigned int enabled = e->current == NULL ? 0 : formats & e->current->formats;
	info.formats = formats & (uiDropFormatFiles | uiDropFormatText);
	info.format = (enabled & uiDropFormatFiles) ? uiDropFormatFiles :
		(enabled & uiDropFormatText) ? uiDropFormatText : uiDropFormatNone;
	info.x = x;
	info.y = y;
	return info;
}

uiDropAction uiprivDropOver(uiprivDropEndpoint *e, const uiDropInfo *info, int copy)
{
	uiprivDropRegistration *r;
	uiDropAction action = uiDropActionNone;
	uint64_t generation = e->generation;
	if (!copy || info->format == uiDropFormatNone || !uiprivDropActive(e, generation))
		return action;
	uiprivDropRetain(e);
	r = e->current;
	r->refs++;
	uiprivUserCallbackEnter(e->owner);
	action = r->handler.Over == NULL ? uiDropActionCopy : r->handler.Over(info, r->data);
	if (!uiprivDropActive(e, generation) || action != uiDropActionCopy)
		action = uiDropActionNone;
	releaseRegistration(r);
	uiprivDropRelease(e);
	uiprivUserCallbackLeave();
	return action;
}

void uiprivDropLeave(uiprivDropEndpoint *e, uint64_t generation)
{
	uiprivDropRegistration *r;
	if (!uiprivDropActive(e, generation) || e->current->handler.Leave == NULL)
		return;
	uiprivDropRetain(e);
	r = e->current;
	r->refs++;
	uiprivUserCallbackEnter(e->owner);
	r->handler.Leave(r->data);
	releaseRegistration(r);
	uiprivDropRelease(e);
	uiprivUserCallbackLeave();
}

uiDropResult uiprivDropDeliver(uiprivDropEndpoint *e, uint64_t generation,
	const uiDropInfo *info, const uiDropData *payload)
{
	uiprivDropRegistration *r;
	uiDropResult result;
	if (!uiprivDropActive(e, generation) || info->format == uiDropFormatNone ||
		payload->format != info->format || !(e->current->formats & payload->format))
		return uiDropRejected;
	uiprivDropRetain(e);
	r = e->current;
	r->refs++;
	uiprivUserCallbackEnter(e->owner);
	result = r->handler.Drop(info, payload, r->data);
	// Drop may destroy its owner after consuming the payload and still accept.
	releaseRegistration(r);
	uiprivDropRelease(e);
	uiprivUserCallbackLeave();
	return result == uiDropAccepted ? uiDropAccepted : uiDropRejected;
}

static char *copyBytes(const char *s, size_t n)
{
	char *out = (char *) uiprivAlloc(n + 1, "drop string");
	memcpy(out, s, n);
	out[n] = 0;
	return out;
}

int uiprivDropSetText(uiDropData *p, const char *s, size_t n)
{
	size_t i = 0;
	if (s == NULL || n >= uiprivDropMaxBytes || memchr(s, 0, n) != NULL ||
		p->format != uiDropFormatNone)
		return 0;
	while (i < n) {
		uint32_t rune;
		const char *next = uiprivUTF8DecodeRune(s + i, n - i, &rune);
		// DecodeRune consumes one byte for malformed sequences; U+FFFD itself
		// is a valid three-byte sequence and must remain accepted.
		if ((unsigned char) s[i] >= 0x80 && next == s + i + 1)
			return 0;
		i = (size_t) (next - s);
	}
	p->text = copyBytes(s, n);
	p->length = n;
	p->bytes = n + 1;
	p->format = uiDropFormatText;
	return 1;
}

int uiprivDropAddFile(uiDropData *p, const char *s, size_t n)
{
	if (s == NULL || n == 0 || n >= uiprivDropMaxBytes ||
		p->bytes > uiprivDropMaxBytes - n - 1 || p->count >= uiprivDropMaxFiles ||
		memchr(s, 0, n) != NULL ||
		(p->format != uiDropFormatNone && p->format != uiDropFormatFiles))
		return 0;
	// Grow geometrically rather than reallocating for every path.
	if ((p->count & (p->count - 1)) == 0)
		p->files = (char **) uiprivRealloc(p->files,
			(p->count == 0 ? 1 : p->count * 2) * sizeof(char *), "drop files");
	p->files[p->count++] = copyBytes(s, n);
	p->bytes += n + 1;
	p->format = uiDropFormatFiles;
	return 1;
}

void uiprivDropDataClear(uiDropData *p)
{
	size_t i;
	if (p->text != NULL)
		uiprivFree(p->text);
	for (i = 0; i < p->count; i++)
		uiprivFree(p->files[i]);
	if (p->files != NULL)
		uiprivFree(p->files);
	memset(p, 0, sizeof *p);
}

unsigned int uiDropInfoFormats(const uiDropInfo *i) { return i->formats; }
uiDropFormat uiDropInfoFormat(const uiDropInfo *i) { return i->format; }
void uiDropInfoPosition(const uiDropInfo *i, double *x, double *y)
{
	if (x != NULL) *x = i->x;
	if (y != NULL) *y = i->y;
}
uiDropFormat uiDropDataFormat(const uiDropData *p) { return p->format; }
const char *uiDropDataText(const uiDropData *p) { return p->format == uiDropFormatText ? p->text : NULL; }
size_t uiDropDataTextLength(const uiDropData *p) { return p->format == uiDropFormatText ? p->length : 0; }
size_t uiDropDataFileCount(const uiDropData *p) { return p->format == uiDropFormatFiles ? p->count : 0; }
const char *uiDropDataFilePath(const uiDropData *p, size_t i)
{
	return p->format == uiDropFormatFiles && i < p->count ? p->files[i] : NULL;
}

static char *copyPublicText(const char *s, size_t n)
{
	char *out;
#ifdef _WIN32
	out = (char *) uiprivAlloc(n + 1, "drop text copy");
#else
	// uiFreeText() uses free()/g_free() on Darwin/Unix, unlike Windows.
	out = (char *) malloc(n + 1);
	if (out == NULL) abort();
#endif
	memcpy(out, s, n + 1);
	return out;
}

char *uiDropDataCopyText(const uiDropData *p)
{
	return p->format == uiDropFormatText ? copyPublicText(p->text, p->length) : NULL;
}

char **uiDropDataCopyFiles(const uiDropData *p)
{
	char **out;
	size_t i;
	if (p->format != uiDropFormatFiles)
		return NULL;
	out = uiprivNewTextArray(p->count);
	if (out != NULL)
		for (i = 0; i < p->count; i++)
			out[i] = copyPublicText(p->files[i], strlen(p->files[i]));
	return out;
}
