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

static void
download_base_init (gpointer g_iface)
{
    static gboolean initialized = FALSE;

    if (!initialized) {
        initialized = TRUE;
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
            NULL,                   /* base finalize */
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

gboolean
download_is_complete (Download *self)
{
    DownloadInterface *iface = DOWNLOAD_GET_IFACE (self);

    if (iface->is_completed) {
        return
        iface->is_completed (self);
    } else {
        return -1;
    }
}
