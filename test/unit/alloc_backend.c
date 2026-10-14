#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <spawn.h>
#include <stdint.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include <sys/wait.h>
#include <unistd.h>

#include "../../ui.h"
#include "../../common/uipriv.h"

#ifndef __APPLE__
#include <glib.h>
#endif

extern char **environ;
extern void uiprivInitAlloc(void);
extern void uiprivUninitAlloc(void);

static const char *executablePath;

void uiprivDoImplBug(const char *file, const char *line, const char *func,
	const char *format, ...)
{
	fprintf(stderr, "unexpected allocator implementation error: %s\n", format);
	abort();
}

void uiprivDoUserBug(const char *file, const char *line, const char *func,
	const char *format, ...)
{
	fprintf(stderr, "unexpected allocator user error: %s\n", format);
	abort();
}

static void allocatorPreservesDataAndTracksReallocation(void **state)
{
	unsigned char *bytes;

	(void) state;
	uiprivInitAlloc();
	bytes = uiprivAlloc(8, "test");
	for (size_t i = 0; i < 8; i++)
		assert_int_equal(bytes[i], 0);
	bytes[0] = 42;
	bytes = uiprivRealloc(bytes, 16, "test");
	assert_int_equal(bytes[0], 42);
	for (size_t i = 8; i < 16; i++)
		assert_int_equal(bytes[i], 0);
	bytes = uiprivRealloc(bytes, 4, "test");
	assert_int_equal(bytes[0], 42);
	uiprivFree(bytes);
	bytes = uiprivRealloc(NULL, 4, "test");
	for (size_t i = 0; i < 4; i++)
		assert_int_equal(bytes[i], 0);
	uiprivFree(bytes);
	uiprivUninitAlloc();
}

static void textArrayUsesBackendAllocators(void **state)
{
	char **texts;

	(void) state;
	uiprivInitAlloc();
	texts = uiprivNewTextArray(2);
	assert_non_null(texts);
	assert_null(texts[0]);
	assert_null(texts[1]);
	assert_null(texts[2]);
#ifdef __APPLE__
	texts[0] = malloc(1);
	assert_non_null(texts[0]);
	texts[0][0] = '\0';
#else
	texts[0] = g_strdup("");
#endif
	uiFreeTextArray(texts);
	texts = uiprivNewTextArray(0);
	assert_non_null(texts);
	uiFreeTextArray(texts);
	uiFreeTextArray(NULL);
	uiprivUninitAlloc();
}

static void assertOverflowAborts(const char *mode)
{
	posix_spawn_file_actions_t actions;
	char *args[] = { (char *) executablePath, (char *) mode, NULL };
	char diagnostic[256];
	pid_t pid;
	int fds[2];
	int status;
	ssize_t count;

	assert_int_equal(pipe(fds), 0);
	assert_int_equal(posix_spawn_file_actions_init(&actions), 0);
	assert_int_equal(posix_spawn_file_actions_adddup2(&actions, fds[1], STDERR_FILENO), 0);
	assert_int_equal(posix_spawn_file_actions_addclose(&actions, fds[0]), 0);
	assert_int_equal(posix_spawn_file_actions_addclose(&actions, fds[1]), 0);
	assert_int_equal(posix_spawn(&pid, executablePath, &actions, NULL, args, environ), 0);
	posix_spawn_file_actions_destroy(&actions);
	close(fds[1]);
	count = read(fds[0], diagnostic, sizeof diagnostic - 1);
	assert_true(count > 0);
	diagnostic[count] = '\0';
	close(fds[0]);
	assert_int_equal(waitpid(pid, &status, 0), pid);
	assert_true(WIFSIGNALED(status));
	assert_int_equal(WTERMSIG(status), SIGABRT);
	assert_non_null(strstr(diagnostic, "allocation size overflow in libui allocator"));
}

static void allocatorRejectsOverflowBeforeAllocating(void **state)
{
	(void) state;
	assertOverflowAborts("alloc-overflow");
	assertOverflowAborts("alloc-max");
	assertOverflowAborts("realloc-overflow");
	assertOverflowAborts("realloc-null-overflow");
}

int runAllocatorBackendTests(int argc, char **argv)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(allocatorPreservesDataAndTracksReallocation),
		cmocka_unit_test(textArrayUsesBackendAllocators),
		cmocka_unit_test(allocatorRejectsOverflowBeforeAllocating),
	};
	const size_t extra = sizeof(size_t) + sizeof(const char **);
	struct rlimit noCore = {0, 0};
	unsigned char *bytes;

	if (argc == 2) {
#ifdef __linux__
		/* RLIMIT_CORE does not disable Linux's piped core dump collectors. */
		if (prctl(PR_SET_DUMPABLE, 0L) != 0) {
			perror("prctl(PR_SET_DUMPABLE)");
			return 2;
		}
#endif
		setrlimit(RLIMIT_CORE, &noCore);
		uiprivInitAlloc();
		if (strcmp(argv[1], "alloc-overflow") == 0)
			uiprivAlloc(SIZE_MAX - extra + 1, "test");
		if (strcmp(argv[1], "alloc-max") == 0)
			uiprivAlloc(SIZE_MAX, "test");
		if (strcmp(argv[1], "realloc-null-overflow") == 0)
			uiprivRealloc(NULL, SIZE_MAX, "test");
		bytes = uiprivAlloc(4, "test");
		if (strcmp(argv[1], "realloc-overflow") == 0)
			uiprivRealloc(bytes, SIZE_MAX, "test");
		return 2;
	}
	executablePath = argv[0];
	return cmocka_run_group_tests(tests, NULL, NULL);
}

#ifndef __APPLE__
int main(int argc, char **argv)
{
	return runAllocatorBackendTests(argc, argv);
}
#endif
