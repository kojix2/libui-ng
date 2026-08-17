// 11 june 2016
#import "uipriv_darwin.h"

#include <limits.h>
#include <stdint.h>

// NSGridView uses a dense matrix internally. Keep hostile sparse coordinates
// from turning a small logical grid into an uncontrolled allocation.
enum {
	maxGridRows = 10000,
	maxGridColumns = 10000,
	maxGridCells = 100000,
};

@interface gridChild : NSObject
@property uiControl *control;
@property int left;
@property int top;
@property int xspan;
@property int yspan;
@property BOOL hexpand;
@property uiAlign halign;
@property BOOL vexpand;
@property uiAlign valign;
@property NSLayoutPriority oldHorizontalHuggingPriority;
@property NSLayoutPriority oldVerticalHuggingPriority;
- (NSView *)view;
@end

@interface gridView : NSView {
	uiGrid *grid;
	NSMutableArray *children;
	NSGridView *nativeGrid;
	NSMutableArray *nativeGridConstraints;
	BOOL padded;
	BOOL horizontalExpansion;
	BOOL verticalExpansion;
}
- (id)initWithGrid:(uiGrid *)g;
- (void)onDestroy;
- (void)syncEnableStates:(int)enabled;
- (void)rebuild;
- (void)append:(gridChild *)child;
- (void)insert:(gridChild *)child after:(uiControl *)existing at:(uiAt)at;
- (int)isPadded;
- (void)setPadded:(int)p;
- (BOOL)hugsTrailing;
- (BOOL)hugsBottom;
@end

struct uiGrid {
	uiDarwinControl c;
	gridView *view;
};

@implementation gridChild

- (NSView *)view
{
	return (NSView *) uiControlHandle(self.control);
}

@end


static NSGridCellPlacement placement(uiAlign align)
{
	switch (align) {
	case uiAlignFill:
		return NSGridCellPlacementFill;
	case uiAlignStart:
		return NSGridCellPlacementLeading;
	case uiAlignCenter:
		return NSGridCellPlacementCenter;
	case uiAlignEnd:
		return NSGridCellPlacementTrailing;
	}
	uiprivUserBug("Invalid uiAlign value %d.", align);
	return NSGridCellPlacementFill;
}

static void validateSpan(int span, const char *name)
{
	if (span < 1)
		uiprivUserBug("uiGrid %s must be at least 1.", name);
}

static int checkedEnd(int origin, int span, const char *axis)
{
	int64_t end;

	end = ((int64_t) origin) + span;
	if (end > INT_MAX)
		uiprivUserBug("uiGrid %s coordinate and span overflow.", axis);
	return (int) end;
}

static BOOL rangesOverlap(int astart, int aend, int bstart, int bend)
{
	return astart < bend && bstart < aend;
}

static void addConstraint(NSView *owner, id first, NSLayoutAttribute firstAttribute,
	NSLayoutRelation relation, id second, NSLayoutAttribute secondAttribute,
	NSString *description)
{
	[owner addConstraint:uiprivMkConstraint(first, firstAttribute, relation,
		second, secondAttribute, 1, 0, description)];
}

