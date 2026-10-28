#include "../../windows/drop.cpp"
extern "C" {
#include "unit.h"
}
#define LIBUI_DROP_TEST_NATIVE_HEADERS
#include "drop_test.h"

template<typename T>
static std::vector<BYTE> fileList(const T *paths, size_t count, bool wide)
{
	std::vector<BYTE> bytes(sizeof(DROPFILES) + count * sizeof(T));
	DROPFILES header = {};
	header.pFiles = sizeof header;
	header.fWide = wide;
	memcpy(bytes.data(), &header, sizeof header);
	memcpy(bytes.data() + sizeof header, paths, count * sizeof(T));
	return bytes;
}

static void widePaths(void **state)
{
	const WCHAR paths[] = L"C:\\one.txt\0C:\\two.txt\0";
	auto bytes = fileList(paths, sizeof paths / sizeof paths[0], true);
	uiprivDropData payload = {};
	assert_true(readFiles(bytes.data(), bytes.size(), &payload));
	assert_string_equal(payload.data.Files[0], "C:\\one.txt");
	assert_string_equal(payload.data.Files[1], "C:\\two.txt");
	uiprivDropDataClear(&payload);
	assert_int_equal(allocations, 0);
}

static void ansiAndLongPaths(void **state)
{
	const char paths[] = "C:\\one.txt\0C:\\two.txt\0";
	auto bytes = fileList(paths, sizeof paths, false);
	uiprivDropData payload = {};
	assert_true(readFiles(bytes.data(), bytes.size(), &payload));
	assert_string_equal(payload.data.Files[1], "C:\\two.txt");
	uiprivDropDataClear(&payload);
	std::vector<WCHAR> longPath(4098, L'a');
	longPath[4096] = longPath[4097] = 0;
	bytes = fileList(longPath.data(), longPath.size(), true);
	assert_true(readFiles(bytes.data(), bytes.size(), &payload));
	assert_int_equal(strlen(payload.data.Files[0]), 4096);
	uiprivDropDataClear(&payload);
	assert_int_equal(allocations, 0);
}

static void malformedListsAreRejected(void **state)
{
	const WCHAR paths[] = L"C:\\one.txt\0";
	auto bytes = fileList(paths, sizeof paths / sizeof paths[0], true);
	uiprivDropData payload = {};
	assert_false(readFiles(bytes.data(), sizeof(DROPFILES) - 1, &payload));
	// Remove the extra terminator: a single terminated filename is not a list.
	assert_false(readFiles(bytes.data(), bytes.size() - sizeof(WCHAR), &payload));
	uiprivDropDataClear(&payload);
	DROPFILES *header = (DROPFILES *) bytes.data();
	header->pFiles = 1;
	assert_false(readFiles(bytes.data(), bytes.size(), &payload));
	header->pFiles = sizeof(DROPFILES) + 1;
	assert_false(readFiles(bytes.data(), bytes.size(), &payload));
	header->pFiles = (DWORD) bytes.size();
	assert_false(readFiles(bytes.data(), bytes.size(), &payload));
	assert_int_equal(allocations, 0);
}

static void malformedUnicodeRejectsWholeList(void **state)
{
	const WCHAR paths[] = {L'C', L':', L'\\', L'a', 0, 0xD800, 0, 0};
	auto bytes = fileList(paths, sizeof paths / sizeof paths[0], true);
	uiprivDropData payload = {};
	assert_false(readFiles(bytes.data(), bytes.size(), &payload));
	// The caller discards the entire payload, including earlier valid paths.
	uiprivDropDataClear(&payload);
	assert_int_equal(allocations, 0);
}

