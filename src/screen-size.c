#include "screen-size.h"

void
screen_size_get_dimensions (gint *width,
                            gint *height)
{
    GdkScreen *screen;
    GdkWindow *window;

    screen = gdk_screen_get_default ();
    window = gdk_screen_get_root_window (screen);

    *width = gdk_window_get_width (window);
    *height = gdk_window_get_height (window);
}

void
screen_size_set (GtkWindow *window,
                 gint width,
                 gint height)
{
    gint scale = gtk_widget_get_scale_factor (GTK_WIDGET(window));
    gtk_window_resize (window, width / scale, height / scale);
}
