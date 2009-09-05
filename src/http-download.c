/*
 *      http-download.c
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

#include "http-download.h"

#include "download.h"

static void download_init (DownloadInterface *iface);
G_DEFINE_TYPE_WITH_CODE (HttpDownload, http_download, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (DOWNLOAD_TYPE, download_init)
)

struct _HttpDownloadPrivate {
    gchar *source, *dest;
};

static gchar *http_download_get_title (Download *self);
static gint http_download_get_size_total (Download *self);
static gint http_download_get_size_remaining (Download *self);
static gint http_download_get_time_total (Download *self);
static gint http_download_get_time_remaining (Download *self);
static gint http_download_get_percentage (Download *self);
static gboolean http_download_get_state (Download *self);
static gboolean http_download_start (Download *self);
static gboolean http_download_stop (Download *self);
static gboolean http_download_cancel (Download *self);
static gboolean http_download_pause (Download *self);
gpointer http_download_main (HttpDownload *self);

static void
download_init (DownloadInterface *iface)
{
    iface->get_title = http_download_get_title;

    iface->get_size_tot = http_download_get_size_total;
    iface->get_size_rem = http_download_get_size_remaining;

    iface->get_time_tot = http_download_get_time_total;
    iface->get_time_rem = http_download_get_time_remaining;

    iface->get_percentage = http_download_get_percentage;
    iface->get_state = http_download_get_state;

    iface->start = http_download_start;
    iface->stop = http_download_stop;
    iface->cancel = http_download_cancel;
    iface->pause = http_download_pause;
}

static void
http_download_finalize (GObject *object)
{
    HttpDownload *self = HTTP_DOWNLOAD (object);

    G_OBJECT_CLASS (http_download_parent_class)->finalize (object);
}

static void
http_download_class_init (HttpDownloadClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (HttpDownloadPrivate));

    object_class->finalize = http_download_finalize;
}

static void
http_download_init (HttpDownload *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), HTTP_DOWNLOAD_TYPE, HttpDownloadPrivate);

}

Download*
http_download_new (const gchar *source, const gchar *dest)
{
    HttpDownload *self = g_object_new (HTTP_DOWNLOAD_TYPE, NULL);

    self->priv->source = g_strdup (source);
    self->priv->dest = g_strdup (dest);

    return DOWNLOAD (self);
}

gchar*
http_download_get_title (Download *self)
{

}

gint
http_download_get_size_total (Download *self)
{

}

gint
http_download_get_size_remaining (Download *self)
{

}

gint
http_download_get_time_total (Download *self)
{

}

gint
http_download_get_time_remaining (Download *self)
{

}

gint
http_download_get_percentage (Download *self)
{

}

gint
http_download_get_state (Download *self)
{

}

gboolean
http_download_start (Download *self)
{

}

gboolean
http_download_stop (Download *self)
{

}

gboolean
http_download_cancel (Download *self)
{

}

gboolean
http_download_pause (Download *self)
{

}

gpointer
http_download_main (HttpDownload *self)
{

}
