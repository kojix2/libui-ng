// 26 june 2015
#include "uipriv_unix.h"

// LONGTERM figure out why, and describe, that this is the desired behavior
// LONGTERM also point out that font and color buttons also work like this

#define windowWindow(w) ((w) ? (GTK_WINDOW(uiControlHandle(uiControl(w)))) : NULL)

static char *filedialog(GtkWindow *parent, GtkFileChooserAction mode, const gchar *confirm)
{
	GtkWidget *fcd;
	GtkFileChooser *fc;
	gint response;
	char *filename;

	fcd = gtk_file_chooser_dialog_new(NULL, parent, mode,
		"_Cancel", GTK_RESPONSE_CANCEL,
		confirm, GTK_RESPONSE_ACCEPT,
		NULL);
	fc = GTK_FILE_CHOOSER(fcd);
	gtk_file_chooser_set_local_only(fc, TRUE);
	gtk_file_chooser_set_select_multiple(fc, FALSE);
	gtk_file_chooser_set_show_hidden(fc, TRUE);
	gtk_file_chooser_set_do_overwrite_confirmation(fc, TRUE);
	gtk_file_chooser_set_create_folders(fc, TRUE);
	response = gtk_dialog_run(GTK_DIALOG(fcd));
	if (response != GTK_RESPONSE_ACCEPT) {
		gtk_widget_destroy(fcd);
		return NULL;
	}
	filename = gtk_file_chooser_get_filename(fc);
	gtk_widget_destroy(fcd);
	return filename;
}

char *uiOpenFile(uiWindow *parent)
{
	return filedialog(windowWindow(parent), GTK_FILE_CHOOSER_ACTION_OPEN, "_Open");
}

char *uiOpenFolder(uiWindow *parent)
{
	return filedialog(windowWindow(parent), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, "_Open");
}

static char **filedialogMultiple(GtkWindow *parent, GtkFileChooserAction mode)
{
	GtkWidget *fcd;
	GtkFileChooser *fc;
	GSList *files, *item;
	char **paths;
	size_t i, count;

	fcd = gtk_file_chooser_dialog_new(NULL, parent, mode,
		"_Cancel", GTK_RESPONSE_CANCEL,
		"_Open", GTK_RESPONSE_ACCEPT,
		NULL);
	fc = GTK_FILE_CHOOSER(fcd);
	gtk_file_chooser_set_local_only(fc, TRUE);
	gtk_file_chooser_set_select_multiple(fc, TRUE);
	gtk_file_chooser_set_show_hidden(fc, TRUE);
	gtk_file_chooser_set_create_folders(fc, TRUE);
	if (gtk_dialog_run(GTK_DIALOG(fcd)) != GTK_RESPONSE_ACCEPT) {
		gtk_widget_destroy(fcd);
		return NULL;
	}
	files = gtk_file_chooser_get_files(fc);
	count = (size_t) g_slist_length(files);
	paths = NULL;
	if (count != 0)
		paths = uiprivNewTextArray(count);
	if (paths != NULL) {
		i = 0;
		for (item = files; item != NULL; item = item->next) {
			paths[i] = g_file_get_path(G_FILE(item->data));
			if (paths[i] == NULL) {
				uiFreeTextArray(paths);
				paths = NULL;
				break;
			}
			i++;
		}
	}
	for (item = files; item != NULL; item = item->next)
		g_object_unref(item->data);
	g_slist_free(files);
	gtk_widget_destroy(fcd);
	return paths;
}

char **uiOpenFiles(uiWindow *parent)
{
	return filedialogMultiple(windowWindow(parent), GTK_FILE_CHOOSER_ACTION_OPEN);
}

char **uiOpenFolders(uiWindow *parent)
{
	return filedialogMultiple(windowWindow(parent), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
}

char *uiSaveFile(uiWindow *parent)
{
	return filedialog(windowWindow(parent), GTK_FILE_CHOOSER_ACTION_SAVE, "_Save");
}

static void msgbox(GtkWindow *parent, const char *title, const char *description, GtkMessageType type, GtkButtonsType buttons)
{
	GtkWidget *md;

	md = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL,
		type, buttons,
		"%s", title);
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(md), "%s", description);
	gtk_dialog_run(GTK_DIALOG(md));
	gtk_widget_destroy(md);
}

void uiMsgBox(uiWindow *parent, const char *title, const char *description)
{
	msgbox(windowWindow(parent), title, description, GTK_MESSAGE_OTHER, GTK_BUTTONS_OK);
}

void uiMsgBoxError(uiWindow *parent, const char *title, const char *description)
{
	msgbox(windowWindow(parent), title, description, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK);
}
