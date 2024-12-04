#ifndef TILDA_SCREEN_SIZE_H
#define TILDA_SCREEN_SIZE_H

//#include <glib.h>
#include <gtk/gtk.h>

void screen_size_get_dimensions (gint *width,
                                 gint *height);

void screen_size_set (GtkWindow *window,
                      gint width,
                      gint height);

#endif
