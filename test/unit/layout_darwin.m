#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#import <AppKit/AppKit.h>

#include "../../ui.h"

#define EPSILON 0.001

static NSView *controlView(uiControl *c)
{
	return (NSView *) uiControlHandle(c);
}

static NSLayoutConstraint *minimumConstraint(NSView *view,
	NSLayoutAttribute attribute)
{
	NSLayoutConstraint *constraint;

	for (constraint in [view constraints])
		if ([constraint firstItem] == view &&
			[constraint firstAttribute] == attribute &&
			[[constraint identifier]
				isEqualToString:@"uiControl minimum size override"])
			return constraint;
	return nil;
}

#define assertNear(actual, expected) \
	assert_float_equal((actual), (expected), EPSILON)

static void spinboxHasStableNaturalWidthAndHeight(void **state)
{
	uiSpinbox *spinbox;
	NSSize natural;
	NSSize changed;
	NSTextField *field;
	NSStepper *stepper;
	NSView *view;
	NSView *subview;
	NSRect fieldAlignmentRect;
	NSRect stepperAlignmentRect;
	CGFloat expectedBaseline;

	(void) state;
	spinbox = uiNewSpinbox(-1000, 1000);
	view = controlView(uiControl(spinbox));
	field = nil;
	stepper = nil;
	for (subview in [view subviews]) {
		if ([subview isKindOfClass:[NSStepper class]])
			stepper = (NSStepper *) subview;
		else if ([subview isKindOfClass:[NSTextField class]])
			field = (NSTextField *) subview;
	}
	assert_non_null(field);
	assert_non_null(stepper);

	natural = [view intrinsicContentSize];
	assertNear(natural.width,
		[field intrinsicContentSize].width +
		[stepper intrinsicContentSize].width);
	assert_true(natural.height == NSViewNoIntrinsicMetric);

	uiSpinboxSetValue(spinbox, 1);
	changed = [view intrinsicContentSize];
	assertNear(changed.width, natural.width);
	uiSpinboxSetValue(spinbox, 1000);
	changed = [view intrinsicContentSize];
	assertNear(changed.width, natural.width);

	[view setFrameSize:NSMakeSize(natural.width, 80)];
	[view layoutSubtreeIfNeeded];
	// Auto Layout sizes and positions alignment rects. On some macOS versions,
	// NSStepper's frame includes vertical alignment insets (for example, a
	// 28-point frame around a 20-point intrinsic alignment rect).
	fieldAlignmentRect = [field alignmentRectForFrame:[field frame]];
	stepperAlignmentRect = [stepper alignmentRectForFrame:[stepper frame]];
	assertNear(NSHeight(fieldAlignmentRect), [field intrinsicContentSize].height);
	assertNear(NSHeight(stepperAlignmentRect), [stepper intrinsicContentSize].height);
	assertNear(NSMidY(fieldAlignmentRect), NSMidY(stepperAlignmentRect));
	assertNear(NSMidY(stepperAlignmentRect), NSMidY([view bounds]));
	expectedBaseline = (NSHeight([view bounds]) -
		[field intrinsicContentSize].height) / 2 +
		[field firstBaselineOffsetFromTop];
	assertNear([view firstBaselineOffsetFromTop], expectedBaseline);

	uiControlDestroy(uiControl(spinbox));
}

