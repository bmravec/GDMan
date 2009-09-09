/*
 *      youtube-download.c
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

#include <curl/curl.h>

#include "youtube-download.h"

#include "http-download.h"
#include "download.h"

static void download_init (DownloadInterface *iface);
G_DEFINE_TYPE_WITH_CODE (YoutubeDownload, youtube_download, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (DOWNLOAD_TYPE, download_init)
)

enum {
    YOUTUBE_STAGE_DFIRST = 0,
    YOUTUBE_STAGE_DFILE,
};

struct _YoutubeDownloadPrivate {
    gchar *source, *dest;

    gchar *buff;
    gint buff_pos;

    gint size, completed, prev_comp;

    FILE *fptr;
    CURL *curl;

    GThread *main;

    gint state, stage;
};

static gchar *youtube_download_get_title (Download *self);
static gint youtube_download_get_size_total (Download *self);
static gint youtube_download_get_size_completed (Download *self);
static gint youtube_download_get_time_total (Download *self);
static gint youtube_download_get_time_remaining (Download *self);
static gboolean youtube_download_get_state (Download *self);
static gboolean youtube_download_start (Download *self);
static gboolean youtube_download_stop (Download *self);
static gboolean youtube_download_cancel (Download *self);
static gboolean youtube_download_pause (Download *self);
static gboolean youtube_download_export_to_file (Download *self);

gboolean youtube_timeout (YoutubeDownload *self);
gpointer youtube_download_main (YoutubeDownload *self);

static size_t youtube_write_data (char *buff, size_t size, size_t num, YoutubeDownload *self);

static void
download_init (DownloadInterface *iface)
{
    iface->get_title = youtube_download_get_title;

    iface->get_size_tot = youtube_download_get_size_total;
    iface->get_size_comp = youtube_download_get_size_completed;

    iface->get_time_tot = youtube_download_get_time_total;
    iface->get_time_rem = youtube_download_get_time_remaining;

    iface->get_state = youtube_download_get_state;

    iface->start = youtube_download_start;
    iface->stop = youtube_download_stop;
    iface->cancel = youtube_download_cancel;
    iface->pause = youtube_download_pause;

    iface->export = youtube_download_export_to_file;
}

static void
youtube_download_finalize (GObject *object)
{
    YoutubeDownload *self = YOUTUBE_DOWNLOAD (object);

    G_OBJECT_CLASS (youtube_download_parent_class)->finalize (object);
}

static void
youtube_download_class_init (YoutubeDownloadClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (YoutubeDownloadPrivate));

    object_class->finalize = youtube_download_finalize;
}

static void
youtube_download_init (YoutubeDownload *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), YOUTUBE_DOWNLOAD_TYPE, YoutubeDownloadPrivate);
}

Download*
youtube_download_new (const gchar *source, const gchar *dest)
{
    YoutubeDownload *self = g_object_new (YOUTUBE_DOWNLOAD_TYPE, NULL);

    self->priv->source = g_strdup (source);
    self->priv->dest = g_strdup (dest);

    return DOWNLOAD (self);
}

Download*
youtube_download_new_from_file (const gchar *filename)
{
    GError *err = NULL;
    GKeyFile *kf = g_key_file_new ();

    YoutubeDownload *self = g_object_new (YOUTUBE_DOWNLOAD_TYPE, NULL);

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

    return DOWNLOAD (self);
}

gboolean
youtube_download_export_to_file (Download *self)
{
    YoutubeDownloadPrivate *priv = YOUTUBE_DOWNLOAD (self)->priv;

    gint i = strlen (priv->source);
    while (priv->source[i--] != '=');

    gchar *str = g_strdup_printf ("%s/gdman/%s.youtube", g_get_user_config_dir (), priv->source+i+2);
    FILE *fptr = fopen (str, "w");
    g_free (str);

    gchar *group = "[Download]\n";
    fwrite (group, 1, strlen (group), fptr);

    fwrite ("Source=", 1, 7, fptr);
    fwrite (priv->source, 1, strlen (priv->source), fptr);

    fwrite ("\nDestination=", 1, 13, fptr);
    fwrite (priv->dest, 1, strlen (priv->dest), fptr);

//    fwrite ("\nSize=", 1, 6, fptr);
    str = g_strdup_printf ("\nSize=%d\n", priv->size);
    fwrite (str, 1, strlen (str), fptr);
    g_free (str);

    str = g_strdup_printf ("Completed=%d\n", priv->completed);
    fwrite (str, 1, strlen (str), fptr);
    g_free (str);

    fclose (fptr);
}

gchar*
youtube_download_get_title (Download *self)
{
    YoutubeDownloadPrivate *priv = YOUTUBE_DOWNLOAD (self)->priv;

    switch (priv->stage) {
        case YOUTUBE_STAGE_DFIRST:
            return g_strdup_printf ("Youtube 1 / 2: %s", priv->source);
            break;
        case YOUTUBE_STAGE_DFILE:
            return g_strdup_printf ("Youtube 2 / 2: %s", priv->source);
            break;
        default:
            return NULL;
    }
}

gint
youtube_download_get_size_total (Download *self)
{
    return YOUTUBE_DOWNLOAD (self)->priv->size;
}

gint
youtube_download_get_size_completed (Download *self)
{
    return YOUTUBE_DOWNLOAD (self)->priv->completed;
}

gint
youtube_download_get_time_total (Download *self)
{

}

gint
youtube_download_get_time_remaining (Download *self)
{
    return -1; // download_get_time_remaining (YOUTUBE_DOWNLOAD (self)->priv->down);
}

gint
youtube_download_get_state (Download *self)
{
    return YOUTUBE_DOWNLOAD (self)->priv->state;
}

gboolean
youtube_download_start (Download *self)
{
    YoutubeDownloadPrivate *priv = YOUTUBE_DOWNLOAD (self)->priv;

    priv->main = g_thread_create ((GThreadFunc) youtube_download_main, YOUTUBE_DOWNLOAD (self), TRUE, NULL);

    g_timeout_add (500, (GSourceFunc) youtube_timeout, self);
    g_object_ref (self);
}

gboolean
youtube_download_stop (Download *self)
{

}

gboolean
youtube_download_cancel (Download *self)
{

}

gboolean
youtube_download_pause (Download *self)
{
    YoutubeDownloadPrivate *priv = YOUTUBE_DOWNLOAD (self)->priv;

    switch (priv->state) {
        case DOWNLOAD_STATE_RUNNING:
            priv->state = DOWNLOAD_STATE_PAUSED;
            g_thread_join (priv->main);
            break;
//        default:
    };
}

static void
on_pos_changed (Download *download, YoutubeDownload *self)
{
    _emit_download_position_changed (DOWNLOAD (self));
}

static gchar*
youtube_parse_dl_link (gchar *data)
{
    gint i, j;

    gint fmt = 0;
    gchar *url = NULL;

    gchar **lines = g_strsplit (data, "&", 0);

    for (i = 0; lines[i]; i++) {
        if (g_str_has_prefix (lines[i], "fmt_url_map")) {
            gchar *str = g_uri_unescape_string (lines[i]+12, "");
            gchar **urlmap = g_strsplit (str, ",", 0);
            g_free (str);

            for (j = 0; urlmap[j]; j++) {
                gchar **kv = g_strsplit (urlmap[j], "|", 0);

                gint nfmt = atoi (kv[0]);
                if (nfmt > fmt) {
                    if (url) g_free (url);
                    url = g_uri_unescape_string (kv[1], "");
                    fmt = nfmt;
                }

                g_strfreev (kv);
            }

            g_strfreev (urlmap);
            break;
        }
    }

    g_strfreev (lines);

    return url;
}

static size_t
youtube_write_data (char *buff, size_t size, size_t num, YoutubeDownload *self)
{
    if (self->priv->state != DOWNLOAD_STATE_RUNNING) {
        return -1;
    }

    switch (self->priv->stage) {
        case YOUTUBE_STAGE_DFIRST:
            if (self->priv->buff == NULL) {
                gdouble cs;
                curl_easy_getinfo (self->priv->curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &cs);

                self->priv->buff = g_new0 (gchar, (gint) cs);
                self->priv->buff_pos = 0;
            }

            memcpy (self->priv->buff+self->priv->buff_pos, buff, size * num);
            self->priv->buff_pos += size * num;
            break;
        case YOUTUBE_STAGE_DFILE:
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
            break;
        default:
            return -1;
    }

    return size * num;
}

gboolean
youtube_timeout (YoutubeDownload *self)
{
//    self->priv->total_time++;
/*
    gdouble cr;
    curl_easy_getinfo (self->priv->curl, CURLINFO_SPEED_DOWNLOAD, &cr);

    int diff = (self->priv->rate / 2 + 3 * (self->priv->completed - self->priv->prev_comp)) / 4;

    self->priv->rate = (gint) cr;
    self->priv->prev_comp = self->priv->completed;
*/
    _emit_download_position_changed (DOWNLOAD (self));

    if (self->priv->state != DOWNLOAD_STATE_RUNNING) {
        return FALSE;
        g_object_unref (self);
    }

    return TRUE;
}

