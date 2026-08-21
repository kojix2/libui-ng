# Plan: Control Lifetime and Destruction

**Status:** proposal. Step 0 (deferred destruction) exists as uncommitted work in the
tree; steps 1–5 are not started.

**Audience:** anyone touching control destruction in libui-ng, and anyone maintaining a
language binding on top of it.

---

## TL;DR

`uiControlDestroy()` used to be unsafe to call from inside a user callback. The
in-tree patch fixes that by deferring destruction to a later main-loop turn. That fix
is correct, but it moves cost downstream: language bindings now have to reason about a
window in which a control is "requested to be destroyed but still alive and still
delivering events".

This plan keeps the deferral (it is the right core) and adds three pieces that remove
the downstream cost, then optionally moves to synchronous logical destruction later.

| Step | What | Breaks ABI | Scope |
|---|---|---|---|
| 0 | Deferred destruction (in tree) | no | done |
| 1 | `uiControlOnDestroyed()` | no | 1 hook site |
| 2 | Suppress callbacks for doomed controls | no | 92 call sites, mechanical |
| 3 | Ordered deferred free for `uiTableModel` / `uiImage` | no | ~50 lines, one file |
| 4 | `uiGridDelete()` | no | 3 implementations |
| 5 | Automatic detach from parent (optional, later) | **yes** | 1 vtable entry + ~21 implementations |

Steps 1–4 are additive. Step 5 requires breaking the `uiControl` vtable and should be
batched with whatever other ABI changes happen before the API is versioned.

---

## 1. Background: the crash class this is about

### 1.1 The shape of the bug

Every libui backend dispatches native events by calling a user-supplied function
pointer and then continuing to do work:

```c
// windows/window.cpp, WM_WINDOWPOSCHANGED (simplified, pre-patch)
(*(w->onPositionChanged))(w, w->onPositionChangedData);
...
windowRelayout(w);          // <-- w may already be freed
```

If the user's callback calls `uiControlDestroy(uiControl(w))`, the control is freed
synchronously. Everything the backend touches afterwards — `w`, `w->hwnd`, the
enclosing `NSWindow`, the `GtkWidget` — is a dangling reference.

The same applies one level up, in the *native toolkit*:

```objc
// darwin/button.m
- (IBAction)onClicked:(id)sender    // self is the NSButton; target == self
{
    (*(b->onClicked))(b, b->onClickedData);   // may [b->button release] -> self dealloc'd
}                                              // AppKit then touches self
```

### 1.2 Why the previous mitigation was insufficient

Before the patch, two spots guarded against this with per-object "lifetime" counters:

```c
// windows/table.cpp (removed by the patch)
static std::map<uiTable *, uint64_t> tables;
...
lifetime = tableLifetime(t);
(*(t->onRowClicked))(t, ht.iItem, t->onRowClickedData);
if (lifetime != 0 && tableLifetime(t) != lifetime)
    return TRUE;     // t was destroyed inside the callback
```

There was an equivalent map for `uiWindow` in `windows/window.cpp`. Problems:

- Windows only. darwin and unix had no protection at all.
- Two ad-hoc registries covering two types out of seventeen.
- Safety by discipline: every new "touch the control after a callback" site is a place
  to forget the guard, and forgetting it is a use-after-free.
- The libui documentation's workaround for users was "wrap the destroy in
  `uiQueueMain()`", which is a workaround, not a fix. `examples/control-destroy.c`
  demonstrated exactly that.

---

## 2. Step 0: what the in-tree patch does

### 2.1 Mechanism

A common-layer region marker is placed around every piece of backend work that invokes
a user callback:

```c
uiprivUserCallbackEnter();
(*(b->onClicked))(b, b->onClickedData);
uiprivUserCallbackLeave();
```

`uiprivUserCallbackEnter()`/`Leave()` maintain a depth counter. `uiControlDestroy()`
checks it:

