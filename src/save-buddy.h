#ifndef SAVE_BUDDY_H
#define SAVE_BUDDY_H

#include <gtk/gtkwindow.h>

#define BB_SAVE_ERROR (bb_save_error_quark ())
GQuark bb_save_error_quark (void);

gboolean bb_save_file (GtkWindow *parent, const char *filename, const char *utf8data, GError **err);

#endif /* SAVE_BUDDY_H */
