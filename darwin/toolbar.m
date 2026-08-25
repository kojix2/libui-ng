#import "uipriv_darwin.h"
#include "../common/toolbar.h"

@interface uiprivToolbarDelegate : NSObject<NSToolbarDelegate> {
@public
	uiToolbar *toolbar;
	NSMutableArray *identifiers;
}
- (id)initWithToolbar:(uiToolbar *)t;
- (IBAction)itemClicked:(id)sender;
@end

static uiToolbarDisplayMode effectiveToolbarDisplayMode(uiToolbarDisplayMode mode)
{
	/* Horizontal labels are not a native macOS toolbar convention. */
	if (mode == uiToolbarDisplayModeIconAndTextHorizontal)
		return uiToolbarDisplayModeIconAndTextVertical;
	return mode;
}

static NSToolbarDisplayMode nativeToolbarDisplayMode(uiToolbarDisplayMode mode)
{
	switch (effectiveToolbarDisplayMode(mode)) {
	case uiToolbarDisplayModeIconOnly:
		return NSToolbarDisplayModeIconOnly;
	case uiToolbarDisplayModeIconAndTextHorizontal:
	case uiToolbarDisplayModeIconAndTextVertical:
		return NSToolbarDisplayModeIconAndLabel;
	case uiToolbarDisplayModeTextOnly:
		return NSToolbarDisplayModeLabelOnly;
	}
	return NSToolbarDisplayModeIconAndLabel;
}

static int customButtonShowsText(uiToolbarItem *item)
{
	return item->icon == NULL ||
		effectiveToolbarDisplayMode(item->toolbar->displayMode) ==
			uiToolbarDisplayModeTextOnly;
}

static void syncCustomButtonContent(uiToolbarItem *item, NSButton *button,
	NSString *text)
{
	if (customButtonShowsText(item)) {
		[button setTitle:text];
		[button setImage:nil];
		[button setImagePosition:NSNoImage];
		return;
	}
	[button setTitle:@""];
	[button setImage:uiprivImageNSImage(item->icon)];
	[button setImagePosition:NSImageOnly];
}

static NSString *itemIdentifier(uiToolbarItem *item, size_t index)
{
	switch (item->type) {
	case uiprivToolbarItemSpace:
		return NSToolbarSpaceItemIdentifier;
	case uiprivToolbarItemFlexibleSpace:
		return NSToolbarFlexibleSpaceItemIdentifier;
	default:
		return [NSString stringWithFormat:@"org.libui-ng.toolbar.%p.%lu",
			(void *) item->toolbar, (unsigned long) index];
	}
}

static int itemNeedsCustomButton(uiToolbarItem *item)
{
	if (item->type == uiprivToolbarItemToggleButton)
		return 1;
	return item->type == uiprivToolbarItemButton &&
		effectiveToolbarDisplayMode(item->toolbar->displayMode) ==
			uiToolbarDisplayModeIconOnly;
}

static void syncItem(uiToolbarItem *item)
{
	NSToolbarItem *native = (NSToolbarItem *) item->native;
	NSString *text;
	int customButton;

	if (native == nil)
		return;
	text = uiprivToNSString(item->text);
	customButton = itemNeedsCustomButton(item);
	[native setLabel:customButton && customButtonShowsText(item) ? @"" : text];
	[native setPaletteLabel:text];
	[native setToolTip:item->tooltip[0] == '\0' ? nil : uiprivToNSString(item->tooltip)];
	[native setImage:uiprivImageNSImage(item->icon)];
	[native setEnabled:item->enabled != 0];
	if (customButton) {
		NSButton *button = (NSButton *) [native view];
		NSMenuItem *menuItem = [native menuFormRepresentation];

		syncCustomButtonContent(item, button, text);
		[button setToolTip:item->tooltip[0] == '\0' ? nil : uiprivToNSString(item->tooltip)];
		[button setEnabled:item->enabled != 0];
		[button setState:item->checked ? NSOnState : NSOffState];
		[button sizeToFit];
		[native setMinSize:[button frame].size];
		[native setMaxSize:[button frame].size];
		[menuItem setTitle:text];
		[menuItem setToolTip:item->tooltip[0] == '\0' ? nil : uiprivToNSString(item->tooltip)];
		[menuItem setEnabled:item->enabled != 0];
		[menuItem setState:item->checked ? NSOnState : NSOffState];
	}
}

@implementation uiprivToolbarDelegate

- (id)initWithToolbar:(uiToolbar *)t
{
	self = [super init];
	if (self) {
		self->toolbar = t;
		self->identifiers = [[NSMutableArray alloc] init];
	}
	return self;
}

- (void)dealloc
{
	[self->identifiers release];
	[super dealloc];
}

- (NSArray *)toolbarAllowedItemIdentifiers:(NSToolbar *)native
{
	return self->identifiers;
}

- (NSArray *)toolbarDefaultItemIdentifiers:(NSToolbar *)native
{
	return self->identifiers;
}

- (NSArray *)toolbarSelectableItemIdentifiers:(NSToolbar *)native
{
	return [NSArray array];
}

