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
#include <sys/stat.h>
#include <time.h>

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
    GThread *main;

    gchar *title;
    gint size, completed;
    time_t ot;

    gint state;
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
static gboolean http_download_export_to_file (Download *self);

gpointer http_download_main (HttpDownload *self);
int http_download_progress (HttpDownload *self, gdouble dt, gdouble dn, gdouble ut, gdouble un);
static size_t http_download_write_data (char *buff, size_t size, size_t num, HttpDownload *self);

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

    iface->export = http_download_export_to_file;
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

    self->priv->source = NULL;
    self->priv->dest = NULL;

    self->priv->size = 0;
    self->priv->completed = 0;
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

    return DOWNLOAD (self);
}

Download*
http_download_new_from_file (const gchar *filename)
{
    GError *err = NULL;
    GKeyFile *kf = g_key_file_new ();

    HttpDownload *self = g_object_new (HTTP_DOWNLOAD_TYPE, NULL);

    g_key_file_load_from_file (kf, filename, G_KEY_FILE_NONE, &err);

    if (err) {
        g_print ("Error loading %s: %s\n", filename, err->message);
        g_error_free (err);
        err = NULL;
    }

    self->priv->source = g_key_file_get_string (kf, "Download", "Source", NULL);
    self->priv->dest = g_key_file_get_string (kf, "Download", "Destination", NULL);
    self->priv->size = g_key_file_get_integer (kf, "Dowload", "Size", NULL);
    self->priv->completed = g_key_file_get_integer (kf, "Download", "Completed", NULL);
    self->priv->title = g_path_get_basename (self->priv->dest);

    return DOWNLOAD (self);
}

static gboolean
http_download_export_to_file (Download *self)
{
    HttpDownloadPrivate *priv = HTTP_DOWNLOAD (self)->priv;
    gint i = strlen (priv->source);
    while (priv->source[i--] != '/');

    gchar *str = g_strdup_printf ("%s/gdman/%s.http", g_get_user_config_dir (), priv->source+i+2);
    FILE *fptr = fopen (str, "w");
    g_free (str);

    gchar *group = "[Download]\n";
    fwrite (group, 1, strlen (group), fptr);

    fwrite ("Source=", 1, 7, fptr);
    fwrite (priv->source, 1, strlen (priv->source), fptr);

    fwrite ("\nDestination=", 1, 13, fptr);
    fwrite (priv->dest, 1, strlen (priv->dest), fptr);

    str = g_strdup_printf ("\nSize=%d\n", priv->size);
    fwrite (str, 1, strlen (str), fptr);
    g_free (str);

    str = g_strdup_printf ("Completed=%d\n", priv->completed);
    fwrite (str, 1, strlen (str), fptr);
    g_free (str);

    fclose (fptr);
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
    return -1;
}

gint
http_download_get_time_remaining (Download *self)
{
    HttpDownloadPrivate *priv = HTTP_DOWNLOAD (self)->priv;

    gdouble cr;
    curl_easy_getinfo (priv->curl, CURLINFO_SPEED_DOWNLOAD, &cr);

    if (cr != 0) {
        return (priv->size - priv->completed) / cr;
    } else {
        return -1;
    }
}

gint
http_download_get_state (Download *self)
{
    return HTTP_DOWNLOAD (self)->priv->state;
}

static size_t
http_download_write_data (char *buff, size_t size, size_t num, HttpDownload *self)
{
    if (self->priv->state == DOWNLOAD_STATE_RUNNING) {
        fwrite (buff, size, num, self->priv->fptr);
        self->priv->completed += num * size;
    } else {
        return -1;
    }

    return size * num;
}

gboolean
http_download_start (Download *self)
{
    HttpDownloadPrivate *priv = HTTP_DOWNLOAD (self)->priv;

    priv->main = g_thread_create ((GThreadFunc) http_download_main,
        HTTP_DOWNLOAD (self), TRUE, NULL);
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
    HttpDownloadPrivate *priv = HTTP_DOWNLOAD (self)->priv;

    switch (priv->state) {
        case DOWNLOAD_STATE_RUNNING:
            priv->state = DOWNLOAD_STATE_PAUSED;
            g_thread_join (priv->main);
            break;
//        default:
    };
}

int
http_download_progress (HttpDownload *self, gdouble dt, gdouble dn, gdouble ut, gdouble un)
{
    time_t nt = time (NULL);

    if (nt != self->priv->ot) {
        self->priv->ot = nt;
        _emit_download_position_changed (DOWNLOAD (self));
    }

    return 0;
}

gpointer
http_download_main (HttpDownload *self)
{
    self->priv->curl = curl_easy_init ();

    // Get file length in a HEAD request
    curl_easy_setopt (self->priv->curl, CURLOPT_URL, self->priv->source);
    curl_easy_setopt (self->priv->curl, CURLOPT_NOBODY, 1);

    curl_easy_perform (self->priv->curl);

    gdouble cl;
    curl_easy_getinfo (self->priv->curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &cl);

    struct stat ostat;
    g_stat (self->priv->dest, &ostat);

    self->priv->size = cl;
    if (ostat.st_size > 0 && ostat.st_size == self->priv->completed && ostat.st_size < cl) {
        // If file has a length > 0 and is the same as the stored completed value
        // and the file is not already downloaded, continue where left off
        self->priv->completed = ostat.st_size;
        curl_easy_setopt (self->priv->curl, CURLOPT_RESUME_FROM, (long) self->priv->completed);

        self->priv->fptr = fopen (self->priv->dest, "a");
    } else if (ostat.st_size == cl) {
        // Download is completed
        self->priv->state = DOWNLOAD_STATE_COMPLETED;
        return;
    } else {
        // Either the download is new or an error occured so start over
        self->priv->fptr = fopen (self->priv->dest, "w");
    }

    curl_easy_setopt (self->priv->curl, CURLOPT_URL, self->priv->source);
    curl_easy_setopt (self->priv->curl, CURLOPT_NOBODY, 0);

    curl_easy_setopt (self->priv->curl, CURLOPT_WRITEFUNCTION, (curl_write_callback) http_download_write_data);
    curl_easy_setopt (self->priv->curl, CURLOPT_WRITEDATA, self);

    curl_easy_setopt (self->priv->curl, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt (self->priv->curl, CURLOPT_PROGRESSFUNCTION, (curl_progress_callback) http_download_progress);
    curl_easy_setopt (self->priv->curl, CURLOPT_PROGRESSDATA, self);

    self->priv->state = DOWNLOAD_STATE_RUNNING;

    curl_easy_perform (self->priv->curl);

    fclose (self->priv->fptr);

    self->priv->state = DOWNLOAD_STATE_COMPLETED;
    _emit_download_state_changed (DOWNLOAD (self), self->priv->state);
}