static void formTracksCenteredSpinboxBaseline(void **state)
{
	uiForm *form;
	uiSpinbox *spinbox;
	NSGridView *grid;
	NSTextField *field;
	NSTextField *label;
	NSStepper *stepper;
	NSView *spinboxView;
	NSView *subview;
	NSRect fieldFrame;
	NSRect labelFrame;
	CGFloat fieldBaseline;
	CGFloat labelBaseline;

	(void) state;
	form = uiNewForm();
	spinbox = uiNewSpinbox(0, 1000);
	uiFormAppend(form, "Spinbox", uiControl(spinbox), 0);
	uiControlSetMinimumSize(uiControl(spinbox), -1, 80);
	grid = (NSGridView *) controlView(uiControl(form));
	spinboxView = controlView(uiControl(spinbox));
	label = (NSTextField *) [[grid cellAtColumnIndex:0 rowIndex:0] contentView];
	field = nil;
	stepper = nil;
	for (subview in [spinboxView subviews]) {
		if ([subview isKindOfClass:[NSStepper class]])
			stepper = (NSStepper *) subview;
		else if ([subview isKindOfClass:[NSTextField class]])
			field = (NSTextField *) subview;
	}
	assert_non_null(field);
	assert_non_null(stepper);
	[grid setFrameSize:NSMakeSize(320, [grid fittingSize].height)];
	[grid layoutSubtreeIfNeeded];
	assertNear(NSMidY([field frame]), NSMidY([spinboxView bounds]));
	assertNear(NSMidY([stepper frame]), NSMidY([spinboxView bounds]));
	fieldFrame = [field convertRect:[field bounds] toView:grid];
	labelFrame = [label convertRect:[label bounds] toView:grid];
	fieldBaseline = NSMaxY(fieldFrame) - [field firstBaselineOffsetFromTop];
	labelBaseline = NSMaxY(labelFrame) - [label firstBaselineOffsetFromTop];
	assertNear(fieldBaseline, labelBaseline);

	uiControlSetMinimumSize(uiControl(spinbox), -1, -1);
	[grid layoutSubtreeIfNeeded];
	fieldFrame = [field convertRect:[field bounds] toView:grid];
	labelFrame = [label convertRect:[label bounds] toView:grid];
	fieldBaseline = NSMaxY(fieldFrame) - [field firstBaselineOffsetFromTop];
	labelBaseline = NSMaxY(labelFrame) - [label firstBaselineOffsetFromTop];
	assertNear(fieldBaseline, labelBaseline);
	uiControlDestroy(uiControl(form));
}

static void minimumSizeBeforeAndAfterInsertionDoesNotConflict(void **state)
{
	uiBox *box;
	uiEntry *before;
	uiEntry *after;
	NSLayoutConstraint *constraint;
	NSLayoutPriority beforeHorizontalHugging;
	NSLayoutPriority beforeCompression;
	NSLayoutPriority afterHorizontalHugging;
	NSView *beforeView;
	NSView *afterView;

	(void) state;
	box = uiNewHorizontalBox();
	before = uiNewEntry();
	beforeView = controlView(uiControl(before));
	beforeHorizontalHugging = [beforeView
		contentHuggingPriorityForOrientation:
			NSLayoutConstraintOrientationHorizontal];
	beforeCompression = [beforeView
		contentCompressionResistancePriorityForOrientation:
			NSLayoutConstraintOrientationHorizontal];
	uiControlSetMinimumSize(uiControl(before), 160, -1);
	uiBoxAppend(box, uiControl(before), 0);
	assertNear([beforeView contentHuggingPriorityForOrientation:
		NSLayoutConstraintOrientationHorizontal],
		NSLayoutPriorityRequired - 1);
	constraint = minimumConstraint(beforeView, NSLayoutAttributeWidth);
	assert_non_null(constraint);
	assertNear([constraint constant], 160);
	assertNear([constraint priority], NSLayoutPriorityRequired);

	after = uiNewEntry();
	afterView = controlView(uiControl(after));
	afterHorizontalHugging = [afterView
		contentHuggingPriorityForOrientation:
			NSLayoutConstraintOrientationHorizontal];
	uiBoxAppend(box, uiControl(after), 0);
	uiControlSetMinimumSize(uiControl(after), 160, -1);
	assertNear([afterView contentHuggingPriorityForOrientation:
		NSLayoutConstraintOrientationHorizontal],
		NSLayoutPriorityRequired - 1);
	constraint = minimumConstraint(afterView, NSLayoutAttributeWidth);
	assert_non_null(constraint);
	assertNear([constraint constant], 160);
	assertNear([constraint priority], NSLayoutPriorityRequired);

	uiControlSetMinimumSize(uiControl(before), -1, -1);
	assert_null(minimumConstraint(beforeView, NSLayoutAttributeWidth));
	assertNear([beforeView contentCompressionResistancePriorityForOrientation:
		NSLayoutConstraintOrientationHorizontal], beforeCompression);
	uiControlSetMinimumSize(uiControl(after), 0, 48);
	assertNear([minimumConstraint(afterView, NSLayoutAttributeWidth) constant], 0);
	assertNear([minimumConstraint(afterView, NSLayoutAttributeHeight) constant], 48);
	uiControlSetMinimumSize(uiControl(after), -1, -1);
	assert_null(minimumConstraint(afterView, NSLayoutAttributeWidth));
	assert_null(minimumConstraint(afterView, NSLayoutAttributeHeight));

	uiBoxDelete(box, 1);
	assertNear([afterView contentHuggingPriorityForOrientation:
		NSLayoutConstraintOrientationHorizontal], afterHorizontalHugging);
	uiControlDestroy(uiControl(after));
	uiBoxDelete(box, 0);
	assertNear([beforeView contentHuggingPriorityForOrientation:
		NSLayoutConstraintOrientationHorizontal], beforeHorizontalHugging);
	uiControlDestroy(uiControl(before));
	uiControlDestroy(uiControl(box));
}

