// 14 february 2017
#import "uipriv_darwin.h"
#import "attrstr.h"

// Maps OpenType feature tags to legacy AAT feature type/selector pairs for
// Core Text versions that do not expose OpenType feature dictionary keys.

static void boolspec(uint32_t value, uint16_t type, uint16_t ifTrue, uint16_t ifFalse, uiprivAATBlock f)
{
	// TODO are values other than 1 accepted for true by OpenType itself? (same for the rest of the file)
	if (value != 0) {
		f(type, ifTrue);
		return;
	}
	f(type, ifFalse);
}

// TODO remove the need for this
// TODO remove x8tox32()
#define x8tox32(x) ((uint32_t) (((uint8_t) (x)) & 0xFF))
#define mkTag(a, b, c, d)		\
	((x8tox32(a) << 24) |	\
	(x8tox32(b) << 16) |		\
	(x8tox32(c) << 8) |		\
	x8tox32(d))

struct booleanFeatureMapping {
	uint32_t tag;
	uint16_t type;
	uint16_t onSelector;
	uint16_t offSelector;
};

struct nonzeroFeatureMapping {
	uint32_t tag;
	uint16_t type;
	uint16_t selector;
};

static const struct booleanFeatureMapping booleanFeatureMappings[] = {
	{ mkTag('l', 'i', 'g', 'a'), kLigaturesType, kCommonLigaturesOnSelector, kCommonLigaturesOffSelector },
	{ mkTag('r', 'l', 'i', 'g'), kLigaturesType, kRequiredLigaturesOnSelector, kRequiredLigaturesOffSelector },
	{ mkTag('d', 'l', 'i', 'g'), kLigaturesType, kRareLigaturesOnSelector, kRareLigaturesOffSelector },
	{ mkTag('c', 'l', 'i', 'g'), kLigaturesType, kContextualLigaturesOnSelector, kContextualLigaturesOffSelector },
	{ mkTag('z', 'e', 'r', 'o'), kTypographicExtrasType, kSlashedZeroOnSelector, kSlashedZeroOffSelector },
	{ mkTag('m', 'g', 'r', 'k'), kMathematicalExtrasType, kMathematicalGreekOnSelector, kMathematicalGreekOffSelector },
	{ mkTag('c', 'a', 's', 'e'), kCaseSensitiveLayoutType, kCaseSensitiveLayoutOnSelector, kCaseSensitiveLayoutOffSelector },
	{ mkTag('c', 'p', 's', 'p'), kCaseSensitiveLayoutType, kCaseSensitiveSpacingOnSelector, kCaseSensitiveSpacingOffSelector },
	{ mkTag('h', 'k', 'n', 'a'), kAlternateKanaType, kAlternateHorizKanaOnSelector, kAlternateHorizKanaOffSelector },
	{ mkTag('v', 'k', 'n', 'a'), kAlternateKanaType, kAlternateVertKanaOnSelector, kAlternateVertKanaOffSelector },
	{ mkTag('s', 's', '0', '1'), kStylisticAlternativesType, kStylisticAltOneOnSelector, kStylisticAltOneOffSelector },
	{ mkTag('s', 's', '0', '2'), kStylisticAlternativesType, kStylisticAltTwoOnSelector, kStylisticAltTwoOffSelector },
	{ mkTag('s', 's', '0', '3'), kStylisticAlternativesType, kStylisticAltThreeOnSelector, kStylisticAltThreeOffSelector },
	{ mkTag('s', 's', '0', '4'), kStylisticAlternativesType, kStylisticAltFourOnSelector, kStylisticAltFourOffSelector },
	{ mkTag('s', 's', '0', '5'), kStylisticAlternativesType, kStylisticAltFiveOnSelector, kStylisticAltFiveOffSelector },
	{ mkTag('s', 's', '0', '6'), kStylisticAlternativesType, kStylisticAltSixOnSelector, kStylisticAltSixOffSelector },
	{ mkTag('s', 's', '0', '7'), kStylisticAlternativesType, kStylisticAltSevenOnSelector, kStylisticAltSevenOffSelector },
	{ mkTag('s', 's', '0', '8'), kStylisticAlternativesType, kStylisticAltEightOnSelector, kStylisticAltEightOffSelector },
	{ mkTag('s', 's', '0', '9'), kStylisticAlternativesType, kStylisticAltNineOnSelector, kStylisticAltNineOffSelector },
	{ mkTag('s', 's', '1', '0'), kStylisticAlternativesType, kStylisticAltTenOnSelector, kStylisticAltTenOffSelector },
	{ mkTag('s', 's', '1', '1'), kStylisticAlternativesType, kStylisticAltElevenOnSelector, kStylisticAltElevenOffSelector },
	{ mkTag('s', 's', '1', '2'), kStylisticAlternativesType, kStylisticAltTwelveOnSelector, kStylisticAltTwelveOffSelector },
	{ mkTag('s', 's', '1', '3'), kStylisticAlternativesType, kStylisticAltThirteenOnSelector, kStylisticAltThirteenOffSelector },
	{ mkTag('s', 's', '1', '4'), kStylisticAlternativesType, kStylisticAltFourteenOnSelector, kStylisticAltFourteenOffSelector },
	{ mkTag('s', 's', '1', '5'), kStylisticAlternativesType, kStylisticAltFifteenOnSelector, kStylisticAltFifteenOffSelector },
	{ mkTag('s', 's', '1', '6'), kStylisticAlternativesType, kStylisticAltSixteenOnSelector, kStylisticAltSixteenOffSelector },
	{ mkTag('s', 's', '1', '7'), kStylisticAlternativesType, kStylisticAltSeventeenOnSelector, kStylisticAltSeventeenOffSelector },
	{ mkTag('s', 's', '1', '8'), kStylisticAlternativesType, kStylisticAltEighteenOnSelector, kStylisticAltEighteenOffSelector },
	{ mkTag('s', 's', '1', '9'), kStylisticAlternativesType, kStylisticAltNineteenOnSelector, kStylisticAltNineteenOffSelector },
	{ mkTag('s', 's', '2', '0'), kStylisticAlternativesType, kStylisticAltTwentyOnSelector, kStylisticAltTwentyOffSelector },
	{ mkTag('c', 'a', 'l', 't'), kContextualAlternatesType, kContextualAlternatesOnSelector, kContextualAlternatesOffSelector },
	{ mkTag('s', 'w', 's', 'h'), kContextualAlternatesType, kSwashAlternatesOnSelector, kSwashAlternatesOffSelector },
	{ mkTag('c', 's', 'w', 'h'), kContextualAlternatesType, kContextualSwashAlternatesOnSelector, kContextualSwashAlternatesOffSelector },
};

