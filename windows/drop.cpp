#include "uipriv_windows.hpp"
#include <shlobj.h>
#include <new>
#include "drop.hpp"

static FORMATETC format(CLIPFORMAT type)
{
	FORMATETC f = {type, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	return f;
}

static unsigned int offeredFormats(IDataObject *object)
{
	unsigned int offered = 0;
	FORMATETC f = format(CF_HDROP);
	if (object->QueryGetData(&f) == S_OK) offered |= uiDropFormatFiles;
	f = format(CF_UNICODETEXT);
	if (object->QueryGetData(&f) == S_OK) offered |= uiDropFormatText;
	return offered;
}

static char *utf8(const WCHAR *s, size_t length)
{
	int bytes;
	char *out;
	if (length > uiprivDropMaxBytes / sizeof(WCHAR)) return NULL;
	if (length == 0) {
		out = (char *) uiprivAlloc(1, "drop UTF-8");
		out[0] = 0;
		return out;
	}
	bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, s, (int) length, NULL, 0, NULL, NULL);
	if (bytes <= 0 || (size_t) bytes >= uiprivDropMaxBytes) return NULL;
	out = (char *) uiprivAlloc((size_t) bytes + 1, "drop UTF-8");
	if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, s, (int) length, out, bytes, NULL, NULL) != bytes) {
		uiprivFree(out);
		return NULL;
	}
	out[bytes] = 0;
	return out;
}

// Both guards refer to the HGLOBAL handle, never the GlobalLock data pointer.
struct Medium {
	STGMEDIUM value;
	Medium() : value{} {}
	~Medium() { if (value.tymed != TYMED_NULL) ReleaseStgMedium(&value); }
};
struct Locked {
	HGLOBAL handle;
	const void *data;
	explicit Locked(HGLOBAL h) : handle(h), data(GlobalLock(h)) {}
	~Locked() { if (data != NULL) GlobalUnlock(handle); }
};

static char *utf8(const char *s, size_t length)
{
	int units = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, s, (int) length, NULL, 0);
	if (units <= 0) return NULL;
	WCHAR *wide = (WCHAR *) uiprivAlloc((size_t) units * sizeof(WCHAR), "drop ANSI conversion");
	char *out = NULL;
	if (MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, s, (int) length, wide, units) == units)
		out = utf8(wide, (size_t) units);
	uiprivFree(wide);
	return out;
}

template<typename T>
static int readPaths(const T *paths, size_t length, uiDropData *payload)
{
	size_t start = 0;
	while (start < length && paths[start] != 0) {
		size_t end = start;
		while (end < length && paths[end] != 0) end++;
		if (end == length) return 0;
		char *path = utf8(paths + start, end - start);
		if (path == NULL) return 0;
		int valid = uiprivDropAddFile(payload, path, strlen(path));
		uiprivFree(path);
		if (!valid) return 0;
		start = end + 1;
	}
	// Every name needs a terminator, followed by one extra list terminator.
	return start < length && payload->count != 0;
}

static int readFiles(const void *data, size_t size, uiDropData *payload)
{
	if (size < sizeof(DROPFILES)) return 0;
	const DROPFILES *header = (const DROPFILES *) data;
	size_t offset = header->pFiles;
	if (offset < sizeof(DROPFILES) || offset >= size) return 0;
	const BYTE *paths = (const BYTE *) data + offset;
	// Walk the bounded list once. Indexed DragQueryFile calls would repeatedly
	// scan the same prefixes for large lists. Both encodings share validation.
	if (header->fWide) {
		if (offset % sizeof(WCHAR) != 0) return 0;
		return readPaths((const WCHAR *) paths, (size - offset) / sizeof(WCHAR), payload);
	}
	return readPaths((const char *) paths, size - offset, payload);
}

static int readPayload(IDataObject *object, uiDropFormat type, uiDropData *payload)
{
	FORMATETC f = format(type == uiDropFormatFiles ? CF_HDROP : CF_UNICODETEXT);
	Medium medium;
	// A failed GetData does not transfer ownership of a medium.
	STGMEDIUM received = {};
	if (FAILED(object->GetData(&f, &received))) return 0;
	medium.value = received;
	if (received.tymed != TYMED_HGLOBAL || received.hGlobal == NULL) return 0;
	size_t size = GlobalSize(received.hGlobal);
	if (size == 0 || size > uiprivDropMaxBytes) return 0;
	Locked lock(received.hGlobal);
	if (lock.data == NULL) return 0;
	if (type == uiDropFormatFiles)
		return readFiles(lock.data, size, payload);
	const WCHAR *text = (const WCHAR *) lock.data;
	size_t length = 0;
	while (length < size / sizeof(WCHAR) && text[length] != 0) length++;
	if (length == size / sizeof(WCHAR)) return 0;
	char *converted = utf8(text, length);
	if (converted == NULL) return 0;
	int valid = uiprivDropSetText(payload, converted, strlen(converted));
	uiprivFree(converted);
	return valid;
}

