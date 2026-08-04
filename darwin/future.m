// 19 may 2017
#import "uipriv_darwin.h"

// functions and constants FROM THE FUTURE!
// note: for constants, dlsym() returns the address of the constant itself, as if we had done &constantName

// available in OS X 10.10
CFStringRef *uiprivFUTURE_kCTFontOpenTypeFeatureTag = NULL;
CFStringRef *uiprivFUTURE_kCTFontOpenTypeFeatureValue = NULL;

// added in OS X 10.12; we need 10.11
CFStringRef *uiprivFUTURE_kCTBackgroundColorAttributeName = NULL;

// note that we treat any error as "the symbols aren't there" (and don't care if dlclose() failed)
void uiprivLoadFutures(void)
{
	void *handle;

	// dlsym() walks the dependency chain, so opening the current process should be sufficient
	handle = dlopen(NULL, RTLD_LAZY);
	if (handle == NULL)
		return;
#define GET(var, fn) *((void **) (&var)) = dlsym(handle, #fn)
	GET(uiprivFUTURE_kCTFontOpenTypeFeatureTag, kCTFontOpenTypeFeatureTag);
	GET(uiprivFUTURE_kCTFontOpenTypeFeatureValue, kCTFontOpenTypeFeatureValue);
	GET(uiprivFUTURE_kCTBackgroundColorAttributeName, kCTBackgroundColorAttributeName);
#undef GET
	dlclose(handle);
}
