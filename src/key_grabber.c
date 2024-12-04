#include "config.h"

#include "debug.h"
#include "key_grabber.h"
#include "screen-size.h"
#include "configsys.h"
#include "tilda.h"
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#define ANIMATION_UP 0
#define ANIMATION_DOWN 1

/* Define local variables here */
#define ANIMATION_Y 0
#define ANIMATION_X 1

static void pull_up (struct tilda_window_ *tw) {
    tw->current_state = STATE_GOING_UP;

    if (tw->fullscreen)
    {
        gtk_window_unfullscreen (GTK_WINDOW(tw->window));
    }

    gtk_widget_hide (GTK_WIDGET(tw->window));

    g_debug ("pull_up(): MOVED UP");
    tw->current_state = STATE_UP;
}

static void pull_down (struct tilda_window_ *tw) {
    tw->current_state = STATE_GOING_DOWN;

    tilda_window_set_active (tw);

    if (tw->fullscreen)
    {
        gtk_window_fullscreen (GTK_WINDOW(tw->window));
    }

    g_debug ("pull_down(): MOVED DOWN");
    tw->current_state = STATE_DOWN;
}

void pull (struct tilda_window_ *tw, enum pull_action action, gboolean force_hide)
{
    DEBUG_FUNCTION ("pull");
    DEBUG_ASSERT (tw != NULL);
    DEBUG_ASSERT (action == PULL_UP || action == PULL_DOWN || action == PULL_TOGGLE);

    gboolean needsFocus;

    if (!tw->wayland) {
        needsFocus = !gtk_window_is_active(GTK_WINDOW(tw->window)) &&
                     !force_hide &&
                     !tw->hide_non_focused;
    } else {
        needsFocus = FALSE;
    }

    if (g_get_monotonic_time() - tw->last_action_time < 150 * G_TIME_SPAN_MILLISECOND) {
        /* this is to prevent crazy toggling, with 150ms prevention time */
        return;
    }

    if (tw->current_state == STATE_DOWN && needsFocus) {
        /**
         * See tilda_window.c in focus_out_event_cb for an explanation about focus_loss_on_keypress
         * This conditional branch will only focus tilda but it does not actually pull the window up.
         */
        g_debug ("Tilda window not focused but visible");

        tilda_window_set_active(tw);
        return;
    }

    if ((tw->current_state == STATE_UP) && action != PULL_UP) {
        pull_down (tw);
    } else if ((tw->current_state == STATE_DOWN) && action != PULL_DOWN) {
        pull_up (tw);
    }

    tw->last_action = action;
    tw->last_action_time = g_get_monotonic_time();
}

static gint animation_coordinates[2][2][32];

static float animation_ease_function_down(gint i, gint n) {
    const float t = (float)i/n;
    const float ts = t*t;
    const float tc = ts*t;
    return 1*tc*ts + -5*ts*ts + 10*tc + -10*ts + 5*t;
}

static float animation_ease_function_up(gint i, gint n) {
    const float t = (float)i/n;
    const float ts = t*t;
    const float tc = ts*t;
    return 1 - (0*tc*ts + 0*ts*ts + 0*tc + 1*ts + 0*t);
}

void generate_animation_positions (struct tilda_window_ *tw)
{
    DEBUG_FUNCTION ("generate_animation_positions");
    DEBUG_ASSERT (tw != NULL);

    gint i;
    gint last_pos_x = config_getint ("x_pos");
    gint last_pos_y = config_getint ("y_pos");

    GdkRectangle rectangle;
    config_get_configured_window_size (&rectangle);

    gint last_width = rectangle.width;
    gint last_height = rectangle.height;
    gint screen_width;
    gint screen_height;
    screen_size_get_dimensions (&screen_width, &screen_height);

    for (i=0; i<32; i++)
    {
        switch (config_getint ("animation_orientation"))
        {
        case 3: /* right->left RIGHT */
            animation_coordinates[ANIMATION_UP][ANIMATION_Y][i] = last_pos_y;
            animation_coordinates[ANIMATION_UP][ANIMATION_X][i] =
                    (gint)(screen_width + (last_pos_x - screen_width) * animation_ease_function_up(i, 32));
            animation_coordinates[ANIMATION_DOWN][ANIMATION_Y][i] = last_pos_y;
            animation_coordinates[ANIMATION_DOWN][ANIMATION_X][i] =
                    (gint)(screen_width + (last_pos_x - screen_width) * animation_ease_function_down(i, 32));
            break;
        case 2: /* left->right LEFT */
            animation_coordinates[ANIMATION_UP][ANIMATION_Y][i] = last_pos_y;
            animation_coordinates[ANIMATION_UP][ANIMATION_X][i] =
                    (gint)(-last_width + (last_pos_x - -last_width) * animation_ease_function_up(i, 32));
            animation_coordinates[ANIMATION_DOWN][ANIMATION_Y][i] = last_pos_y;
            animation_coordinates[ANIMATION_DOWN][ANIMATION_X][i] =
                    (gint)(-last_width + (last_pos_x - -last_width) * animation_ease_function_down(i, 32));
            break;
        case 1: /* bottom->top BOTTOM */
            animation_coordinates[ANIMATION_UP][ANIMATION_Y][i] =
                    (gint)(screen_height + (last_pos_y - screen_height) * animation_ease_function_up(i, 32));
            animation_coordinates[ANIMATION_UP][ANIMATION_X][i] = last_pos_x;
            animation_coordinates[ANIMATION_DOWN][ANIMATION_Y][i] =
                    (gint)(screen_height + (last_pos_y - screen_height) * animation_ease_function_down(i, 32));
            animation_coordinates[ANIMATION_DOWN][ANIMATION_X][i] = last_pos_x;
            break;
        case 0: /* top->bottom TOP */
        default:
            animation_coordinates[ANIMATION_UP][ANIMATION_Y][i] =
                    (gint)(-last_height + (last_pos_y - -last_height) * animation_ease_function_up(i, 32));
            animation_coordinates[ANIMATION_UP][ANIMATION_X][i] = last_pos_x;
            animation_coordinates[ANIMATION_DOWN][ANIMATION_Y][i] =
                    (gint)(-last_height + (last_pos_y - -last_height) * animation_ease_function_down(i, 32));
            animation_coordinates[ANIMATION_DOWN][ANIMATION_X][i] = last_pos_x;
            break;
        }
    }
}

gboolean tilda_keygrabber_bind (const gchar *keystr, tilda_window *tw)
{
    return TRUE;
}

void tilda_keygrabber_unbind (const gchar *keystr);void tilda_keygrabber_unbind (const gchar *keystr)
{
    return;
}

void tilda_window_set_active (tilda_window *tw)
{
    DEBUG_FUNCTION ("tilda_window_set_active");
    DEBUG_ASSERT (tw != NULL);

    gtk_window_present(GTK_WINDOW(tw->window));

}
