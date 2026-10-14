#import <AppKit/AppKit.h>

extern int runAllocatorBackendTests(int argc, char **argv);

int main(int argc, char **argv)
{
	NSAutoreleasePool *pool = [NSAutoreleasePool new];
	int result = runAllocatorBackendTests(argc, argv);
	[pool drain];
	return result;
}
