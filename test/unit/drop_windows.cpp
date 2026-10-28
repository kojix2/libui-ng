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

static void widePathsAndCopies(void **state)
{
	const WCHAR paths[] = L"C:\\one.txt\0C:\\two.txt\0";
	auto bytes = fileList(paths, sizeof paths / sizeof paths[0], true);
	uiDropData payload = {};
	assert_true(readFiles(bytes.data(), bytes.size(), &payload));
	assert_int_equal(uiDropDataFileCount(&payload), 2);
	char **copy = uiDropDataCopyFiles(&payload);
	uiprivDropDataClear(&payload);
	assert_string_equal(copy[0], "C:\\one.txt");
	assert_string_equal(copy[1], "C:\\two.txt");
	assert_null(copy[2]);
	uiFreeTextArray(copy);
	assert_int_equal(allocations, 0);
}

static void ansiAndLongPaths(void **state)
{
	const char paths[] = "C:\\one.txt\0C:\\two.txt\0";
	auto bytes = fileList(paths, sizeof paths, false);
	uiDropData payload = {};
	assert_true(readFiles(bytes.data(), bytes.size(), &payload));
	assert_string_equal(uiDropDataFilePath(&payload, 1), "C:\\two.txt");
	uiprivDropDataClear(&payload);
	std::vector<WCHAR> longPath(4098, L'a');
	longPath[4096] = longPath[4097] = 0;
	bytes = fileList(longPath.data(), longPath.size(), true);
	assert_true(readFiles(bytes.data(), bytes.size(), &payload));
	assert_int_equal(strlen(uiDropDataFilePath(&payload, 0)), 4096);
	uiprivDropDataClear(&payload);
	assert_int_equal(allocations, 0);
}

static void malformedListsAreRejected(void **state)
{
	const WCHAR paths[] = L"C:\\one.txt\0";
	auto bytes = fileList(paths, sizeof paths / sizeof paths[0], true);
	uiDropData payload = {};
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
	uiDropData payload = {};
	assert_false(readFiles(bytes.data(), bytes.size(), &payload));
	// The caller discards the entire payload, including earlier valid paths.
	uiprivDropDataClear(&payload);
	assert_int_equal(allocations, 0);
}

int main(void)
{
	const CMUnitTest tests[] = {
		cmocka_unit_test(widePathsAndCopies),
		cmocka_unit_test(ansiAndLongPaths),
		cmocka_unit_test(malformedListsAreRejected),
		cmocka_unit_test(malformedUnicodeRejectsWholeList),
	};
	return cmocka_run_group_tests_name("drop Windows payloads", tests, NULL, NULL);
}
