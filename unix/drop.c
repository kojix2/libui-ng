#include "uipriv_unix.h"
#include "drop.h"

typedef struct unixDrop unixDrop;
typedef struct transfer transfer;
typedef struct hover hover;
struct hover {
	uiprivDropEndpoint *endpoint;
	GdkDragContext *context; // Weak; its qdata destructor unlinks this record.
	uint64_t generation;
	unsigned int refs;
	int showing;
	uiDropAction action;
};

static GQuark hoverKey(void)
{
	return g_quark_from_static_string("libui-drop-hover");
}

enum {
	dropUnclaimed,
	dropPending,
	dropFinished,
};

static GQuark dropKey(void)
{
	return g_quark_from_static_string("libui-drop-state");
}

static void hoverRelease(gpointer data)
{
	hover *h = data;
	if (--h->refs != 0) return;
	uiprivDropRelease(h->endpoint);
	uiprivFree(h);
}

struct unixDrop {
	GtkWidget *widget;
	GtkWidget *coordinates;
	GtkTargetList *targets;
	GtkTargetList *textTargets;
	gulong signals[4];
	GList *hovers;
	GList *pending;
};

static void hoverUnlink(hover *h)
{
	unixDrop *d = h->endpoint->native;
	d->hovers = g_list_remove(d->hovers, h);
	h->context = NULL;
}

static void hoverRetire(gpointer data)
{
	hover *h = data;
	hoverUnlink(h);
	hoverRelease(h);
}

static void nativeFinish(GdkDragContext *context, gboolean accepted, guint time)
{
	if (GPOINTER_TO_INT(g_object_get_qdata(G_OBJECT(context), dropKey())) == dropFinished)
		return;
	g_object_set_qdata(G_OBJECT(context), dropKey(), GINT_TO_POINTER(dropFinished));
	gtk_drag_finish(context, accepted, FALSE, time);
}

struct transfer {
	uiprivDropEndpoint *endpoint;
	GdkDragContext *context;
	uiDropInfo info;
	GdkAtom target;
	uint64_t generation;
	guint time;
	guint timer;
	unsigned int refs;
	int finished;
};

static void transferRelease(gpointer data)
{
	transfer *t = data;
	if (--t->refs != 0)
		return;
	g_object_unref(t->context);
	uiprivDropRelease(t->endpoint);
	uiprivFree(t);
}

static void finish(transfer *t, const uiDropData *payload)
{
	unixDrop *d = t->endpoint->native;
	guint timer;
	gboolean accepted = FALSE;
	if (t->finished)
		return;
	t->refs++;
	t->finished = 1;
	d->pending = g_list_remove(d->pending, t);
	transferRelease(t); // list ownership
	timer = t->timer;
	t->timer = 0;
	if (timer != 0)
		g_source_remove(timer);
	// Detach before callbacks: a delivered Drop can remove its registration
	// and still accept, while late responses must never deliver it again.
	if (payload != NULL)
		accepted = uiprivDropDeliver(t->endpoint, t->generation, &t->info, payload) == uiDropAccepted;
	nativeFinish(t->context, accepted, t->time);
	transferRelease(t);
}

static gboolean timedOut(gpointer data)
{
	transfer *t = data;
	uiprivUserCallbackEnter(NULL);
	t->timer = 0; // GLib releases this source's reference on return.
	finish(t, NULL);
	uiprivUserCallbackLeave();
	return G_SOURCE_REMOVE;
}

static void cancel(uiprivDropEndpoint *e)
{
	unixDrop *d = e->native;
	while (d->hovers != NULL) {
		hover *h = d->hovers->data;
		g_object_steal_qdata(G_OBJECT(h->context), hoverKey());
		hoverRetire(h);
	}
	while (d->pending != NULL)
		finish(d->pending->data, NULL);
}

static uiDropStatus enable(uiprivDropEndpoint *e, int active)
{
	unixDrop *d = e->native;
	if (active) {
		gtk_drag_dest_set(d->widget, 0, NULL, 0, GDK_ACTION_COPY);
		// Broad known categories make an explicit Area's rejection exclusive.
		gtk_drag_dest_set_target_list(d->widget, d->targets);
	} else
		gtk_drag_dest_unset(d->widget);
	return uiDropStatusOK;
}

