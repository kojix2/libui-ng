// 11 may 2017
#import "uipriv_darwin.h"
#import "attrstr.h"

struct addCTFeatureEntryParams {
	CFMutableArrayRef array;
	const void *tagKey;
	const void *tagValue;
	const void *valueKey;
	CFNumberType valueType;
	const void *valueValue;
};

struct otfArrayContext {
	CFMutableArrayRef array;
	int failed;
};

static int addCTFeatureEntry(struct addCTFeatureEntryParams *p)
{
	CFDictionaryRef featureDict;
	CFNumberRef valueNum;
	const void *keys[2], *values[2];

	keys[0] = p->tagKey;
	values[0] = p->tagValue;

	keys[1] = p->valueKey;
	valueNum = CFNumberCreate(NULL, p->valueType, p->valueValue);
	if (valueNum == NULL)
		return 0;
	values[1] = valueNum;

	featureDict = CFDictionaryCreate(NULL,
		keys, values, 2,
		&kCFCopyStringDictionaryKeyCallBacks,
		&kCFTypeDictionaryValueCallBacks);
	if (featureDict == NULL) {
		CFRelease(valueNum);
		return 0;
	}
	CFArrayAppendValue(p->array, featureDict);

	CFRelease(featureDict);
	CFRelease(valueNum);
	return 1;
}

static uiForEach otfArrayForEach(const uiOpenTypeFeatures *otf, char a, char b, char c, char d, uint32_t value, void *data)
{
	struct addCTFeatureEntryParams p;
	struct otfArrayContext *context = (struct otfArrayContext *) data;
	UInt8 tagBytes[4];
	CFStringRef tagstr;

	p.array = context->array;

	// These keys are available since OS X 10.10; libui-ng requires macOS 10.12.
	p.tagKey = kCTFontOpenTypeFeatureTag;
	tagBytes[0] = (UInt8) a;
	tagBytes[1] = (UInt8) b;
	tagBytes[2] = (UInt8) c;
	tagBytes[3] = (UInt8) d;
	tagstr = CFStringCreateWithBytes(NULL, tagBytes, 4, kCFStringEncodingASCII, false);
	if (tagstr == NULL)
		goto fail;
	p.tagValue = tagstr;

	p.valueKey = kCTFontOpenTypeFeatureValue;
	p.valueType = kCFNumberSInt32Type;
	p.valueValue = (const SInt32 *) (&value);
	if (!addCTFeatureEntry(&p)) {
		CFRelease(tagstr);
		goto fail;
	}

	CFRelease(tagstr);
	return uiForEachContinue;

fail:
	context->failed = 1;
	return uiForEachStop;
}

CFArrayRef uiprivOpenTypeFeaturesToCTFeatures(const uiOpenTypeFeatures *otf)
{
	CFMutableArrayRef array;
	struct otfArrayContext context;

	array = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
	if (array == NULL)
		return NULL;
	context.array = array;
	context.failed = 0;
	uiOpenTypeFeaturesForEach(otf, otfArrayForEach, &context);
	if (context.failed) {
		CFRelease(array);
		return NULL;
	}
	return array;
}
