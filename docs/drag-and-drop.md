# Receiving drag and drop

This API is experimental and may change. Window and Area destinations currently
receive UTF-8 text and filesystem paths using Copy; libui does not open or copy
the files themselves.

```c
static uiDropResult receive(const uiDropInfo *info,
    const uiDropData *data, void *userData)
{
    if (data->Format == uiDropFormatFiles) {
        for (size_t i = 0; i < data->FileCount; i++)
            printf("%s\n", data->Files[i]);
    } else {
        printf("%s\n", data->Text);
    }
    return uiDropAccepted;
}

uiDropHandler handler = {0};
handler.Size = sizeof handler;
handler.Drop = receive;
uiDropStatus status = uiWindowSetDropHandler(window,
    uiDropFormatFiles | uiDropFormatText, &handler, NULL);
/* Check status before assuming registration succeeded. */
```

Use `uiAreaSetDropHandler()` for an Area. Window destinations cover content
not handled by native child destinations; a registered Area is an exclusive
drop zone. Area coordinates match mouse events, including scrolling. Window
coordinates start at the top left in `uiWindowContentSize()` units.

All calls and callbacks run on the UI thread. Data, strings and arrays are
borrowed until the callback returns. To retain them, copy into application or
language-runtime memory, or use `uiDropDataCopyText()` / `uiDropDataCopyFiles()`
and release the copies with `uiFreeText()` / `uiFreeTextArray()` on the UI
thread before `uiUninit()`. A structure copy does not retain its pointers.
Unix paths may contain non-UTF-8 bytes.

`Drop` is required. Optional `Over` sees metadata only and can reject an offer;
without it, compatible Copy offers are accepted. Files takes precedence over
Text when both are enabled and offered. Conversion failure rejects the drop.
Optional `Leave` ends hover feedback and may run before a successful Drop.

The handler is copied. Remove it by passing `(0, NULL, NULL)` to the setter.
Failed registration preserves the previous handler. Replacement, removal or
destruction stops new callbacks and discards pending transfers for the old
registration; an executing callback may finish.

Bindings should copy data into language-owned values before calling application
code. Retain callback closures and userdata until `ReleaseUserData`, which runs
once per successful registration after retirement and active callbacks return.
Keep the release trampoline alive until it returns, and prevent language
exceptions from unwinding through C callbacks.

See [ui.h](../ui.h) for field details and [examples/drop.c](../examples/drop.c)
for Window, Area and hover feedback usage.