static NSView *newCellView(gridChild *child)
{
	NSView *container;
	NSView *view;

	container = [[NSView alloc] initWithFrame:NSZeroRect];
	[container setTranslatesAutoresizingMaskIntoConstraints:NO];
	view = [child view];
	uiDarwinControlSetSuperview(uiDarwinControl(child.control), container);

	switch (child.halign) {
	case uiAlignFill:
		addConstraint(container, container, NSLayoutAttributeLeading,
			NSLayoutRelationEqual, view, NSLayoutAttributeLeading,
			@"uiGrid cell fill leading constraint");
		addConstraint(container, container, NSLayoutAttributeTrailing,
			NSLayoutRelationEqual, view, NSLayoutAttributeTrailing,
			@"uiGrid cell fill trailing constraint");
		break;
	case uiAlignStart:
		addConstraint(container, container, NSLayoutAttributeLeading,
			NSLayoutRelationEqual, view, NSLayoutAttributeLeading,
			@"uiGrid cell leading alignment constraint");
		addConstraint(container, container, NSLayoutAttributeTrailing,
			NSLayoutRelationGreaterThanOrEqual, view, NSLayoutAttributeTrailing,
			@"uiGrid cell trailing bound constraint");
		break;
	case uiAlignCenter:
		addConstraint(container, container, NSLayoutAttributeCenterX,
			NSLayoutRelationEqual, view, NSLayoutAttributeCenterX,
			@"uiGrid cell horizontal center constraint");
		addConstraint(container, container, NSLayoutAttributeLeading,
			NSLayoutRelationLessThanOrEqual, view, NSLayoutAttributeLeading,
			@"uiGrid cell leading bound constraint");
		addConstraint(container, container, NSLayoutAttributeTrailing,
			NSLayoutRelationGreaterThanOrEqual, view, NSLayoutAttributeTrailing,
			@"uiGrid cell trailing bound constraint");
		break;
	case uiAlignEnd:
		addConstraint(container, container, NSLayoutAttributeTrailing,
			NSLayoutRelationEqual, view, NSLayoutAttributeTrailing,
			@"uiGrid cell trailing alignment constraint");
		addConstraint(container, container, NSLayoutAttributeLeading,
			NSLayoutRelationLessThanOrEqual, view, NSLayoutAttributeLeading,
			@"uiGrid cell leading bound constraint");
		break;
	}

	switch (child.valign) {
	case uiAlignFill:
		addConstraint(container, container, NSLayoutAttributeTop,
			NSLayoutRelationEqual, view, NSLayoutAttributeTop,
			@"uiGrid cell fill top constraint");
		addConstraint(container, container, NSLayoutAttributeBottom,
			NSLayoutRelationEqual, view, NSLayoutAttributeBottom,
			@"uiGrid cell fill bottom constraint");
		break;
	case uiAlignStart:
		addConstraint(container, container, NSLayoutAttributeTop,
			NSLayoutRelationEqual, view, NSLayoutAttributeTop,
			@"uiGrid cell top alignment constraint");
		addConstraint(container, container, NSLayoutAttributeBottom,
			NSLayoutRelationGreaterThanOrEqual, view, NSLayoutAttributeBottom,
			@"uiGrid cell bottom bound constraint");
		break;
	case uiAlignCenter:
		addConstraint(container, container, NSLayoutAttributeCenterY,
			NSLayoutRelationEqual, view, NSLayoutAttributeCenterY,
			@"uiGrid cell vertical center constraint");
		addConstraint(container, container, NSLayoutAttributeTop,
			NSLayoutRelationLessThanOrEqual, view, NSLayoutAttributeTop,
			@"uiGrid cell top bound constraint");
		addConstraint(container, container, NSLayoutAttributeBottom,
			NSLayoutRelationGreaterThanOrEqual, view, NSLayoutAttributeBottom,
			@"uiGrid cell bottom bound constraint");
		break;
	case uiAlignEnd:
		addConstraint(container, container, NSLayoutAttributeBottom,
			NSLayoutRelationEqual, view, NSLayoutAttributeBottom,
			@"uiGrid cell bottom alignment constraint");
		addConstraint(container, container, NSLayoutAttributeTop,
			NSLayoutRelationLessThanOrEqual, view, NSLayoutAttributeTop,
			@"uiGrid cell top bound constraint");
		break;
	}
	return container;
}

@implementation gridView

- (id)initWithGrid:(uiGrid *)g
{
	self = [super initWithFrame:NSZeroRect];
	if (self != nil) {
		self->grid = g;
		self->children = [NSMutableArray new];
		self->nativeGridConstraints = [NSMutableArray new];
		self->nativeGrid = nil;
		self->padded = NO;
		self->horizontalExpansion = NO;
		self->verticalExpansion = NO;
	}
	return self;
}

- (void)removeNativeGrid
{
	gridChild *child;

	for (child in self->children)
		uiDarwinControlSetSuperview(uiDarwinControl(child.control), nil);
	if ([self->nativeGridConstraints count] != 0) {
		[self removeConstraints:self->nativeGridConstraints];
		[self->nativeGridConstraints removeAllObjects];
	}
	if (self->nativeGrid != nil) {
		[self->nativeGrid removeFromSuperview];
		[self->nativeGrid release];
		self->nativeGrid = nil;
	}
}

- (void)onDestroy
{
	gridChild *child;

	[self removeNativeGrid];
	for (child in self->children) {
		uiControlSetParent(child.control, NULL);
		uiDarwinControlSetHuggingPriority(uiDarwinControl(child.control),
			child.oldHorizontalHuggingPriority,
			NSLayoutConstraintOrientationHorizontal);
		uiDarwinControlSetHuggingPriority(uiDarwinControl(child.control),
			child.oldVerticalHuggingPriority,
			NSLayoutConstraintOrientationVertical);
		uiControlDestroy(child.control);
	}
	[self->nativeGridConstraints release];
	[self->children release];
}

