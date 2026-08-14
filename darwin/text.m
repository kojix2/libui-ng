// 10 april 2015
#import "uipriv_darwin.h"

char *uiDarwinNSStringToText(NSString *s)
{
	char *out;

	out = strdup([s UTF8String]);
	if (out == NULL) {
		fprintf(stderr, "memory exhausted in uiDarwinNSStringToText()\n");
		abort();
	}
	return out;
}

void uiFreeText(char *s)
{
	free(s);
}

int uiprivStricmp(const char *a, const char *b)
{
	NSString *as, *bs;
	NSComparisonResult result;

	as = [[NSString alloc] initWithUTF8String:a];
	bs = [[NSString alloc] initWithUTF8String:b];
	result = [as caseInsensitiveCompare:bs];
	[bs release];
	[as release];
	return (int) result;
}