static void dispose(uiprivDropEndpoint *e)
{
	unixDrop *d = e->native;
	unsigned int i;
	for (i = 0; i < 4; i++)
		g_signal_handler_disconnect(d->widget, d->signals[i]);
	gtk_target_list_unref(d->textTargets);
	gtk_target_list_unref(d->targets);
	g_object_unref(d->coordinates);
	g_object_unref(d->widget);
	uiprivFree(d);
}

static uiDropInfo makeInfo(uiprivDropEndpoint *e, GdkDragContext *context, gint x, gint y)
{
	unixDrop *d = e->native;
	GList *item;
	unsigned int formats = 0;
	gint tx = x, ty = y;
	for (item = gdk_drag_context_list_targets(context); item != NULL; item = item->next) {
		GdkAtom atom = GDK_POINTER_TO_ATOM(item->data);
		if (gtk_targets_include_uri(&atom, 1)) formats |= uiDropFormatFiles;
		if (gtk_targets_include_text(&atom, 1)) formats |= uiDropFormatText;
	}
	gtk_widget_translate_coordinates(d->widget, d->coordinates, x, y, &tx, &ty);
	return uiprivDropMakeInfo(e, formats, tx, ty);
}

static gboolean motion(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, gpointer data)
{
	uiprivDropEndpoint *e = data;
	unixDrop *d = e->native;
	uint64_t generation = e->generation;
	hover *h;
	uiDropInfo info;
	uiDropAction action;
	uiprivDropRetain(e);
	uiprivUserCallbackEnter(NULL);
	info = makeInfo(e, context, x, y);
	h = uiprivNew(hover);
	h->endpoint = e;
	h->context = context;
	uiprivDropRetain(e);
	h->generation = generation;
	h->refs = 2; // context and this delivery; nested motion can replace qdata.
	h->showing = 1;
	d->hovers = g_list_prepend(d->hovers, h);
	g_object_set_qdata_full(G_OBJECT(context), hoverKey(), h, hoverRetire);
	action = uiprivDropOver(e, &info, (gdk_drag_context_get_actions(context) & GDK_ACTION_COPY) != 0);
	if (g_object_get_qdata(G_OBJECT(context), hoverKey()) != h)
		action = uiDropActionNone;
	h->action = action;
	gdk_drag_status(context, action == uiDropActionCopy ? GDK_ACTION_COPY : 0, time);
	hoverRelease(h);
	uiprivDropRelease(e);
	uiprivUserCallbackLeave();
	return TRUE; // action 0 rejects without falling through to the parent.
}

static void leave(GtkWidget *widget, GdkDragContext *context, guint time, gpointer data)
{
	uiprivDropEndpoint *e = data;
	hover *h = g_object_get_qdata(G_OBJECT(context), hoverKey());
	uint64_t generation;
	if (h == NULL || h->endpoint != e || !h->showing)
		return;
	generation = h->generation;
	h->showing = 0;
	// GTK emits Leave before Drop. Keep the last negotiated action on the
	// native context without pinning a retired registration or replaying Over.
	uiprivDropLeave(e, generation);
}

static gboolean drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, gpointer data)
{
	uiprivDropEndpoint *e = data;
	unixDrop *d = e->native;
	uiDropInfo info;
	transfer *t;
	GdkAtom target;
	hover *h;
	uint64_t generation = e->generation;
	if (GPOINTER_TO_INT(g_object_get_qdata(G_OBJECT(context), dropKey())) != dropUnclaimed)
		return TRUE;
	g_object_set_qdata(G_OBJECT(context), dropKey(), GINT_TO_POINTER(dropPending));
	uiprivDropRetain(e);
	uiprivUserCallbackEnter(NULL);
	info = makeInfo(e, context, x, y);
	h = g_object_steal_qdata(G_OBJECT(context), hoverKey());
	if (h != NULL) hoverUnlink(h);
	if (h == NULL || h->endpoint != e || h->generation != generation ||
		h->action != uiDropActionCopy || !(gdk_drag_context_get_actions(context) & GDK_ACTION_COPY) ||
		!uiprivDropActive(e, generation)) {
		nativeFinish(context, FALSE, time);
		goto done;
	}
	if (h->showing)
		uiprivDropLeave(e, generation);
	if (!uiprivDropActive(e, generation)) {
		nativeFinish(context, FALSE, time);
		goto done;
	}
	target = info.Format == uiDropFormatFiles ? gdk_atom_intern_static_string("text/uri-list") :
		gtk_drag_dest_find_target(widget, context, d->textTargets);
	if (target == GDK_NONE) {
		nativeFinish(context, FALSE, time);
		goto done;
	}
	t = uiprivNew(transfer);
	t->endpoint = e;
	uiprivDropRetain(e);
	t->context = g_object_ref(context);
	t->info = info; // Freeze physical drop coordinates before requesting data.
	t->generation = generation;
	t->time = time;
	t->target = target;
	t->refs = 3; // list, this call, timeout source
	d->pending = g_list_prepend(d->pending, t);
	t->timer = g_timeout_add_full(G_PRIORITY_DEFAULT, 10000, timedOut, t, transferRelease);
	gtk_drag_get_data(widget, context, target, time);
	transferRelease(t);
done:
	if (h != NULL) hoverRelease(h);
	uiprivDropRelease(e);
	uiprivUserCallbackLeave();
	return TRUE;
}

