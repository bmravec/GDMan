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

static void
download_base_init (gpointer g_iface)
{
    static gboolean initialized = FALSE;

    if (!initialized) {
        initialized = TRUE;

        signal_state_changed = g_signal_new ("state-changed", DOWNLOAD_TYPE,
            G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__INT,
            G_TYPE_NONE, 1, G_TYPE_INT);
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
download_get_size_remaining (Download *self)
{
    DownloadInterface *iface = DOWNLOAD_GET_IFACE (self);

    if (iface->get_size_rem) {
        return iface->get_size_rem (self);
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
download_get_percentage (Download *self)
{
    DownloadInterface *iface = DOWNLOAD_GET_IFACE (self);

    if (iface->get_percentage) {
        return iface->get_percentage (self);
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

void
_emit_state_changed (Download *self, gint state)
{
    g_signal_emit (self, signal_state_changed, state);
}