- (void)syncEnableStates:(int)enabled
{
	gridChild *child;

	for (child in self->children)
		uiDarwinControlSyncEnableState(uiDarwinControl(child.control), enabled);
}

- (void)addNativeGridConstraint:(NSLayoutConstraint *)constraint
{
	[self addConstraint:constraint];
	[self->nativeGridConstraints addObject:constraint];
}

- (void)rebuild
{
	gridChild *child;
	BOOL first;
	int xmin, ymin, xmax, ymax;
	int xcount, ycount;
	int x, y, xx, yy;
	int64_t cellCount;
	CGFloat spacing;
	BOOL *expandedColumns, *expandedRows;
	int *occupancy;
	NSView **columnViews, **rowViews;

	[self removeNativeGrid];
	self->horizontalExpansion = NO;
	self->verticalExpansion = NO;

	first = YES;
	for (child in self->children) {
		int xend, yend;

		if (!uiControlVisible(child.control))
			continue;
		xend = checkedEnd(child.left, child.xspan, "horizontal");
		yend = checkedEnd(child.top, child.yspan, "vertical");
		if (first) {
			xmin = child.left;
			ymin = child.top;
			xmax = xend;
			ymax = yend;
			first = NO;
		} else {
			if (xmin > child.left)
				xmin = child.left;
			if (ymin > child.top)
				ymin = child.top;
			if (xmax < xend)
				xmax = xend;
			if (ymax < yend)
				ymax = yend;
		}
		if (child.hexpand)
			self->horizontalExpansion = YES;
		if (child.vexpand)
			self->verticalExpansion = YES;
	}
	if (first)
		return;

	xcount = xmax - xmin;
	ycount = ymax - ymin;
	cellCount = ((int64_t) xcount) * ycount;
	if (xcount > maxGridColumns || ycount > maxGridRows ||
		cellCount > maxGridCells)
		uiprivUserBug("uiGrid dimensions are too large (%d columns, %d rows).",
			xcount, ycount);
	expandedColumns = (BOOL *) uiprivAlloc(xcount * sizeof (BOOL), "uiGrid expanded columns");
	expandedRows = (BOOL *) uiprivAlloc(ycount * sizeof (BOOL), "uiGrid expanded rows");
	occupancy = (int *) uiprivAlloc(cellCount * sizeof (int), "uiGrid occupancy");
	columnViews = (NSView **) uiprivAlloc(xcount * sizeof (NSView *), "uiGrid column views");
	rowViews = (NSView **) uiprivAlloc(ycount * sizeof (NSView *), "uiGrid row views");
	for (x = 0; x < cellCount; x++)
		occupancy[x] = -1;

	// Non-spanning children select an expandable row or column directly.
	for (child in self->children) {
		if (!uiControlVisible(child.control))
			continue;
		if (child.hexpand && child.xspan == 1)
			expandedColumns[child.left - xmin] = YES;
		if (child.vexpand && child.yspan == 1)
			expandedRows[child.top - ymin] = YES;
	}
	// A spanning child uses existing expandable tracks in its range. If none
	// exist, all tracks in the span share the extra space.
	for (child in self->children) {
		BOOL found;

		if (!uiControlVisible(child.control))
			continue;
		if (child.hexpand && child.xspan > 1) {
			found = NO;
			for (x = child.left - xmin; x < child.left - xmin + child.xspan; x++)
				if (expandedColumns[x])
					found = YES;
			if (!found)
				for (x = child.left - xmin; x < child.left - xmin + child.xspan; x++)
					expandedColumns[x] = YES;
		}
		if (child.vexpand && child.yspan > 1) {
			found = NO;
			for (y = child.top - ymin; y < child.top - ymin + child.yspan; y++)
				if (expandedRows[y])
					found = YES;
			if (!found)
				for (y = child.top - ymin; y < child.top - ymin + child.yspan; y++)
					expandedRows[y] = YES;
		}
	}

	self->nativeGrid = [[NSGridView gridViewWithNumberOfColumns:xcount rows:ycount] retain];
	[self->nativeGrid setTranslatesAutoresizingMaskIntoConstraints:NO];
	spacing = self->padded ? uiDarwinPaddingAmount(NULL) : 0;
	[self->nativeGrid setColumnSpacing:spacing];
	[self->nativeGrid setRowSpacing:spacing];
	[self addSubview:self->nativeGrid];

	[self addNativeGridConstraint:uiprivMkConstraint(self, NSLayoutAttributeLeading,
		NSLayoutRelationEqual, self->nativeGrid, NSLayoutAttributeLeading,
		1, 0, @"uiGrid native leading constraint")];
	[self addNativeGridConstraint:uiprivMkConstraint(self, NSLayoutAttributeTop,
		NSLayoutRelationEqual, self->nativeGrid, NSLayoutAttributeTop,
		1, 0, @"uiGrid native top constraint")];
	[self addNativeGridConstraint:uiprivMkConstraint(self, NSLayoutAttributeTrailing,
		NSLayoutRelationEqual, self->nativeGrid, NSLayoutAttributeTrailing,
		1, 0, @"uiGrid native trailing constraint")];
	[self addNativeGridConstraint:uiprivMkConstraint(self, NSLayoutAttributeBottom,
		NSLayoutRelationEqual, self->nativeGrid, NSLayoutAttributeBottom,
		1, 0, @"uiGrid native bottom constraint")];

	// Merge first, then install content into each top-leading cell.
	for (child in self->children) {
		NSUInteger index;

		if (!uiControlVisible(child.control))
			continue;
		x = child.left - xmin;
		y = child.top - ymin;
		index = [self->children indexOfObjectIdenticalTo:child];
		for (yy = y; yy < y + child.yspan; yy++)
			for (xx = x; xx < x + child.xspan; xx++)
				occupancy[yy * xcount + xx] = (int) index;
		if (child.xspan != 1 || child.yspan != 1)
			[self->nativeGrid
				mergeCellsInHorizontalRange:NSMakeRange(x, child.xspan)
				verticalRange:NSMakeRange(y, child.yspan)];
	}
	for (child in self->children) {
		NSGridCell *cell;
		NSView *container;
		NSLayoutPriority priority;

		if (!uiControlVisible(child.control))
			continue;
		x = child.left - xmin;
		y = child.top - ymin;
		cell = [self->nativeGrid cellAtColumnIndex:x rowIndex:y];
		[cell setXPlacement:NSGridCellPlacementFill];
		[cell setYPlacement:NSGridCellPlacementFill];
		container = newCellView(child);
		[cell setContentView:container];
		if (child.xspan == 1)
			columnViews[x] = container;
		if (child.yspan == 1)
			rowViews[y] = container;
		[container release];

		priority = child.hexpand ? NSLayoutPriorityDefaultLow :
			child.oldHorizontalHuggingPriority;
		uiDarwinControlSetHuggingPriority(uiDarwinControl(child.control),
			priority, NSLayoutConstraintOrientationHorizontal);
		priority = child.vexpand ? NSLayoutPriorityDefaultLow :
			child.oldVerticalHuggingPriority;
		uiDarwinControlSetHuggingPriority(uiDarwinControl(child.control),
			priority, NSLayoutConstraintOrientationVertical);
	}

	// Empty cells can serve as sizing representatives for expandable tracks.
	for (y = 0; y < ycount; y++)
		for (x = 0; x < xcount; x++) {
			NSGridCell *cell;
			NSView *sizingView;

			if (occupancy[y * xcount + x] != -1)
				continue;
			if (!expandedColumns[x] && !expandedRows[y])
				continue;
			sizingView = [[NSView alloc] initWithFrame:NSZeroRect];
			[sizingView setTranslatesAutoresizingMaskIntoConstraints:NO];
			cell = [self->nativeGrid cellAtColumnIndex:x rowIndex:y];
			[cell setXPlacement:NSGridCellPlacementFill];
			[cell setYPlacement:NSGridCellPlacementFill];
			[cell setContentView:sizingView];
			if (columnViews[x] == nil)
				columnViews[x] = sizingView;
			if (rowViews[y] == nil)
				rowViews[y] = sizingView;
			[sizingView release];
		}

	{
		NSView *firstColumnView;
		NSView *firstRowView;

		firstColumnView = nil;
		for (x = 0; x < xcount; x++) {
			NSLayoutConstraint *constraint;

			if (!expandedColumns[x] || columnViews[x] == nil)
				continue;
			[columnViews[x] setContentHuggingPriority:NSLayoutPriorityDefaultLow
				forOrientation:NSLayoutConstraintOrientationHorizontal];
			if (firstColumnView == nil) {
				firstColumnView = columnViews[x];
				continue;
			}
			constraint = uiprivMkConstraint(firstColumnView, NSLayoutAttributeWidth,
				NSLayoutRelationEqual, columnViews[x], NSLayoutAttributeWidth,
				1, 0, @"uiGrid expandable column equality constraint");
			[constraint setPriority:NSLayoutPriorityDefaultHigh];
			[self->nativeGrid addConstraint:constraint];
		}

		firstRowView = nil;
		for (y = 0; y < ycount; y++) {
			NSLayoutConstraint *constraint;

			if (!expandedRows[y] || rowViews[y] == nil)
				continue;
			[rowViews[y] setContentHuggingPriority:NSLayoutPriorityDefaultLow
				forOrientation:NSLayoutConstraintOrientationVertical];
			if (firstRowView == nil) {
				firstRowView = rowViews[y];
				continue;
			}
			constraint = uiprivMkConstraint(firstRowView, NSLayoutAttributeHeight,
				NSLayoutRelationEqual, rowViews[y], NSLayoutAttributeHeight,
				1, 0, @"uiGrid expandable row equality constraint");
			[constraint setPriority:NSLayoutPriorityDefaultHigh];
			[self->nativeGrid addConstraint:constraint];
		}
	}

	uiprivFree(rowViews);
	uiprivFree(columnViews);
	uiprivFree(occupancy);
	uiprivFree(expandedRows);
	uiprivFree(expandedColumns);
}