static void formUsesOptionalPreferredHeight(void **state)
{
	uiEntry *entry;
	uiForm *form;
	NSLayoutPriority original;
	NSView *view;

	(void) state;
	form = uiNewForm();
	entry = uiNewEntry();
	view = controlView(uiControl(entry));
	original = [view contentHuggingPriorityForOrientation:
		NSLayoutConstraintOrientationVertical];
	uiFormAppend(form, "Entry", uiControl(entry), 0);
	assertNear([view contentHuggingPriorityForOrientation:
		NSLayoutConstraintOrientationVertical],
		NSLayoutPriorityRequired - 1);
	uiControlSetMinimumSize(uiControl(entry), -1, 80);
	assertNear([minimumConstraint(view, NSLayoutAttributeHeight) priority],
		NSLayoutPriorityRequired);
	uiControlSetMinimumSize(uiControl(entry), -1, -1);
	uiFormDelete(form, 0);
	assertNear([view contentHuggingPriorityForOrientation:
		NSLayoutConstraintOrientationVertical], original);
	uiControlDestroy(uiControl(entry));
	uiControlDestroy(uiControl(form));
}

static void radioButtonsUseStackHuggingAndRestoreIt(void **state)
{
	uiBox *horizontalBox;
	uiBox *verticalBox;
	uiRadioButtons *radioButtons;
	NSLayoutPriority horizontal;
	NSLayoutPriority vertical;
	NSStackView *view;

	(void) state;
	radioButtons = uiNewRadioButtons();
	uiRadioButtonsAppend(radioButtons, "Short");
	uiRadioButtonsAppend(radioButtons, "A much longer option");
	view = (NSStackView *) controlView(uiControl(radioButtons));
	horizontal = [view huggingPriorityForOrientation:
		NSLayoutConstraintOrientationHorizontal];
	vertical = [view huggingPriorityForOrientation:
		NSLayoutConstraintOrientationVertical];

	horizontalBox = uiNewHorizontalBox();
	uiBoxAppend(horizontalBox, uiControl(radioButtons), 0);
	assertNear([view huggingPriorityForOrientation:
		NSLayoutConstraintOrientationHorizontal],
		NSLayoutPriorityRequired - 1);
	uiBoxDelete(horizontalBox, 0);
	assertNear([view huggingPriorityForOrientation:
		NSLayoutConstraintOrientationHorizontal], horizontal);
	uiControlDestroy(uiControl(horizontalBox));

	verticalBox = uiNewVerticalBox();
	uiBoxAppend(verticalBox, uiControl(radioButtons), 0);
	assertNear([view huggingPriorityForOrientation:
		NSLayoutConstraintOrientationVertical],
		NSLayoutPriorityRequired - 1);
	uiBoxDelete(verticalBox, 0);
	assertNear([view huggingPriorityForOrientation:
		NSLayoutConstraintOrientationVertical], vertical);
	uiControlDestroy(uiControl(verticalBox));
	uiControlDestroy(uiControl(radioButtons));
}