class DropTarget final : public IDropTarget {
public:
	ULONG refs = 1;
	uiprivDropEndpoint *endpoint;
	HWND hwnd;
	uiprivDropPosition position;
	unsigned int offered = 0;
	uint64_t generation = 0;
	bool registered = false;
	bool hovering = false;
	bool accepted = false;
	DropTarget(uiprivDropEndpoint *e, HWND h, uiprivDropPosition p) : endpoint(e), hwnd(h), position(p) {}

	struct Scope {
		DropTarget *target;
		uiprivDropEndpoint *e;
		explicit Scope(DropTarget *t) : target(t), e(t->endpoint) {
			t->AddRef();
			uiprivDropRetain(e);
			uiprivUserCallbackEnter(NULL);
		}
		~Scope() {
			uiprivDropRelease(e);
			target->Release();
			uiprivUserCallbackLeave();
		}
	};
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **out) override {
		if (out == NULL) return E_POINTER;
		*out = NULL;
		if (iid != IID_IUnknown && iid != IID_IDropTarget) return E_NOINTERFACE;
		*out = static_cast<IDropTarget *>(this);
		AddRef();
		return S_OK;
	}
	ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
	ULONG STDMETHODCALLTYPE Release() override {
		ULONG remaining = --refs;
		if (remaining == 0) delete this;
		return remaining;
	}
	DWORD over(POINTL p, DWORD effects) {
		double x, y;
		accepted = false;
		if (!uiprivDropActive(endpoint, generation) || !position(endpoint->owner, p, &x, &y))
			return DROPEFFECT_NONE;
		uiDropInfo info = uiprivDropMakeInfo(endpoint, offered, x, y);
		accepted = uiprivDropOver(endpoint, &info, (effects & DROPEFFECT_COPY) != 0) == uiDropActionCopy;
		return accepted ? DROPEFFECT_COPY : DROPEFFECT_NONE;
	}
	HRESULT STDMETHODCALLTYPE DragEnter(IDataObject *object, DWORD keys, POINTL p, DWORD *effect) override {
		if (endpoint == NULL) { *effect = DROPEFFECT_NONE; return S_OK; }
		Scope scope(this);
		generation = endpoint->generation;
		hovering = true;
		offered = offeredFormats(object);
		*effect = over(p, *effect);
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE DragOver(DWORD keys, POINTL p, DWORD *effect) override {
		if (endpoint == NULL) { *effect = DROPEFFECT_NONE; return S_OK; }
		Scope scope(this);
		*effect = over(p, *effect);
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE DragLeave() override {
		if (endpoint == NULL) return S_OK;
		Scope scope(this);
		bool notify = hovering;
		hovering = accepted = false;
		if (notify) uiprivDropLeave(endpoint, generation);
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE Drop(IDataObject *object, DWORD keys, POINTL p, DWORD *effect) override {
		if (endpoint == NULL) { *effect = DROPEFFECT_NONE; return S_OK; }
		Scope scope(this);
		DWORD effects = *effect;
		*effect = DROPEFFECT_NONE;
		double x, y;
		uint64_t transferGeneration = generation;
		bool deliver = accepted && (effects & DROPEFFECT_COPY) != 0 &&
			uiprivDropActive(endpoint, transferGeneration) && position(endpoint->owner, p, &x, &y);
		hovering = accepted = false;
		if (!deliver) return S_OK;
		uiDropInfo info = uiprivDropMakeInfo(endpoint, offered, x, y);
		uiprivDropLeave(endpoint, transferGeneration);
		if (!uiprivDropActive(endpoint, transferGeneration)) return S_OK;
		uiDropData payload = {};
		// GetData executes source/provider code: the scope protects the full
		// delivery, and the generation is checked again by Deliver afterwards.
		if (readPayload(object, info.format, &payload) &&
			uiprivDropDeliver(endpoint, transferGeneration, &info, &payload) == uiDropAccepted)
			*effect = DROPEFFECT_COPY;
		uiprivDropDataClear(&payload);
		return S_OK;
	}
};

static uiDropStatus enable(uiprivDropEndpoint *e, int active)
{
	DropTarget *d = (DropTarget *) e->native;
	if (active && !d->registered) {
		if (RegisterDragDrop(d->hwnd, d) != S_OK) return uiDropStatusNativeError;
		d->registered = true;
	} else if (!active && d->registered) {
		if (RevokeDragDrop(d->hwnd) != S_OK) return uiDropStatusNativeError;
		d->registered = false;
	}
	return uiDropStatusOK;
}

static void cancel(uiprivDropEndpoint *e)
{
	DropTarget *d = (DropTarget *) e->native;
	d->hovering = d->accepted = false;
	d->offered = 0;
}

static void dispose(uiprivDropEndpoint *e)
{
	DropTarget *d = (DropTarget *) e->native;
	d->endpoint = NULL;
	d->Release();
}

uiprivDropEndpoint *uiprivWindowsDropNew(uiControl *c, HWND hwnd, uiprivDropPosition position)
{
	uiprivDropEndpoint *e = uiprivDropNew(c);
	DropTarget *d = new (std::nothrow) DropTarget(e, hwnd, position);
	if (d == NULL) {
		uiprivDropRelease(e);
		return NULL;
	}
	e->native = d;
	e->enable = enable;
	e->cancel = cancel;
	e->dispose = dispose;
	return e;
}