static const struct nonzeroFeatureMapping nonzeroFeatureMappings[] = {
	{ mkTag('p', 'n', 'u', 'm'), kNumberSpacingType, kProportionalNumbersSelector },
	{ mkTag('t', 'n', 'u', 'm'), kNumberSpacingType, kMonospacedNumbersSelector },
	{ mkTag('s', 'u', 'p', 's'), kVerticalPositionType, kSuperiorsSelector },
	{ mkTag('s', 'u', 'b', 's'), kVerticalPositionType, kInferiorsSelector },
	{ mkTag('o', 'r', 'd', 'n'), kVerticalPositionType, kOrdinalsSelector },
	{ mkTag('s', 'i', 'n', 'f'), kVerticalPositionType, kScientificInferiorsSelector },
	{ mkTag('a', 'f', 'r', 'c'), kFractionsType, kVerticalFractionsSelector },
	{ mkTag('f', 'r', 'a', 'c'), kFractionsType, kDiagonalFractionsSelector },
	{ mkTag('t', 'i', 't', 'l'), kStyleOptionsType, kTitlingCapsSelector },
	{ mkTag('t', 'r', 'a', 'd'), kCharacterShapeType, kTraditionalCharactersSelector },
	{ mkTag('s', 'm', 'p', 'l'), kCharacterShapeType, kSimplifiedCharactersSelector },
	{ mkTag('j', 'p', '7', '8'), kCharacterShapeType, kJIS1978CharactersSelector },
	{ mkTag('j', 'p', '8', '3'), kCharacterShapeType, kJIS1983CharactersSelector },
	{ mkTag('j', 'p', '9', '0'), kCharacterShapeType, kJIS1990CharactersSelector },
	{ mkTag('e', 'x', 'p', 't'), kCharacterShapeType, kExpertCharactersSelector },
	{ mkTag('j', 'p', '0', '4'), kCharacterShapeType, kJIS2004CharactersSelector },
	{ mkTag('h', 'o', 'j', 'o'), kCharacterShapeType, kHojoCharactersSelector },
	{ mkTag('n', 'l', 'c', 'k'), kCharacterShapeType, kNLCCharactersSelector },
	{ mkTag('t', 'n', 'a', 'm'), kCharacterShapeType, kTraditionalNamesCharactersSelector },
	{ mkTag('h', 'n', 'g', 'l'), kTransliterationType, kHanjaToHangulSelector },
	{ mkTag('p', 'c', 'a', 'p'), kLowerCaseType, kLowerCasePetiteCapsSelector },
	{ mkTag('c', '2', 's', 'c'), kUpperCaseType, kUpperCaseSmallCapsSelector },
	{ mkTag('c', '2', 'p', 'c'), kUpperCaseType, kUpperCasePetiteCapsSelector },
};

