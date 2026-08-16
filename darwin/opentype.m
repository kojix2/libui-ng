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

static void addCTFeatureEntry(struct addCTFeatureEntryParams *p)
{
	CFDictionaryRef featureDict;
	CFNumberRef valueNum;
	const void *keys[2], *values[2];

	keys[0] = p->tagKey;
	values[0] = p->tagValue;

	keys[1] = p->valueKey;
	valueNum = CFNumberCreate(NULL, p->valueType, p->valueValue);
	values[1] = valueNum;

	featureDict = CFDictionaryCreate(NULL,
		keys, values, 2,
		&kCFCopyStringDictionaryKeyCallBacks,
		&kCFTypeDictionaryValueCallBacks);
	if (featureDict == NULL) {
		// TODO
	}
	CFArrayAppendValue(p->array, featureDict);

	CFRelease(featureDict);
	CFRelease(valueNum);
}

static uiForEach otfArrayForEach(const uiOpenTypeFeatures *otf, char a, char b, char c, char d, uint32_t value, void *data)
{
	struct addCTFeatureEntryParams p;
	char tagcstr[5];
	CFStringRef tagstr;

	p.array = (CFMutableArrayRef) data;

	// These keys are available since OS X 10.10; libui-ng requires 10.11.
	p.tagKey = kCTFontOpenTypeFeatureTag;
	tagcstr[0] = a;
	tagcstr[1] = b;
	tagcstr[2] = c;
	tagcstr[3] = d;
	tagcstr[4] = '\0';
	tagstr = CFStringCreateWithCString(NULL, tagcstr, kCFStringEncodingUTF8);
	if (tagstr == NULL) {
		// TODO
	}
	p.tagValue = tagstr;

	p.valueKey = kCTFontOpenTypeFeatureValue;
	p.valueType = kCFNumberSInt32Type;
	p.valueValue = (const SInt32 *) (&value);
	addCTFeatureEntry(&p);

	CFRelease(tagstr);
	return uiForEachContinue;
}

CFArrayRef uiprivOpenTypeFeaturesToCTFeatures(const uiOpenTypeFeatures *otf)
{
	CFMutableArrayRef array;

	array = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
	if (array == NULL) {
		// TODO
	}
	uiOpenTypeFeaturesForEach(otf, otfArrayForEach, array);
	return array;
}