- (void)validateChild:(gridChild *)candidate
{
	gridChild *child;
	int candidateRight, candidateBottom;

	validateSpan(candidate.xspan, "xspan");
	validateSpan(candidate.yspan, "yspan");
	candidateRight = checkedEnd(candidate.left, candidate.xspan, "horizontal");
	candidateBottom = checkedEnd(candidate.top, candidate.yspan, "vertical");
	for (child in self->children) {
		int right, bottom;

		right = checkedEnd(child.left, child.xspan, "horizontal");
		bottom = checkedEnd(child.top, child.yspan, "vertical");
		if (rangesOverlap(candidate.left, candidateRight, child.left, right) &&
			rangesOverlap(candidate.top, candidateBottom, child.top, bottom))
			uiprivUserBug("Controls in a uiGrid cannot overlap.");
	}
}

- (void)append:(gridChild *)child
{
	BOOL oldHorizontalExpansion, oldVerticalExpansion;

	[self validateChild:child];
	oldHorizontalExpansion = self->horizontalExpansion;
	oldVerticalExpansion = self->verticalExpansion;
	uiControlSetParent(child.control, uiControl(self->grid));
	uiDarwinControlSyncEnableState(uiDarwinControl(child.control),
		uiControlEnabledToUser(uiControl(self->grid)));
	[self->children addObject:child];
	[child release];
	[self rebuild];
	if (oldHorizontalExpansion != self->horizontalExpansion ||
		oldVerticalExpansion != self->verticalExpansion)
		uiDarwinNotifyEdgeHuggingChanged(uiDarwinControl(self->grid));
}

