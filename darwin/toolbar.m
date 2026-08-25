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

static void syncItem(uiToolbarItem *item)
{
	NSToolbarItem *native = (NSToolbarItem *) item->native;
	NSString *text;

	if (native == nil)
		return;
	text = uiprivToNSString(item->text);
	[native setLabel:text];
	[native setPaletteLabel:text];
	[native setToolTip:item->tooltip[0] == '\0' ? nil : uiprivToNSString(item->tooltip)];
	[native setImage:uiprivImageNSImage(item->icon)];
	[native setEnabled:item->enabled != 0];
	if (item->type == uiprivToolbarItemToggleButton) {
		NSButton *button = (NSButton *) [native view];
		[button setTitle:text];
		[button setToolTip:item->tooltip[0] == '\0' ? nil : uiprivToNSString(item->tooltip)];
		[button setImage:uiprivImageNSImage(item->icon)];
		[button setEnabled:item->enabled != 0];
		[button setState:item->checked ? NSOnState : NSOffState];
		[button sizeToFit];
		[native setMinSize:[button frame].size];
		[native setMaxSize:[button frame].size];
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
	if (item->type == uiprivToolbarItemToggleButton)
		checked = [(NSButton *) sender state] == NSOnState;
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
		[native setDisplayMode:NSToolbarDisplayModeIconAndLabel];
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
			if (item->type == uiprivToolbarItemToggleButton) {
				NSButton *button = [[NSButton alloc] initWithFrame:NSZeroRect];
				[button setButtonType:NSPushOnPushOffButton];
				[button setBezelStyle:NSTexturedRoundedBezelStyle];
				[button setTag:(NSInteger) i];
				[button setTarget:delegate];
				[button setAction:@selector(itemClicked:)];
				[nativeItem setView:button];
				[button release];
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
