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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <curl/curl.h>

#include "http-download.h"

#include "download.h"

static void download_init (DownloadInterface *iface);
G_DEFINE_TYPE_WITH_CODE (HttpDownload, http_download, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (DOWNLOAD_TYPE, download_init)
)

struct _HttpDownloadPrivate {
    gchar *source, *dest;

    CURL *curl;
    FILE *fptr;

    gchar *title;
    gchar *host, *path;
    gint port;

    gint size, completed;
    gint rate;

    gint prev_time, prev_comp;

    gboolean allow_bytes;
    gboolean running;

    gint total_time;

    gint state;

    gchar *post_data;
    gchar *referer;
};

static gchar *http_download_get_title (Download *self);
static gint http_download_get_size_total (Download *self);
static gint http_download_get_size_completed (Download *self);
static gint http_download_get_time_total (Download *self);
static gint http_download_get_time_remaining (Download *self);
static gboolean http_download_get_state (Download *self);
static gboolean http_download_start (Download *self);
static gboolean http_download_stop (Download *self);
static gboolean http_download_cancel (Download *self);
static gboolean http_download_pause (Download *self);
gpointer http_download_main (HttpDownload *self);

static int socket_connect (char *host, int port);

static void
download_init (DownloadInterface *iface)
{
    iface->get_title = http_download_get_title;

    iface->get_size_tot = http_download_get_size_total;
    iface->get_size_comp = http_download_get_size_completed;

    iface->get_time_tot = http_download_get_time_total;
    iface->get_time_rem = http_download_get_time_remaining;

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

    self->priv->allow_bytes = FALSE;
    self->priv->source = NULL;
    self->priv->dest = NULL;
    self->priv->host = NULL;
    self->priv->path = NULL;

    self->priv->size = 0;
    self->priv->completed = 0;
    self->priv->total_time = 0;

    self->priv->post_data = NULL;
    self->priv->referer = NULL;
}

Download*
http_download_new (const gchar *source, const gchar *dest, gboolean nohead)
{
    HttpDownload *self = g_object_new (HTTP_DOWNLOAD_TYPE, NULL);

    self->priv->source = g_strdup (source);

    if (dest[0] == '/') {
        self->priv->dest = g_strdup (dest);
    } else if (dest[0] == '~') {
        self->priv->dest = g_build_filename (g_get_home_dir (), dest+2);
    } else {
        self->priv->dest = g_build_filename (g_get_tmp_dir (), dest);
    }

    if (g_file_test (self->priv->dest, G_FILE_TEST_IS_DIR)) {
        gint len = strlen (source);
        while (source[--len] != '/');
        gchar *new_dest = g_strdup_printf ("%s/%s", self->priv->dest, source+len);
        g_free (self->priv->dest);
        self->priv->dest = new_dest;
    }


    self->priv->title = g_path_get_basename (self->priv->dest);

    int sockfd, i;

    gchar **proto_strs = g_strsplit (self->priv->source, "://", 2);
    gchar **host_strs = g_strsplit (proto_strs[1], "/", 2);
    gchar **port_strs = g_strsplit (host_strs[0], ":", 2);

    if (port_strs[1]) {
        self->priv->port = atoi (port_strs[1]);
    } else {
        self->priv->port = 80;
    }

    self->priv->host = g_strdup (port_strs[0]);
    self->priv->path = g_strdup (host_strs[1]);

    g_strfreev (port_strs);
    g_strfreev (host_strs);
    g_strfreev (proto_strs);

    self->priv->curl = curl_easy_init ();
    curl_easy_setopt (self->priv->curl, CURLOPT_URL, self->priv->source);
//    curl_easy_setopt (self->priv->curl, CURLOPT_VERBOSE, 1);

    g_print ("Download: %s -> %s\n", self->priv->source, self->priv->dest);

    return DOWNLOAD (self);
}

gchar*
http_download_get_title (Download *self)
{
    return HTTP_DOWNLOAD (self)->priv->title;
}

gint
http_download_get_size_total (Download *self)
{
    HttpDownloadPrivate *priv = HTTP_DOWNLOAD (self)->priv;
    return !priv->size ? -1 : priv->size;
}

gint
http_download_get_size_completed (Download *self)
{
    return HTTP_DOWNLOAD (self)->priv->completed;
}

gint
http_download_get_time_total (Download *self)
{
    return HTTP_DOWNLOAD (self)->priv->total_time;
}

gint
http_download_get_time_remaining (Download *self)
{
    HttpDownloadPrivate *priv = HTTP_DOWNLOAD (self)->priv;

    if (priv->rate != 0) {
        int nt = (priv->prev_time + 3 * (priv->size - priv->completed) / priv->rate) / 4;

        priv->prev_time = nt;

        return nt;
    } else {
        return 0;
    }
}

gint
http_download_get_state (Download *self)
{
    return HTTP_DOWNLOAD (self)->priv->state;
}

gboolean
on_timeout (HttpDownload *self)
{
    self->priv->total_time++;

    gdouble cr;
    curl_easy_getinfo (self->priv->curl, CURLINFO_SPEED_DOWNLOAD, &cr);

    int diff = (self->priv->rate / 2 + 3 * (self->priv->completed - self->priv->prev_comp)) / 4;

    self->priv->rate = (gint) cr;
    self->priv->prev_comp = self->priv->completed;

    _emit_download_position_changed (DOWNLOAD (self));

    if (self->priv->state == DOWNLOAD_STATE_COMPLETED) {
        return FALSE;
        g_object_unref (self);
    }

    return TRUE;
}

static size_t
write_data (char *buff, size_t size, size_t num, HttpDownload *self)
{
    if (self->priv->size == 0 && self->priv->curl) {
        gdouble fs;
        curl_easy_getinfo (self->priv->curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &fs);
        self->priv->size = (gint) fs;
    }

    if (!self->priv->fptr) {
        self->priv->fptr = fopen (self->priv->dest, "w");
    }

    fwrite (buff, size, num, self->priv->fptr);
    self->priv->completed += num * size;

    return size * num;
}

gboolean
http_download_start (Download *self)
{
    HttpDownloadPrivate *priv = HTTP_DOWNLOAD (self)->priv;

    curl_easy_setopt (priv->curl, CURLOPT_WRITEFUNCTION, (curl_write_callback) write_data);
    curl_easy_setopt (priv->curl, CURLOPT_WRITEDATA, HTTP_DOWNLOAD (self));

    if (priv->post_data) {
        curl_easy_setopt (priv->curl, CURLOPT_POST, 1);
        curl_easy_setopt (priv->curl, CURLOPT_POSTFIELDS, priv->post_data);
    }

    g_thread_create ((GThreadFunc) http_download_main,
        HTTP_DOWNLOAD (self), FALSE, NULL);

    g_timeout_add (500, (GSourceFunc) on_timeout, self);
    g_object_ref (self);

    priv->state = DOWNLOAD_STATE_RUNNING;
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

void
http_download_set_post (HttpDownload *self, gchar *data)
{
    self->priv->post_data = g_strdup (data);
}

void
http_download_set_referer (HttpDownload *self, gchar *data)
{
    self->priv->referer = g_strdup (data);
}

gpointer
http_download_main (HttpDownload *self)
{
    curl_easy_perform (self->priv->curl);

    fclose (self->priv->fptr);

    self->priv->state = DOWNLOAD_STATE_COMPLETED;
    _emit_download_state_changed (DOWNLOAD (self), self->priv->state);
}