- (void)insert:(gridChild *)child after:(uiControl *)existing at:(uiAt)at
{
	gridChild *other;
	int64_t left, top;

	other = nil;
	for (other in self->children)
		if (other.control == existing)
			break;
	if (other == nil || other.control != existing)
		uiprivUserBug("Existing control %p is not in grid %p.", existing, self->grid);

	left = other.left;
	top = other.top;
	switch (at) {
	case uiAtLeading:
		left -= child.xspan;
		break;
	case uiAtTop:
		top -= child.yspan;
		break;
	case uiAtTrailing:
		left += other.xspan;
		break;
	case uiAtBottom:
		top += other.yspan;
		break;
	default:
		uiprivUserBug("Invalid uiAt value %d.", at);
	}
	if (left < INT_MIN || left > INT_MAX || top < INT_MIN || top > INT_MAX)
		uiprivUserBug("uiGrid insertion coordinate overflow.");
	child.left = (int) left;
	child.top = (int) top;
	[self append:child];
}

- (int)isPadded
{
	return self->padded;
}

- (void)setPadded:(int)p
{
	CGFloat spacing;

	self->padded = p != 0;
	if (self->nativeGrid == nil)
		return;
	spacing = self->padded ? uiDarwinPaddingAmount(NULL) : 0;
	[self->nativeGrid setColumnSpacing:spacing];
	[self->nativeGrid setRowSpacing:spacing];
}

- (BOOL)hugsTrailing
{
	return !self->horizontalExpansion;
}

- (BOOL)hugsBottom
{
	return !self->verticalExpansion;
}

@end