class TextOffer final : public IDataObject {
public:
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void **out) override {
		if (out == NULL) return E_POINTER;
		*out = NULL;
		return E_NOINTERFACE;
	}
	ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
	ULONG STDMETHODCALLTYPE Release() override { return 1; }
	HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC *f) override {
		return f->cfFormat == CF_UNICODETEXT ? S_OK : DV_E_FORMATETC;
	}
	HRESULT STDMETHODCALLTYPE GetData(FORMATETC *f, STGMEDIUM *m) override {
		if (f->cfFormat != CF_UNICODETEXT) return DV_E_FORMATETC;
		const WCHAR text[] = L"hello";
		m->tymed = TYMED_HGLOBAL;
		m->hGlobal = GlobalAlloc(GMEM_MOVEABLE, sizeof text);
		m->pUnkForRelease = NULL;
		if (m->hGlobal == NULL) return E_OUTOFMEMORY;
		void *p = GlobalLock(m->hGlobal);
		assert_non_null(p);
		memcpy(p, text, sizeof text);
		GlobalUnlock(m->hGlobal);
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC *, STGMEDIUM *) override { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC *, FORMATETC *) override { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE SetData(FORMATETC *, STGMEDIUM *, BOOL) override { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD, IEnumFORMATETC **) override { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC *, DWORD, IAdviseSink *, DWORD *) override { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE DUnadvise(DWORD) override { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA **) override { return E_NOTIMPL; }
};

struct callbackCounts {
	int overs = 0, drops = 0, leaves = 0;
	uiprivDropEndpoint *replace = NULL;
	callbackCounts *replacement = NULL;
};

static uiDropResult receiveText(const uiDropInfo *, const uiDropData *p, void *data)
{
	((callbackCounts *) data)->drops++;
	assert_string_equal(p->Text, "hello");
	return uiDropAccepted;
}

static uiDropAction acceptText(const uiDropInfo *, void *);
static void leaveText(void *data) { ((callbackCounts *) data)->leaves++; }
static uiDropHandler textHandler(void)
{
	uiDropHandler h = {sizeof h, receiveText, acceptText, leaveText, NULL};
	return h;
}

static uiDropAction acceptText(const uiDropInfo *info, void *data)
{
	callbackCounts *c = (callbackCounts *) data;
	c->overs++;
	assert_int_equal(info->Format, uiDropFormatText);
	if (c->replace != NULL) {
		uiDropHandler h = textHandler();
		assert_int_equal(uiprivDropSetHandler(c->replace, uiDropFormatText, &h, c->replacement), uiDropStatusOK);
	}
	return uiDropActionCopy;
}

static int dropPosition(uiControl *, POINTL p, double *x, double *y)
{
	*x = p.x; *y = p.y;
	return 1;
}

static DropTarget *newDropTarget(callbackCounts *counts)
{
	uiprivDropEndpoint *e = newEndpoint();
	DropTarget *d = new DropTarget(e, NULL, dropPosition);
	e->native = d;
	e->cancel = cancel;
	e->dispose = dispose;
	uiDropHandler h = textHandler();
	assert_int_equal(uiprivDropSetHandler(e, uiDropFormatText, &h, counts), uiDropStatusOK);
	return d;
}

static void replacementRenegotiatesOnNextMotion(void **state)
{
	for (int reentrant = 0; reentrant < 2; reentrant++) {
		callbackCounts old, replacement;
		DropTarget *d = newDropTarget(&old);
		uiprivDropEndpoint *e = d->endpoint;
		TextOffer offer;
		POINTL p = {10, 20};
		DWORD effect = DROPEFFECT_COPY;
		if (reentrant) { old.replace = e; old.replacement = &replacement; }
		d->DragEnter(&offer, 0, p, &effect);
		assert_int_equal(old.overs, 1);
		assert_int_equal(effect, reentrant ? DROPEFFECT_NONE : DROPEFFECT_COPY);
		if (!reentrant) {
			uiDropHandler h = textHandler();
			assert_int_equal(uiprivDropSetHandler(e, uiDropFormatText, &h, &replacement), uiDropStatusOK);
		}
		assert_false(d->accepted); // A replacement cannot inherit the old acceptance.
		assert_int_equal(replacement.overs, 0);
		effect = DROPEFFECT_COPY;
		d->DragOver(0, p, &effect);
		assert_int_equal(effect, DROPEFFECT_COPY);
		assert_int_equal(replacement.overs, 1);
		effect = DROPEFFECT_COPY;
		d->Drop(&offer, 0, p, &effect);
		assert_int_equal(effect, DROPEFFECT_COPY);
		assert_int_equal(replacement.drops, 1);
		assert_int_equal(replacement.leaves, 1);
		assert_int_equal(old.drops, 0);
		assert_int_equal(old.leaves, 0);
		destroyEndpoint(e);
	}
}

static void replacementRequiresFreshAcceptance(void **state)
{
	callbackCounts old, replacement;
	DropTarget *d = newDropTarget(&old);
	uiprivDropEndpoint *e = d->endpoint;
	TextOffer offer;
	POINTL p = {10, 20};
	DWORD effect = DROPEFFECT_COPY;
	d->DragEnter(&offer, 0, p, &effect);
	uiDropHandler h = textHandler();
	assert_int_equal(uiprivDropSetHandler(e, uiDropFormatText, &h, &replacement), uiDropStatusOK);
	d->Drop(&offer, 0, p, &effect); // No DragOver for the replacement yet.
	assert_int_equal(effect, DROPEFFECT_NONE);
	assert_int_equal(replacement.drops, 0);
	assert_int_equal(replacement.leaves, 0);
	destroyEndpoint(e);
}

static void removalDiscardsTheNativeOffer(void **state)
{
	callbackCounts counts;
	DropTarget *d = newDropTarget(&counts);
	uiprivDropEndpoint *e = d->endpoint;
	TextOffer offer;
	POINTL p = {10, 20};
	DWORD effect = DROPEFFECT_COPY;
	d->DragEnter(&offer, 0, p, &effect);
	assert_int_equal(uiprivDropSetHandler(e, 0, NULL, NULL), uiDropStatusOK);
	uiDropHandler h = textHandler();
	assert_int_equal(uiprivDropSetHandler(e, uiDropFormatText, &h, &counts), uiDropStatusOK);
	effect = DROPEFFECT_COPY;
	d->DragOver(0, p, &effect);
	assert_int_equal(effect, DROPEFFECT_NONE);
	assert_int_equal(counts.overs, 1);
	effect = DROPEFFECT_COPY;
	d->DragEnter(&offer, 0, p, &effect);
	assert_int_equal(effect, DROPEFFECT_COPY);
	d->DragLeave();
	destroyEndpoint(e);
}

int main(void)
{
	const CMUnitTest tests[] = {
		cmocka_unit_test(widePaths),
		cmocka_unit_test(ansiAndLongPaths),
		cmocka_unit_test(malformedListsAreRejected),
		cmocka_unit_test(malformedUnicodeRejectsWholeList),
		cmocka_unit_test(replacementRenegotiatesOnNextMotion),
		cmocka_unit_test(replacementRequiresFreshAcceptance),
		cmocka_unit_test(removalDiscardsTheNativeOffer),
	};
	return cmocka_run_group_tests_name("drop Windows", tests, NULL, NULL);
}
