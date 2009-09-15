/*
 *      download.c
 *
 *      Copyright 2009 Brett Mravec <brett.mravec@gmail.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#include "download.h"

static guint signal_state_changed;
static guint signal_pos_changed;

static void
download_base_init (gpointer g_iface)
{
    static gboolean initialized = FALSE;

    if (!initialized) {
        initialized = TRUE;

        signal_state_changed = g_signal_new ("state-changed", DOWNLOAD_TYPE,
            G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__INT,
            G_TYPE_NONE, 1, G_TYPE_INT);

        signal_pos_changed = g_signal_new ("position-changed", DOWNLOAD_TYPE,
            G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);
    }
}

GType
download_get_type ()
{
    static GType object_type = 0;
    if (!object_type) {
        static const GTypeInfo object_info = {
            sizeof(DownloadInterface),
            download_base_init, /* base init */
            NULL,               /* base finalize */
        };

        object_type = g_type_register_static(G_TYPE_INTERFACE,
            "Download", &object_info, 0);
    }

    return object_type;
}

gchar*
download_get_title (Download *self)
{
    DownloadInterface *iface = DOWNLOAD_GET_IFACE (self);

    if (iface->get_title) {
        return iface->get_title (self);
    } else {
        return NULL;
    }
}

gint
download_get_size_total (Download *self)
{
    DownloadInterface *iface = DOWNLOAD_GET_IFACE (self);

    if (iface->get_size_tot) {
        return iface->get_size_tot (self);
    } else {
        return -1;
    }
}

gint
download_get_size_completed (Download *self)
{
    DownloadInterface *iface = DOWNLOAD_GET_IFACE (self);

    if (iface->get_size_comp) {
        return iface->get_size_comp (self);
    } else {
        return -1;
    }
}

gint
download_get_time_total (Download *self)
{
    DownloadInterface *iface = DOWNLOAD_GET_IFACE (self);

    if (iface->get_time_tot) {
        return iface->get_time_tot (self);
    } else {
        return -1;
    }
}

gint
download_get_time_remaining (Download *self)
{
    DownloadInterface *iface = DOWNLOAD_GET_IFACE (self);

    if (iface->get_time_rem) {
        return iface->get_time_rem(self);
    } else {
        return -1;
    }
}

gint
download_get_state (Download *self)
{
    DownloadInterface *iface = DOWNLOAD_GET_IFACE (self);

    if (iface->get_state) {
        return iface->get_state (self);
    } else {
        return DOWNLOAD_STATE_NONE;
    }
}

gboolean
download_start (Download *self)
{
    DownloadInterface *iface = DOWNLOAD_GET_IFACE (self);

    if (iface->start) {
        return iface->start (self);
    } else {
        return FALSE;
    }
}

gboolean
download_queue (Download *self)
{
    DownloadInterface *iface = DOWNLOAD_GET_IFACE (self);

    if (iface->start) {
        return iface->queue (self);
    } else {
        return FALSE;
    }
}

gboolean
download_stop (Download *self)
{
    DownloadInterface *iface = DOWNLOAD_GET_IFACE (self);

    if (iface->stop) {
        return iface->stop (self);
    } else {
        return FALSE;
    }
}

gboolean
download_cancel (Download *self)
{
    DownloadInterface *iface = DOWNLOAD_GET_IFACE (self);

    if (iface->cancel) {
        return iface->cancel (self);
    } else {
        return FALSE;
    }
}

gboolean
download_pause (Download *self)
{
    DownloadInterface *iface = DOWNLOAD_GET_IFACE (self);

    if (iface->pause) {
        return iface->pause (self);
    } else {
        return FALSE;
    }
}

gboolean
download_export_to_file (Download *self)
{
    DownloadInterface *iface = DOWNLOAD_GET_IFACE (self);

    if (iface->export) {
        return iface->export (self);
    } else {
        return FALSE;
    }
}


void
_emit_download_state_changed (Download *self, gint state)
{
    gdk_threads_enter ();
    g_signal_emit (self, signal_state_changed, 0, state);
    gdk_threads_leave ();
}

void
_emit_download_position_changed (Download *self)
{
    gdk_threads_enter ();
    g_signal_emit (self, signal_pos_changed, 0);
    gdk_threads_leave ();
}

gchar*
time_to_string (gint time)
{
    if (time == -1) {
        return g_strdup ("");
    }

    if (time < 60) {
        return g_strdup_printf ("%d seconds", time);
    } else if (time < 3600) {
        return g_strdup_printf ("%2d.%02d", time / 60, time % 60);
    } else {
        return g_strdup_printf ("%d.%02d.%02d", time / 3600, (time / 60) % 60, time % 60);
    }
}

gchar*
size_to_string (gint size)
{
    if (size < 1024) {
        return g_strdup_printf ("%d bytes", size);
    } else if (size < 1024*1024) {
        return g_strdup_printf ("%.2f KB", size / 1024.0);
    } else if (size < 1024 * 1024 * 1024) {
        return g_strdup_printf ("%.2f MB", size / (1024.0 * 1024.0));
    } else {
        return g_strdup_printf ("%.2f GB", size / (1024.0 * 1024.0 * 1024.0));
    }
}