static void uiGridDestroy(uiControl *c)
{
	uiGrid *g = uiGrid(c);

	[g->view onDestroy];
	[g->view release];
	uiFreeControl(uiControl(g));
}

uiDarwinControlDefaultHandle(uiGrid, view)
uiDarwinControlDefaultParent(uiGrid, view)
uiDarwinControlDefaultSetParent(uiGrid, view)
uiDarwinControlDefaultToplevel(uiGrid, view)
uiDarwinControlDefaultVisible(uiGrid, view)
uiDarwinControlDefaultShow(uiGrid, view)
uiDarwinControlDefaultHide(uiGrid, view)
uiDarwinControlDefaultEnabled(uiGrid, view)
uiDarwinControlDefaultEnable(uiGrid, view)
uiDarwinControlDefaultDisable(uiGrid, view)

static void uiGridSyncEnableState(uiDarwinControl *c, int enabled)
{
	uiGrid *g = uiGrid(c);

	if (uiDarwinShouldStopSyncEnableState(uiDarwinControl(g), enabled))
		return;
	[g->view syncEnableStates:enabled];
}

uiDarwinControlDefaultSetSuperview(uiGrid, view)

static BOOL uiGridHugsTrailingEdge(uiDarwinControl *c)
{
	uiGrid *g = uiGrid(c);

	return [g->view hugsTrailing];
}

static BOOL uiGridHugsBottom(uiDarwinControl *c)
{
	uiGrid *g = uiGrid(c);

	return [g->view hugsBottom];
}

static void uiGridChildEdgeHuggingChanged(uiDarwinControl *c)
{
	uiGrid *g = uiGrid(c);

	[g->view rebuild];
}

uiDarwinControlDefaultHuggingPriority(uiGrid, view)
uiDarwinControlDefaultSetHuggingPriority(uiGrid, view)

static void uiGridChildVisibilityChanged(uiDarwinControl *c)
{
	uiGrid *g = uiGrid(c);
	BOOL oldHorizontalExpansion, oldVerticalExpansion;

	oldHorizontalExpansion = ![g->view hugsTrailing];
	oldVerticalExpansion = ![g->view hugsBottom];
	[g->view rebuild];
	if (oldHorizontalExpansion != ![g->view hugsTrailing] ||
		oldVerticalExpansion != ![g->view hugsBottom])
		uiDarwinNotifyEdgeHuggingChanged(uiDarwinControl(g));
}

static gridChild *newGridChild(uiControl *control, int xspan, int yspan,
	int hexpand, uiAlign halign, int vexpand, uiAlign valign)
{
	gridChild *child;

	if (control == NULL)
		uiprivUserBug("You cannot add NULL to a uiGrid.");
	validateSpan(xspan, "xspan");
	validateSpan(yspan, "yspan");
	placement(halign);
	placement(valign);
	child = [gridChild new];
	child.control = control;
	child.xspan = xspan;
	child.yspan = yspan;
	child.hexpand = hexpand != 0;
	child.halign = halign;
	child.vexpand = vexpand != 0;
	child.valign = valign;
	child.oldHorizontalHuggingPriority = uiDarwinControlHuggingPriority(
		uiDarwinControl(control), NSLayoutConstraintOrientationHorizontal);
	child.oldVerticalHuggingPriority = uiDarwinControlHuggingPriority(
		uiDarwinControl(control), NSLayoutConstraintOrientationVertical);
	return child;
}

void uiGridAppend(uiGrid *g, uiControl *c, int left, int top, int xspan,
	int yspan, int hexpand, uiAlign halign, int vexpand, uiAlign valign)
{
	gridChild *child;

	child = newGridChild(c, xspan, yspan, hexpand, halign, vexpand, valign);
	child.left = left;
	child.top = top;
	[g->view append:child];
}

void uiGridInsertAt(uiGrid *g, uiControl *c, uiControl *existing, uiAt at,
	int xspan, int yspan, int hexpand, uiAlign halign, int vexpand,
	uiAlign valign)
{
	gridChild *child;

	child = newGridChild(c, xspan, yspan, hexpand, halign, vexpand, valign);
	[g->view insert:child after:existing at:at];
}

int uiGridPadded(uiGrid *g)
{
	return [g->view isPadded];
}

void uiGridSetPadded(uiGrid *g, int padded)
{
	[g->view setPadded:padded];
}

uiGrid *uiNewGrid(void)
{
	uiGrid *g;

	uiDarwinNewControl(uiGrid, g);
	g->view = [[gridView alloc] initWithGrid:g];
	return g;
}
