#ifndef UIPRIV_DARWIN_DROP_H
#define UIPRIV_DARWIN_DROP_H
#include "../common/drop.h"
extern uiprivDropEndpoint *uiprivDarwinDropNew(uiControl *, id);
extern NSDragOperation uiprivDarwinDropOver(uiprivDropEndpoint *, id<NSDraggingInfo>, NSView *, BOOL);
extern void uiprivDarwinDropLeave(uiprivDropEndpoint *, id<NSDraggingInfo>);
extern BOOL uiprivDarwinDropPrepare(uiprivDropEndpoint *, id<NSDraggingInfo>);
extern BOOL uiprivDarwinDropPerform(uiprivDropEndpoint *, id<NSDraggingInfo>, NSView *, BOOL);
#endif