static const struct booleanFeatureMapping *findBooleanFeatureMapping(uint32_t tag)
{
	size_t i;

	for (i = 0; i < sizeof (booleanFeatureMappings) / sizeof (booleanFeatureMappings[0]); i++)
		if (booleanFeatureMappings[i].tag == tag)
			return &(booleanFeatureMappings[i]);
	return NULL;
}

static const struct nonzeroFeatureMapping *findNonzeroFeatureMapping(uint32_t tag)
{
	size_t i;

	for (i = 0; i < sizeof (nonzeroFeatureMappings) / sizeof (nonzeroFeatureMappings[0]); i++)
		if (nonzeroFeatureMappings[i].tag == tag)
			return &(nonzeroFeatureMappings[i]);
	return NULL;
}

// TODO double-check drawtext example to make sure all of these are used properly (I already screwed dlig up by putting clig twice instead)
void uiprivOpenTypeToAAT(char a, char b, char c, char d, uint32_t value, uiprivAATBlock f)
{
	uint32_t tag;
	const struct booleanFeatureMapping *booleanMapping;
	const struct nonzeroFeatureMapping *nonzeroMapping;

	tag = mkTag(a, b, c, d);
	booleanMapping = findBooleanFeatureMapping(tag);
	if (booleanMapping != NULL) {
		boolspec(value, booleanMapping->type, booleanMapping->onSelector, booleanMapping->offSelector, f);
		return;
	}
	nonzeroMapping = findNonzeroFeatureMapping(tag);
	if (nonzeroMapping != NULL) {
		if (value != 0)
			f(nonzeroMapping->type, nonzeroMapping->selector);
		return;
	}

	switch (tag) {
	case mkTag('h', 'l', 'i', 'g'):
	// This technically isn't what is meant by "historical ligatures", but Core Text's internal AAT-to-OpenType mapping says to include it, so we include it too
	case mkTag('h', 'i', 's', 't'):
		boolspec(value, kLigaturesType,
			kHistoricalLigaturesOnSelector,
			kHistoricalLigaturesOffSelector,
			f);
		break;
	case mkTag('u', 'n', 'i', 'c'):
		// TODO is this correct, or should we provide an else case?
		if (value != 0)
			// this is undocumented; it comes from Core Text's internal AAT-to-OpenType conversion table
			f(kLetterCaseType, 14);
		break;

	case mkTag('o', 'r', 'n', 'm'):
		f(kOrnamentSetsType, (uint16_t) value);
		break;
	case mkTag('a', 'a', 'l', 't'):
		f(kCharacterAlternativesType, (uint16_t) value);
		break;

	case mkTag('o', 'n', 'u', 'm'):
	// Core Text's internal AAT-to-OpenType mapping says to include this, so we include it too
	// TODO is it always set?
	case mkTag('l', 'n', 'u', 'm'):
		// TODO is this correct, or should we provide an else case?
		if (value != 0)
			f(kNumberCaseType, kLowerCaseNumbersSelector);
		break;
	case mkTag('n', 'a', 'l', 't'):
		f(kAnnotationType, (uint16_t) value);
		break;
	case mkTag('r', 'u', 'b', 'y'):
		// include this for completeness
		boolspec(value, kRubyKanaType,
			kRubyKanaSelector,
			kNoRubyKanaSelector,
			f);
		// this is the current one
		boolspec(value, kRubyKanaType,
			kRubyKanaOnSelector,
			kRubyKanaOffSelector,
			f);
		break;
	case mkTag('i', 't', 'a', 'l'):
		// include this for completeness
		boolspec(value, kItalicCJKRomanType,
			kCJKItalicRomanSelector,
			kNoCJKItalicRomanSelector,
			f);
		// this is the current one
		boolspec(value, kItalicCJKRomanType,
			kCJKItalicRomanOnSelector,
			kCJKItalicRomanOffSelector,
			f);
		break;

	// TODO will the following handle all cases properly, or are elses going to be needed?
	case mkTag('s', 'm', 'c', 'p'):
		if (value != 0) {
			// include this for compatibility (some fonts that come with OS X still use this!)
			// TODO make it boolean?
			f(kLetterCaseType, kSmallCapsSelector);
			// this is the current one
			f(kLowerCaseType, kLowerCaseSmallCapsSelector);
		}
		break;
	}
	// TODO handle this properly
	// (it used to return 0 when this still returned the number of selectors produced but IDK what properly is anymore)
}
