# Receiving drag and drop

`uiWindowSetDropHandler()` and `uiAreaSetDropHandler()` receive UTF-8 text and
filesystem paths using Copy. Copy describes the transfer protocol; libui does
not copy or open the files. Sending, Move/Link, arbitrary MIME data, virtual
files, file promises, images and automatic edge scrolling are outside this API.

```c
static uiDropResult receive(const uiDropInfo *info,
    const uiDropData *data, void *userData)
{
    if (uiDropDataFormat(data) == uiDropFormatFiles) {
        for (size_t i = 0; i < uiDropDataFileCount(data); i++)
            printf("%s\n", uiDropDataFilePath(data, i));
    } else {
        printf("%s\n", uiDropDataText(data));
    }
    return uiDropAccepted;
}

uiDropHandler handler = {0};
handler.Size = sizeof handler;
handler.Drop = receive;
uiDropStatus status = uiWindowSetDropHandler(window,
    uiDropFormatFiles | uiDropFormatText, &handler, NULL);
/* Check status == uiDropStatusOK before assuming registration succeeded. */
```

The handler is copied. `Drop` is required; `Over`, `Leave` and `ReleaseUserData`
are optional. A missing `Over` accepts compatible Copy offers. `Over` sees
metadata only; returning `uiDropActionNone` rejects the offer. `Leave` ends
hover feedback and can precede a successful Drop, including on GTK.

All calls and callbacks run on the UI thread. Info, payloads and their strings
are borrowed until the callback returns. Use `uiDropDataCopyText()` with
`uiFreeText()`, or `uiDropDataCopyFiles()` with `uiFreeTextArray()`, for longer
storage; free these on the UI thread before `uiUninit()`. For worker threads,
copy into application/runtime-owned memory instead. Getters perform no native
receive operations. Empty text is valid and has a non-NULL text pointer.

Files take precedence when both enabled formats are offered. Conversion failure
rejects the whole representation; it does not try text or another destination.
Normalized payloads are limited to 64 MiB including string terminators and
65,536 paths. Text must be valid UTF-8 without embedded NULs. Unix paths retain
the filesystem's byte representation, which need not be UTF-8. Paths may name
directories, symlinks or mounted network files; libui does not inspect them.
GTK rejects URI lists containing unconvertible paths. GTK and macOS reject
remote-host file URLs rather than treating their path components as local paths.
Windows receives CF_HDROP and CF_UNICODETEXT. macOS receives file URLs and
native string pasteboard data.

Window destinations cover the ordinary content region where a native child
destination does not handle the drag. Existing Entry/TextView text receivers
keep their native behavior; native child rejection is not guaranteed to fall
back to the Window. Title bars, menus, toolbars and independent popups are
outside the guaranteed receiving region. A registered Area is an explicit drop
zone: rejection does not retry the parent. The native adapters register both
known format categories and filter them through the common registration.

Area coordinates match `uiAreaMouseEvent`, including document scrolling.
Window coordinates start at the top left and use `uiWindowContentSize()` units
(Windows client pixels, macOS points, GTK widget units). GTK captures the drop
position before requesting data; subsequent scrolling does not relocate it.

## Registration lifetime

Remove a registration with `uiWindowSetDropHandler(window, 0, NULL, NULL)` or
the corresponding Area function. Removal is idempotent. Invalid arguments or
native registration failure preserve the previous registration. Only complete
handler fields covered by `Size` are read. The structure must cover `Drop`;
missing optional fields are treated as NULL, and trailing future fields are
ignored.

Replacement/removal stops new callbacks to the retired registration. An active
callback can finish, and `ReleaseUserData` runs exactly once afterwards. Failed
registrations never call `ReleaseUserData`. An old asynchronous receive is
discarded after replacement, removal or destruction. GTK has a 10-second GLib
receive timeout; leave notifications do not cancel pending receives. Native
completion is issued once. Provider calls on Windows/macOS can block inside
the OS; this API does not make those operations asynchronous.

A callback may replace/remove its own registration or destroy its control or
parent. An Over that does so rejects the native offer. A Drop that consumed its
payload may destroy its owner and still return `uiDropAccepted`. Do not let
language exceptions unwind across callback boundaries.

GC bindings must retain callback closures and userdata explicitly until
`ReleaseUserData` runs. C function pointers do not root closures. Use a separate
holder per registration and keep the release trampoline alive throughout its
own call. Copy payloads into language-owned strings/arrays inside Drop.

## Example and checks

Build the `drop` example to try Window fallback, an explicit Area, hover
feedback and a native text entry. It prints dropped paths and displays text.

`drop-unit` exercises the real common callback/deferred-destruction code,
registration failures, shortened handlers, stale generations, self-replacement,
self-removal, self-destruction, UTF-8/NUL validation, size/count bounds and
independent copies. It runs without a display. `drop-darwin` uses real AppKit
pasteboards and the Window/Area destination methods, covering text, multiple
paths, mixed URL rejection, Copy negotiation, scrolling coordinates and
callback destruction, and removal/destruction from a lazy pasteboard provider.
It requires access to the macOS pasteboard service.
These tests do not simulate a cross-process pointer drag.

`drop-gtk` exercises the actual GTK adapter's GLib timeout, pending-transfer
cleanup, hover state across Leave, self-removal during Drop and late responses
after registration replacement. Protocol completion
is replaced by a counter, so it can assert exactly-once completion; this is
not a native pointer-routing test. It is registered in Linux CMake builds and
can also be compiled against GTK 3 on macOS to exercise this state management.

`drop-windows` checks bounded CF_HDROP parsing for wide/ANSI paths, long names,
invalid offsets, missing terminators, malformed UTF-16 and independent copies.
It runs on Windows; cross-compiling it does not verify runtime behavior.

Before release, exercise native pointer routing on Windows/macOS and GTK
X11/Wayland, cross-process providers, child controls, disabled/hidden targets,
margin/toolbars, reparenting, modal reentry, DPI/Retina, source failure, GTK late
responses/timeouts and GC bindings. The supplied design review's native GTK
probe predates this implementation and is not an integration test of it.
