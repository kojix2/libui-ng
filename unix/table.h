// 4 june 2018
#include "../common/table.h"

// tablemodel.c
#define uiTableModelType (uiTableModel_get_type())
#define uiTableModel(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), uiTableModelType, uiTableModel))
typedef struct uiTableModelClass uiTableModelClass;
struct uiTableModel {
	GObject parent_instance;
	gint stamp;
	uiTableModelHandler *mh;
	GPtrArray *tables;
};
struct uiTableModelClass {
	GObjectClass parent_class;
};
extern GType uiTableModel_get_type(void);
extern void uiprivTableRowInserted(uiTable *t, int newIndex);
extern void uiprivTableRowDeleted(uiTable *t, int oldIndex);