```c
void uiControlDestroy(uiControl *c)
{
    if (c == NULL)
        uiprivUserBug("uiControlDestroy() cannot be called with NULL");
    if (userCallbackDepth != 0) {
        /* append c to a FIFO queue and return */
        return;
    }
    (*(c->Destroy))(c);        /* outside a callback: unchanged, synchronous */
}
```

When the depth returns to zero, a flush is scheduled on the platform main loop —
`dispatch_async_f` on darwin, `gdk_threads_add_idle` on unix, `PostMessageW` to the
utility window on Windows. Each scheduled flush carries a monotonically increasing ID
so that a stale task queued before `uiUninit()` cannot fire against a later `uiInit()`.
If a flush task lands while the depth is nonzero (a nested/modal loop), it returns
without rescheduling; the outermost `Leave()` will schedule a new one.

`uiUninit()` calls `uiprivControlDestroyUninit()`, which flushes synchronously and
asserts that the depth is zero and the queue is empty.

### 2.2 What this buys

- Destruction is **safe by construction**. Nothing is freed while any libui or native
  frame that might touch it is still on the stack. Guards become optimizations, not
  correctness requirements.
- Uniform across all three backends.
- Duplicate destroy requests inside one callback are coalesced. As a side effect,
  calling `uiControlDestroy(w)` inside `onClosing` is now safe: libui's own
  `if (onClosing()) uiControlDestroy(w)` collapses into the user's queued request, and
  the window is destroyed exactly once.
- `uiQueueMain()` is no longer required as a workaround.

### 2.3 Measured properties

| Property | Value |
|---|---|
| Exported symbols before / after | 315 / 315, zero difference |
| `uipriv*` symbols exported | 0 (hidden by `gnu_symbol_visibility: 'hidden'`) |
| Public header changes | 4 Doxygen comment lines in `ui.h`; no declarations |
| ABI compatibility | compatible — an existing binary keeps linking |
| Source compatibility | compatible — no signature or struct change |
| Unit tests | pass (static build; the deferred-destroy tests need internal symbols) |

The change is therefore **binary and source compatible**. Everything that follows is
about *behavioural* compatibility.

---

## 3. Why step 0 is not enough

### 3.1 The gap

Between the moment `uiControlDestroy(c)` is requested and the moment the flush runs,
`c` is fully alive: attached to its parent, visible on screen, and reachable by the
native event system. Three consequences.

**Hazard A — callbacks continue to fire.**

```c
static int onClosing(uiWindow *w, void *data) {
    free(appState); appState = NULL;
    uiControlDestroy(uiControl(w));
    return 1;
}
static void onSizeChanged(uiWindow *w, void *data) {
    use(appState);          /* previously unreachable; now reachable */
}
```

Windows guards six spots with `uiprivControlDestroyPending()`; darwin and unix guard
none. The risk is strongly type-dependent:

| Risk | Controls | Why |
|---|---|---|
| High | `uiArea` `Draw`, `uiTable` model handler | fire on every repaint |
| Medium | `uiWindow` geometry / focus callbacks | fire when focus moves |
| Negligible | `uiButton`, `uiCheckbox`, `uiEntry`, … | need human input; nobody clicks twice within one main-loop turn |

**Hazard B — resource free ordering breaks, loudly.**

```c
/* inside a callback */
uiControlDestroy(uiControl(table));   /* deferred; table stays registered with model */
uiFreeTableModel(model);              /* aborts */
```

`uiFreeTableModel()` refuses to run while tables are registered
(`windows/table.cpp:23`, `unix/tablemodel.c:244`, `darwin/table.m:184`) and reports it
with `uiprivUserBug()`, which reaches `__builtin_trap()` on darwin, `G_BREAKPOINT()` on
unix and `DebugBreak()` on Windows. This sequence worked before the patch. It now
crashes.

The silent variant: `uiFreeImage()` has no such guard, so freeing an image after
destroying the table that displays it becomes a use-after-free on the next repaint.