static void gridKeepsNonexpandingAxisAtFittingSize(void **state)
{
	uiBox *box;
	uiButton *spanning;
	uiGrid *grid;
	uiLabel *rowLabel;
	uiLabel *spacer;
	NSGridView *nativeGrid;
	NSSize before;
	NSSize hidden;
	NSSize restored;
	NSView *boxView;
	NSView *outer;
	NSView *subview;
	CGFloat originalFrameHeight;

	(void) state;
	grid = uiNewGrid();
	uiGridSetPadded(grid, 1);
	rowLabel = uiNewLabel("Expandable entry");
	uiGridAppend(grid, uiControl(rowLabel),
		0, 0, 1, 1, 0, uiAlignEnd, 0, uiAlignCenter);
	uiGridAppend(grid, uiControl(uiNewEntry()),
		1, 0, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);
	spanning = uiNewButton("Two-column spanning control");
	uiGridAppend(grid, uiControl(spanning),
		0, 1, 2, 1, 1, uiAlignFill, 0, uiAlignCenter);
	outer = controlView(uiControl(grid));
	nativeGrid = nil;
	for (subview in [outer subviews])
		if ([subview isKindOfClass:[NSGridView class]])
			nativeGrid = (NSGridView *) subview;
	assert_non_null(nativeGrid);
	before = [outer intrinsicContentSize];
	assert_true(before.width == NSViewNoIntrinsicMetric);
	assertNear(before.height, [nativeGrid fittingSize].height);
	box = uiNewVerticalBox();
	uiBoxAppend(box, uiControl(grid), 0);
	spacer = uiNewLabel("");
	uiBoxAppend(box, uiControl(spacer), 1);
	boxView = controlView(uiControl(box));
	[boxView setFrameSize:NSMakeSize(500, 300)];
	[boxView layoutSubtreeIfNeeded];
	assertNear(NSHeight([outer frame]), before.height);
	assert_true(NSHeight([controlView(uiControl(spacer)) frame]) > 100);

	uiControlHide(uiControl(spanning));
	[boxView layoutSubtreeIfNeeded];
	hidden = [outer intrinsicContentSize];
	assert_true(hidden.width == NSViewNoIntrinsicMetric);
	assert_true(hidden.height < before.height);
	assertNear(NSHeight([outer frame]), hidden.height);
	uiControlShow(uiControl(spanning));
	[boxView layoutSubtreeIfNeeded];
	restored = [outer intrinsicContentSize];
	assertNear(restored.height, before.height);
	assertNear(NSHeight([outer frame]), before.height);
	originalFrameHeight = NSHeight([outer frame]);
	uiLabelSetText(rowLabel, "Expandable entry\nsecond line\nthird line");
	[boxView layoutSubtreeIfNeeded];
	assert_true(NSHeight([outer frame]) > originalFrameHeight);
	assertNear(NSHeight([outer frame]), [outer intrinsicContentSize].height);

	uiControlDestroy(uiControl(box));
}

static void imageViewDefaultMinimumCanBeOverriddenAndRestored(void **state)
{
	uiImageView *imageView;
	NSLayoutConstraint *height;
	NSLayoutConstraint *width;
	NSSize intrinsic;
	NSView *view;

	(void) state;
	imageView = uiNewImageView();
	view = controlView(uiControl(imageView));
	intrinsic = [view intrinsicContentSize];
	assert_true(intrinsic.width == NSViewNoIntrinsicMetric);
	assert_true(intrinsic.height == NSViewNoIntrinsicMetric);
	width = minimumConstraint(view, NSLayoutAttributeWidth);
	height = minimumConstraint(view, NSLayoutAttributeHeight);
	assert_non_null(width);
	assert_non_null(height);
	assertNear([width constant], 16);
	assertNear([height constant], 16);

	uiControlSetMinimumSize(uiControl(imageView), 0, 0);
	assertNear([width constant], 0);
	assertNear([height constant], 0);
	uiControlSetMinimumSize(uiControl(imageView), 48, -1);
	assertNear([width constant], 48);
	assertNear([height constant], 16);
	uiControlSetMinimumSize(uiControl(imageView), -1, -1);
	assertNear([width constant], 16);
	assertNear([height constant], 16);

	uiControlDestroy(uiControl(imageView));
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(spinboxHasStableNaturalWidthAndHeight),
		cmocka_unit_test(formTracksCenteredSpinboxBaseline),
		cmocka_unit_test(minimumSizeBeforeAndAfterInsertionDoesNotConflict),
		cmocka_unit_test(formUsesOptionalPreferredHeight),
		cmocka_unit_test(radioButtonsUseStackHuggingAndRestoreIt),
		cmocka_unit_test(gridKeepsNonexpandingAxisAtFittingSize),
		cmocka_unit_test(imageViewDefaultMinimumCanBeOverriddenAndRestored),
	};
	NSAutoreleasePool *pool;
	uiInitOptions options = {0};
	const char *error;
	int result;

	pool = [NSAutoreleasePool new];
	error = uiInit(&options);
	if (error != NULL) {
		fprintf(stderr, "error initializing ui: %s\n", error);
		uiFreeInitError(error);
		[pool drain];
		return 1;
	}
	result = cmocka_run_group_tests_name("Darwin layout", tests, NULL, NULL);
	uiUninit();
	[pool drain];
	return result;
}