static int readFiles(GtkSelectionData *selection, uiprivDropData *payload)
{
	gchar **uris;
	size_t i;
	int valid = 1;
	uris = gtk_selection_data_get_uris(selection);
	if (uris == NULL || uris[0] == NULL)
		return 0;
	for (i = 0; uris[i] != NULL; i++) {
		gchar *host = NULL;
		gchar *path = g_filename_from_uri(uris[i], &host, NULL);
		// A remote file URI cannot be represented as a local Unix path.
		if (path == NULL || (host != NULL && *host != 0 && g_ascii_strcasecmp(host, "localhost") != 0) ||
			!uiprivDropAddFile(payload, path, strlen(path)))
			valid = 0;
		g_free(host);
		g_free(path);
		if (!valid) break;
	}
	g_strfreev(uris);
	return valid;
}

static void received(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
	GtkSelectionData *selection, guint targetInfo, guint time, gpointer data)
{
	uiprivDropEndpoint *e = data;
	unixDrop *d = e->native;
	GList *item;
	transfer *t = NULL;
	uiprivDropData payload = {0};
	int valid = 0;
	gint length = gtk_selection_data_get_length(selection);
	for (item = d->pending; item != NULL; item = item->next) {
		transfer *candidate = item->data;
		if (candidate->context == context && candidate->target == gtk_selection_data_get_target(selection)) {
			t = candidate;
			break;
		}
	}
	if (t == NULL)
		return; // Late response after removal, timeout or destruction.
	t->refs++;
	uiprivUserCallbackEnter(NULL);
	if (uiprivDropActive(e, t->generation) && length >= 0 && (size_t) length < uiprivDropMaxBytes &&
		gtk_selection_data_get_format(selection) == 8 &&
		(length == 0 || (gtk_selection_data_get_data(selection) != NULL &&
		memchr(gtk_selection_data_get_data(selection), 0, (size_t) length) == NULL))) {
		if (t->info.Format == uiDropFormatFiles) {
			valid = readFiles(selection, &payload);
		} else if (length == 0) {
			valid = uiprivDropSetText(&payload, "", 0);
		} else {
			guchar *text = gtk_selection_data_get_text(selection);
			if (text != NULL)
				valid = uiprivDropSetText(&payload, (const char *) text, strlen((const char *) text));
			g_free(text);
		}
	}
	finish(t, valid ? &payload.data : NULL);
	uiprivDropDataClear(&payload);
	transferRelease(t);
	uiprivUserCallbackLeave();
}

uiprivDropEndpoint *uiprivUnixDropNew(uiControl *c, GtkWidget *widget, GtkWidget *coordinates)
{
	uiprivDropEndpoint *e = uiprivDropNew(c);
	unixDrop *d = uiprivNew(unixDrop);
	d->widget = g_object_ref(widget);
	d->coordinates = g_object_ref(coordinates);
	d->targets = gtk_target_list_new(NULL, 0);
	d->textTargets = gtk_target_list_new(NULL, 0);
	gtk_target_list_add_uri_targets(d->targets, 1);
	gtk_target_list_add_text_targets(d->targets, 2);
	gtk_target_list_add_text_targets(d->textTargets, 2);
	e->native = d;
	e->enable = enable;
	e->cancel = cancel;
	e->dispose = dispose;
	d->signals[0] = g_signal_connect(widget, "drag-motion", G_CALLBACK(motion), e);
	d->signals[1] = g_signal_connect(widget, "drag-leave", G_CALLBACK(leave), e);
	d->signals[2] = g_signal_connect(widget, "drag-drop", G_CALLBACK(drop), e);
	d->signals[3] = g_signal_connect(widget, "drag-data-received", G_CALLBACK(received), e);
	return e;
}