**Hazard C — the control stays visible.**

The flush is scheduled at the outermost `Leave()`. If a modal loop runs at nonzero
depth, no flush is even pending:

```c
static void onClicked(uiButton *b, void *d) {   /* depth 1 */
    uiControlDestroy(uiControl(win));            /* queued, nothing scheduled yet */
    uiMsgBox(other, "Done", "...");              /* modal loop, still depth 1 */
}
```

`win` stays on screen for the entire duration of the message box.

### 3.2 What this costs a binding

The cost is not theoretical. It is visible in the memory-management documentation that
downstream bindings have to write. Here is the current UIng (Crystal) policy,
paraphrased, annotated with the libui gap that forces each line:

| Rule the binding must document | Root cause in libui |
|---|---|
| Destroying a parent destroys children; **the binding marks child wrappers destroyed** | there is no way to observe destruction, so the binding must mirror the ownership tree |
| Guard mixed exit paths with `released?` | same; plus destroy is not idempotent |
| Do not use a wrapper after destroy | same; no way to invalidate automatically |
| Do not call `window.destroy` inside `on_closing` | **fixed by step 0** (see §2.2) |
| Do not destroy a child that still has a parent | `uiControlDestroy()` does not detach; violating it traps |
| `uiGrid` has no delete API | `uiGridDelete()` is simply not implemented |
| Destroy all tables before `model.free` | manual ordering, enforced by an abort (hazard B) |
| Keep an `uiImage` alive while a table may display it | `uiTableValue` borrows the pointer; no refcount |
| `uiTableSelection` ownership depends on where you got it | the same type has two ownership rules |
| Use `destroy` for controls and `free` for everything else | two vocabularies; eleven distinct `uiFree*` entry points |

Three of the top four rows have the same root cause: **destruction cannot be observed.**

---

## 4. Design space

Four options were considered.

### Option 1 — status quo (step 0 only), document the gap

Add a sentence to `ui.h` saying callbacks may still arrive, and let bindings cope.
This is what the in-tree patch currently does. Cheapest, but it makes libui harder to
bind, which is the opposite of the project's goal.

### Option 2 — keep destruction synchronous, harden the backends

Rely on toolkit refcounting: `[[self retain] autorelease]` around Cocoa handlers,
`g_object_ref`/`unref` on GTK (largely already the case — `gtk_widget_destroy()` from a
signal handler is the documented GTK pattern), and per-site guards on Windows where
libui touches a control after a callback.

Rejected. This is the `lifetime`-map approach generalised: safety by discipline across
an unbounded set of sites. Missing one is a use-after-free. Step 0 replaced this
deliberately.

### Option 3 ("Alt A") — two-phase destroy

`uiControlDestroy()` synchronously detaches from the parent, disarms all callbacks,
unregisters from registries and hides the control; only the physical native teardown
and struct free are deferred.

This is the classic shape (Qt `deleteLater()` + `setParent(nullptr)`, GTK
dispose/finalize, Cocoa autorelease). It has the highest ceiling: it closes hazards
A, B and C, keeps container invariants true, and removes the "detach before destroy"
rule.

Cost: a new `uiControl` vtable entry for "remove me from my parent", which **breaks
ABI** (`struct uiControl` is public in `ui.h` and bindings embed it to implement
custom controls), plus per-type implementations across 17 control types × 3 backends.

### Option 4 ("Alt D") — keep deferral, add three central pieces

Keep step 0 unchanged, and add:

1. a destruction notification, so bindings can observe;
2. central suppression of callbacks for controls queued for destruction;
3. the same deferred queue generalised to `uiTableModel` / `uiImage` so free ordering
   stops mattering.

All three are additive. None touches `struct uiControl`. Together they close hazards A
and B, and a one-line addition (`Hide` at request time — `Hide` is already in the
vtable) largely closes hazard C.

---

## 5. Decision

**Adopt option 4 now. Keep option 3 available as a later step.**

Reasoning:

- Option 4 reaches binding-facing parity with option 3 for everything except
  *automatic detach* and *one main-loop turn of stale container state*. A binding that
  invalidates its wrapper on the destruction notification never observes the latter.
- Option 4 does not break ABI and requires no per-type work.
- Option 4's pieces are not throwaway. Specifically, piece 2 (central suppression) is a
  *better* mechanism than option 3's per-type disarm:

  | | central suppression | per-type disarm |
  |---|---|---|
  | new control type added | protected automatically | must remember to write `Disarm` |
  | new callback field added | protected automatically | must remember to add the field |
  | `uiAreaHandler` / `uiTableModelHandler` | same mechanism works | app-owned structs; libui cannot disarm them, needs a flag anyway |
  | subtree coverage | automatic (`uiprivControlDestroyPending()` walks ancestors) | no `Children()` vtable entry; each container must implement recursion |

  So if option 3 is built later, piece 2 **stays** and option 3's ~42 disarm
  implementations are never written.

- The only piece discarded on a later move to option 3 is piece 3 (~50 lines, confined
  to `common/control.c`).

What remains for a later option 3 is therefore just automatic detach: one vtable entry,
six container types × 3 backends = 18 implementations, plus 3 for synchronous
table/model unregistration. Roughly **21 implementations and one ABI break** — not the
60–75 a full option 3 from scratch would need.

### When to do step 5

`meson.build` still carries:

> `TODO when the API is stable enough to be versioned, create a pkg-config file`

Versioning has not started, so breaking `struct uiControl` is currently free. It will
become impossible after 1.0. If a versioned release is on the horizon, step 5 should be
batched into the last ABI-breaking window. If not, it can wait.

---

## 6. The plan

### Step 1 — `uiControlOnDestroyed()`

```c
_UI_EXTERN void uiControlOnDestroyed(uiControl *c,
    void (*f)(uiControl *c, void *data), void *data);
```

Fires when a control is actually destroyed, including when it is destroyed as part of
its parent.

**Fire it from `uiFreeControl()`.** Verified: all 47 `uiFreeControl()` call sites across
darwin, unix and windows are reached — every `Destroy` implementation funnels through
it. This gives complete coverage, including every descendant, with zero tree traversal.

Storage: `struct uiControl` cannot grow (ABI), so keep a small `uiControl *`-keyed list
in `common/control.c`, in the same style as the existing pending-destroy queue.

This single addition removes the top three rows of the table in §3.2.

**This is the only irreversible decision in the plan — see §7.**

### Step 2 — do not deliver callbacks to doomed controls

Change the region marker to take the control:

```c
if (uiprivUserCallbackEnter(uiControl(b))) {
    (*(b->onClicked))(b, b->onClickedData);
    uiprivUserCallbackLeave();
}
```

`uiprivUserCallbackEnter()` returns 0 when `uiprivControlDestroyPending()` is true.
Sites that have no associated control (`uiprivShouldQuit`, `uiQueueMain`, `uiTimer`,
table model wrappers in `common/tablemodel.c`) pass `NULL` and always proceed.

Scope: 92 call sites across 57 files. Mechanical — step 0 already visited all of them.

Value-returning callbacks (`KeyEvent`, `NumRows`, `CellValue`, `ColumnType`) cannot be
skipped and do not need to be: they are queries and are harmless on a doomed control.
Skipping only `void` callbacks is sufficient.

Also: call `(*(c->Hide))(c)` synchronously in `uiControlDestroy()` when deferring.
`Hide` is already part of the public vtable, so this costs nothing and largely closes
hazard C.

After this step, the sentence added to `ui.h` in step 0 —

> Until deferred destruction runs, the native event system may deliver additional
> callbacks for the control.

— can be deleted.

### Step 3 — ordered deferred free for non-control resources

Generalise the pending queue with a type tag so `uiTableModel` and `uiImage` can be
queued too. Then:

```c
uiControlDestroy(uiControl(table));   /* queued first  */
uiFreeTableModel(model);              /* queued second */
```

flushes in FIFO order and works. Hazard B disappears, and the "destroy tables before
freeing the model" rule disappears from binding documentation.

Discarded if step 5 is ever done.

### Step 4 — `uiGridDelete()`

`uiGrid` is the only container without a delete API (`uiBoxDelete`, `uiFormDelete`,
`uiTabDelete` all exist). This is an independent gap, and step 5 needs it anyway.

### Step 5 — automatic detach (optional, ABI-breaking)

Add a vtable entry so `uiControlDestroy()` can detach a control from its parent
automatically, removing the "do not destroy a child that still has a parent" rule and
keeping `uiControlParent()` / `uiBoxNumChildren()` honest immediately.

- 1 new `uiControl` vtable entry — **ABI break**; every binding embedding `uiControl`
  must recompile.
- 6 containers × 3 backends = 18 implementations.
- 3 implementations for synchronous table/model unregistration (lets step 3 be removed).

Can be migrated container by container: types not yet converted keep the step-0
deferred behaviour, so this is not a big-bang change.

---

## 7. The one irreversible decision

`uiControlOnDestroyed()` is public API. Its firing point cannot be changed later.

| Option | Coverage | Timing |
|---|---|---|
| At destroy *request* | only the explicitly destroyed control — **descendants are missed**, because `uiControl` has no `Children()` entry to traverse | early |
| At `uiFreeControl()` | **complete**, including all descendants, with no traversal | at flush |

Choose `uiFreeControl()`. "Late" is not a problem once step 2 suppresses callbacks in
the interval, and the firing point is identical under step 5 (which also defers the
physical free). Choosing request-time would look attractive now and become unfixable
once the API is frozen.

---

## 8. Expected end state

Binding-facing documentation after steps 1–4:

> A parent control owns its children. Destroying a parent destroys its children; their
> wrappers are invalidated automatically.
>
> Detach a child from its parent before destroying it.
>
> Call `free` on `Table::Model` and `Image` when you are done with them. Order does not
> matter.

Step 5 removes the second line.

---

## Appendix: measured facts

Everything below was measured against the working tree, not estimated.

| Fact | Value | How to reproduce |
|---|---|---|
| `Destroy` implementations that funnel through `uiFreeControl()` | all of them | `grep -rc uiFreeControl darwin unix windows` → 47 call sites; no `uiXxxDestroy` omits it |
| `uiprivUserCallbackEnter()` call sites | 92, in 57 files | `grep -rc 'uiprivUserCallbackEnter()' darwin unix windows common` |
| Control types per backend | 17 | count of `static void ui*Destroy(uiControl *` in `darwin/` |
| Container types | 6 — Box, Form, Grid, Tab, Window, Group | append/set-child APIs in `ui.h` |
| Containers without a delete API | Grid | `grep uiGridDelete ui.h` → none |
| Public `uiFree*` entry points | 11 | `grep '_UI_EXTERN.*uiFree' ui.h` |
| Exported symbols, HEAD vs patched | 315 vs 315, no difference | build both via `git worktree`, `nm -gU … \| sort \| diff` |
| `uipriv*` symbols exported | 0 | `nm -gU libui.dylib \| grep -c uipriv` |
| `struct uiControl` | public, fixed vtable, embedded by bindings | `ui.h:105-119` |
| Symbol visibility | `hidden` | `meson.build:145` |
| Default library type | shared | `meson.build:8` |
| API versioning | not started | `meson.build:151` TODO |
| Deferred-destroy unit tests | pass; require a static build (they use hidden symbols) | `meson setup b -Ddefault_library=static && ninja -C b && b/meson-out/unit` |
| `uiprivUserBug` termination | `__builtin_trap()` / `G_BREAKPOINT()` / `DebugBreak()` | `darwin/debug.m`, `unix/debug.c`, `windows/debug.cpp` |