- (NSToolbarItem *)toolbar:(NSToolbar *)native
	itemForItemIdentifier:(NSString *)identifier
	willBeInsertedIntoToolbar:(BOOL)flag
{
	size_t i;

	for (i = 0; i < self->toolbar->len; i++) {
		uiToolbarItem *item = self->toolbar->items[i];
		NSToolbarItem *nativeItem = (NSToolbarItem *) item->native;
		if (nativeItem != nil && [[nativeItem itemIdentifier] isEqualToString:identifier])
			return nativeItem;
	}
	return nil;
}

- (IBAction)itemClicked:(id)sender
{
	NSInteger tag = [sender tag];
	uiToolbarItem *item;
	int checked = 0;

	if (tag < 0 || (size_t) tag >= self->toolbar->len)
		return;
	item = self->toolbar->items[tag];
	if (item->type == uiprivToolbarItemToggleButton) {
		if ([sender isKindOfClass:[NSButton class]])
			checked = [sender state] == NSOnState;
		else {
			checked = !item->checked;
			uiToolbarItemSetChecked(item, checked);
		}
	}
	uiprivToolbarItemClicked(item, checked);
}

@end

void uiprivToolbarPlatformNew(uiToolbar *t)
{
	/* Native objects are created on first attachment. */
}

void uiprivToolbarPlatformFree(uiToolbar *t)
{
	size_t i;

	if (t->native != NULL)
		[(NSToolbar *) t->native release];
	if (t->nativeAux != NULL)
		[(uiprivToolbarDelegate *) t->nativeAux release];
	for (i = 0; i < t->len; i++)
		if (t->items[i]->native != NULL)
			[(NSToolbarItem *) t->items[i]->native release];
}

void uiprivToolbarPlatformAttach(uiToolbar *t, uiWindow *w)
{
	NSToolbar *native;
	uiprivToolbarDelegate *delegate;
	size_t i;

	if (t->native == NULL) {
		delegate = [[uiprivToolbarDelegate alloc] initWithToolbar:t];
		native = [[NSToolbar alloc] initWithIdentifier:
			[NSString stringWithFormat:@"org.libui-ng.toolbar.%p", (void *) t]];
		[native setAllowsUserCustomization:NO];
		[native setAutosavesConfiguration:NO];
		[native setDisplayMode:nativeToolbarDisplayMode(t->displayMode)];
		for (i = 0; i < t->len; i++) {
			uiToolbarItem *item = t->items[i];
			NSString *identifier = itemIdentifier(item, i);
			[delegate->identifiers addObject:identifier];
			if (item->type == uiprivToolbarItemSpace ||
				item->type == uiprivToolbarItemFlexibleSpace)
				continue;
			NSToolbarItem *nativeItem = [[NSToolbarItem alloc] initWithItemIdentifier:identifier];
			item->native = nativeItem;
			[nativeItem setTag:(NSInteger) i];
			[nativeItem setTarget:delegate];
			[nativeItem setAction:@selector(itemClicked:)];
			if (item->type == uiprivToolbarItemSeparator) {
				NSBox *separator = [[NSBox alloc] initWithFrame:NSMakeRect(0, 0, 1, 24)];
				[separator setBoxType:NSBoxSeparator];
				[nativeItem setView:separator];
				[nativeItem setMinSize:NSMakeSize(1, 24)];
				[nativeItem setMaxSize:NSMakeSize(1, 24)];
				[separator release];
			}
			if (itemNeedsCustomButton(item)) {
				NSButton *button = [[NSButton alloc] initWithFrame:NSZeroRect];
				NSMenuItem *menuItem;

				[button setButtonType:item->type == uiprivToolbarItemToggleButton ?
					NSPushOnPushOffButton : NSMomentaryPushInButton];
				[button setBezelStyle:NSTexturedRoundedBezelStyle];
				[button setTag:(NSInteger) i];
				[button setTarget:delegate];
				[button setAction:@selector(itemClicked:)];
				[nativeItem setView:button];
				[button release];
				menuItem = [[NSMenuItem alloc] initWithTitle:@""
					action:@selector(itemClicked:) keyEquivalent:@""];
				[menuItem setTag:(NSInteger) i];
				[menuItem setTarget:delegate];
				[nativeItem setMenuFormRepresentation:menuItem];
				[menuItem release];
			}
			syncItem(item);
		}
		[native setDelegate:delegate];
		t->native = native;
		t->nativeAux = delegate;
	}
	[(NSWindow *) uiControlHandle(uiControl(w)) setToolbar:(NSToolbar *) t->native];
}

void uiprivToolbarPlatformDetach(uiToolbar *t, uiWindow *w)
{
	NSWindow *window = (NSWindow *) uiControlHandle(uiControl(w));
	if ([window toolbar] == (NSToolbar *) t->native)
		[window setToolbar:nil];
}

void uiprivToolbarPlatformSyncItem(uiToolbarItem *item)
{
	syncItem(item);
}

void uiprivToolbarPlatformSyncItemIcon(uiToolbarItem *item)
{
	syncItem(item);
}

char *uiprivToolbarPlatformDupText(const char *text)
{
	return uiDarwinNSStringToText(uiprivToNSString(text));
}