gpointer
youtube_download_main (YoutubeDownload *self)
{
    gint i = strlen (self->priv->source);
    while (self->priv->source[i--] != '=');

    self->priv->curl = curl_easy_init ();

    gchar *str = g_strdup_printf ("http://www.youtube.com/get_video_info?&video_id=%s", self->priv->source+i+2);
    curl_easy_setopt (self->priv->curl, CURLOPT_URL, str);
    g_free (str);

    curl_easy_setopt (self->priv->curl, CURLOPT_WRITEFUNCTION, (curl_write_callback) youtube_write_data);
    curl_easy_setopt (self->priv->curl, CURLOPT_WRITEDATA, YOUTUBE_DOWNLOAD (self));

    self->priv->stage = YOUTUBE_STAGE_DFIRST;
    self->priv->state = DOWNLOAD_STATE_RUNNING;

    curl_easy_perform (self->priv->curl);

    str = youtube_parse_dl_link (self->priv->buff);
    g_free (self->priv->buff);

    curl_easy_setopt (self->priv->curl, CURLOPT_URL, str);
    curl_easy_setopt (self->priv->curl, CURLOPT_NOBODY, 1);

    curl_easy_perform (self->priv->curl);

    gdouble cl;
    curl_easy_getinfo (self->priv->curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &cl);
    g_print ("CL: %f\n", cl);

    gchar *dest;
    if (self->priv->dest[0] == '/') {
        dest = g_strdup (self->priv->dest);
    } else if (self->priv->dest[0] == '~') {
        dest = g_build_filename (g_get_home_dir (), self->priv->dest+2);
    } else {
        dest = g_build_filename (g_get_tmp_dir (), self->priv->dest);
    }

    if (g_file_test (dest, G_FILE_TEST_IS_DIR)) {
        gint i = strlen (self->priv->source);
        while (self->priv->source[i--] != '=');

        gchar *new_dest = g_strdup_printf ("%s/youtube%s.flv", dest, self->priv->source+i+2);
        g_free (dest);
        dest = new_dest;
    }

    struct stat ostat;
    g_stat (dest, &ostat);

    g_print ("Prev Size: %d\n", ostat.st_size);

    curl_easy_setopt (self->priv->curl, CURLOPT_URL, str);
    g_free (str);

    self->priv->size = cl;
    if (ostat.st_size > 0 && ostat.st_size < cl) {
        g_print ("Resuming: %d\n", self->priv->completed);
        self->priv->completed = ostat.st_size;
        curl_easy_setopt (self->priv->curl, CURLOPT_RESUME_FROM, (long) self->priv->completed);

        self->priv->fptr = fopen (dest, "a");
    } else if (ostat.st_size == cl) {
        g_print ("Already downloaded\n");
        return;
    } else {
        g_print ("Starting\n");
        self->priv->fptr = fopen (dest, "w");
    }

    curl_easy_setopt (self->priv->curl, CURLOPT_NOBODY, 0);
    curl_easy_setopt (self->priv->curl, CURLOPT_WRITEFUNCTION, (curl_write_callback) youtube_write_data);
    curl_easy_setopt (self->priv->curl, CURLOPT_WRITEDATA, YOUTUBE_DOWNLOAD (self));

    self->priv->stage = YOUTUBE_STAGE_DFILE;

    curl_easy_perform (self->priv->curl);

    g_print ("After Download (%d)\n", self->priv->completed);

    self->priv->state = DOWNLOAD_STATE_COMPLETED;
    _emit_download_state_changed (DOWNLOAD (self), self->priv->state);
}
