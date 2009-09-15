/*
 *      megaupload-download.c
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

#include "megaupload-download.h"

#include "http-download.h"
#include "download.h"

static void download_init (DownloadInterface *iface);
G_DEFINE_TYPE_WITH_CODE (MegauploadDownload, megaupload_download, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (DOWNLOAD_TYPE, download_init)
)

enum {
    MEGAUPLOAD_STATE_NONE = 0,
    MEGAUPLOAD_STAGE_DFIRST,
    MEGAUPLOAD_STAGE_DSECOND,
    MEGAUPLOAD_STAGE_DTHIRD,
    MEGAUPLOAD_STAGE_DFILE,
};

typedef struct _MUCaptcha MUCaptcha;
struct _MUCaptcha {
    gchar *captchacode;
    gchar *megavar;
    gchar *img_addr;
    gchar *captcha;
};

struct _MegauploadDownloadPrivate {
    gchar *source, *dest;

    CURL *curl;
    FILE *fptr;
    GThread *main;

    GdkPixbufLoader *img_loader;

    gint size, completed;
    gint state, stage;
    time_t ot;

    MUCaptcha cap;
};

static gchar *megaupload_download_get_title (Download *self);
static gint megaupload_download_get_size_total (Download *self);
static gint megaupload_download_get_size_completed (Download *self);
static gint megaupload_download_get_time_total (Download *self);
static gint megaupload_download_get_time_remaining (Download *self);
static gboolean megaupload_download_get_state (Download *self);
static gboolean megaupload_download_start (Download *self);
static gboolean megaupload_download_stop (Download *self);
static gboolean megaupload_download_cancel (Download *self);
static gboolean megaupload_download_pause (Download *self);

gpointer megaupload_download_main (MegauploadDownload *self);
int megaupload_download_progress (MegauploadDownload *self, gdouble dt, gdouble dn, gdouble ut, gdouble un);
static size_t megaupload_download_write_data (char *buff, size_t size, size_t num, MegauploadDownload *self);

static void megaupload_download_parse_captcha (gchar *filename, MUCaptcha *cap);

static void megaupload_download_start_element (GMarkupParseContext *context,
    const gchar *element_name, const gchar **attribute_names,
    const gchar **attribute_values, gpointer user_data, GError **error);

static void
download_init (DownloadInterface *iface)
{
    iface->get_title = megaupload_download_get_title;

    iface->get_size_tot = megaupload_download_get_size_total;
    iface->get_size_comp = megaupload_download_get_size_completed;

    iface->get_time_tot = megaupload_download_get_time_total;
    iface->get_time_rem = megaupload_download_get_time_remaining;

    iface->get_state = megaupload_download_get_state;

    iface->start = megaupload_download_start;
    iface->stop = megaupload_download_stop;
    iface->cancel = megaupload_download_cancel;
    iface->pause = megaupload_download_pause;
}

static void
megaupload_download_finalize (GObject *object)
{
    MegauploadDownload *self = MEGAUPLOAD_DOWNLOAD (object);

    G_OBJECT_CLASS (megaupload_download_parent_class)->finalize (object);
}

static void
megaupload_download_class_init (MegauploadDownloadClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (MegauploadDownloadPrivate));

    object_class->finalize = megaupload_download_finalize;
}

static void
megaupload_download_init (MegauploadDownload *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), MEGAUPLOAD_DOWNLOAD_TYPE, MegauploadDownloadPrivate);
}

Download*
megaupload_download_new (const gchar *source, const gchar *dest)
{
    MegauploadDownload *self = g_object_new (MEGAUPLOAD_DOWNLOAD_TYPE, NULL);

    self->priv->source = g_strdup (source);
    self->priv->dest = g_strdup (dest);

    return DOWNLOAD (self);
}

gchar*
megaupload_download_get_title (Download *self)
{
    MegauploadDownloadPrivate *priv = MEGAUPLOAD_DOWNLOAD (self)->priv;

    switch (priv->stage) {
        case MEGAUPLOAD_STAGE_DFIRST:
            return g_strdup_printf ("Stage 1 / 4: %s", priv->source);
            break;
        case MEGAUPLOAD_STAGE_DSECOND:
            return g_strdup_printf ("Stage 2 / 4: %s", priv->source);
            break;
        case MEGAUPLOAD_STAGE_DTHIRD:
            return g_strdup_printf ("Stage 3 / 4: %s", priv->source);
            break;
        case MEGAUPLOAD_STAGE_DFILE:
            return g_strdup_printf ("Stage 4 / 4: %s", priv->source);
            break;
        default:
            return g_strdup (priv->source);
    }
}

gint
megaupload_download_get_size_total (Download *self)
{
    return MEGAUPLOAD_DOWNLOAD (self)->priv->size;
}

gint
megaupload_download_get_size_completed (Download *self)
{
    return MEGAUPLOAD_DOWNLOAD (self)->priv->completed;
}

gint
megaupload_download_get_time_total (Download *self)
{
    return -1;
}

gint
megaupload_download_get_time_remaining (Download *self)
{
    return -1;
}

gint
megaupload_download_get_state (Download *self)
{
    return MEGAUPLOAD_DOWNLOAD (self)->priv->state;
}

gboolean
megaupload_download_start (Download *self)
{
    MegauploadDownloadPrivate *priv = MEGAUPLOAD_DOWNLOAD (self)->priv;

    priv->main = g_thread_create ((GThreadFunc) megaupload_download_main,
        MEGAUPLOAD_DOWNLOAD (self), TRUE, NULL);
}

gboolean
megaupload_download_stop (Download *self)
{

}

gboolean
megaupload_download_cancel (Download *self)
{

}

gboolean
megaupload_download_pause (Download *self)
{
    MegauploadDownloadPrivate *priv = MEGAUPLOAD_DOWNLOAD (self)->priv;

    switch (priv->state) {
        case DOWNLOAD_STATE_RUNNING:
            priv->state = DOWNLOAD_STATE_PAUSED;
            g_thread_join (priv->main);
            break;
    };
}

static void
megaupload_download_parse_captcha (gchar *filename, MUCaptcha *cap)
{
    GMarkupParser get_captcha = {
        .start_element = megaupload_download_start_element,
    };

    GMarkupParseContext *context;
    GError *err;

    FILE *fptr = fopen (filename, "r");
    gchar *data = g_new0 (gchar, 1024);
    gchar *datapos;
    gint len;

    context = g_markup_parse_context_new (&get_captcha, 0, cap, NULL);
    gboolean found = FALSE;

    while ((len = fread (data, 1, 1024, fptr)) > 0) {
        if (!found) {
            gchar *str = g_strrstr_len (data, len, "<FORM method=\"POST\" id=\"captchaform\">");
            if (str) {
                found = TRUE;
                datapos = str;
            }
        } else {
            datapos = data;
        }

        if (found) {
            err = NULL;
            gint dlen = data == datapos ? len : len - (datapos - data);
            g_markup_parse_context_parse (context, datapos, dlen, &err);
            if (err) {
                g_error_free (err);
                break;
            }
        }
    }

    fclose (fptr);
}

static void
megaupload_download_get_captcha (GtkWidget *img, MUCaptcha *cap)
{
    gdk_threads_enter ();
    GtkWidget *dialog = gtk_dialog_new_with_buttons ("Enter Captcha", NULL,
        GTK_DIALOG_MODAL,
        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
        GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
        NULL);

    GtkWidget *cont = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

    GtkWidget *vbox = gtk_vbox_new (FALSE, 0);

    GtkWidget *field = gtk_entry_new ();

    gtk_box_pack_start (GTK_BOX (vbox), img, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), field, FALSE, FALSE, 0);

    gtk_container_add (GTK_CONTAINER (cont), vbox);

    gtk_widget_show_all (dialog);

    gint resp = gtk_dialog_run (GTK_DIALOG (dialog));

    if (resp == GTK_RESPONSE_ACCEPT) {
        cap->captcha = g_strdup (gtk_entry_get_text (GTK_ENTRY (field)));
    }

    gtk_widget_destroy (dialog);
    gdk_threads_leave ();
}

static gchar*
megaupload_download_parse_dl_link (gchar *filename)
{
    GMarkupParseContext *context;

    FILE *fptr = fopen (filename, "r");
    gchar *data = g_new0 (gchar, 5*1024);
    gchar *url = NULL;
    gint len, i;

    while ((len = fread (data, 1, 5*1024, fptr)) > 0) {
        gchar *str = g_strstr_len (data, len, "downloadlink");
        if (str) {
            for (i = 23; i < len - (str-data); i++) {
                if (str[i] == '\"') {
                    str[i] = '\0';
                }
            }

            url = g_strdup (str+23);
            break;
        }
    }

    g_free (data);
    fclose (fptr);

    return url;
}

static void
megaupload_download_start_element (GMarkupParseContext *context,
    const gchar *element_name, const gchar **attribute_names,
    const gchar **attribute_values, gpointer user_data, GError **error)
{
    MUCaptcha *muc = (MUCaptcha*) user_data;

    if (!g_strcmp0 (element_name, "INPUT")) {
        if (!g_strcmp0 (attribute_values[1], "captchacode")) {
            muc->captchacode = g_strdup (attribute_values[2]);
        } else if (!g_strcmp0 (attribute_values[1], "megavar")) {
            muc->megavar = g_strdup (attribute_values[2]);
        }
    } else if (!g_strcmp0 (element_name, "img")) {
        muc->img_addr = g_strdup (attribute_values[0]);
    }
}

int
megaupload_download_progress (MegauploadDownload *self, gdouble dt, gdouble dn, gdouble ut, gdouble un)
{
    if (self->priv->state != DOWNLOAD_STATE_RUNNING)
        return -1;

    time_t nt = time (NULL);

    if (nt != self->priv->ot) {
        self->priv->ot = nt;
        _emit_download_position_changed (DOWNLOAD (self));
    }

    return 0;
}

static size_t
megaupload_download_write_data (char *buff, size_t size, size_t num, MegauploadDownload *self)
{
    GError *err = NULL;

    if (self->priv->state != DOWNLOAD_STATE_RUNNING) {
        return -1;
    }

    switch (self->priv->stage) {
        case MEGAUPLOAD_STAGE_DSECOND:
            fwrite (buff, size, num, self->priv->fptr);
            gdk_pixbuf_loader_write (self->priv->img_loader, buff, size * num, &err);
            if (err) {
                g_print ("Error Loading img: %s\n", err->message);
                g_error_free (err);
                err = NULL;
                return -1;
            }
            break;
        case MEGAUPLOAD_STAGE_DFILE:
            fwrite (buff, size, num, self->priv->fptr);
            self->priv->completed += size * num;
        default:
            fwrite (buff, size, num, self->priv->fptr);
    }

    return size * num;
}

gpointer
megaupload_download_main (MegauploadDownload *self)
{
    gint i = 0;

    self->priv->curl = curl_easy_init ();

    while (self->priv->source[i++]);
    while (self->priv->source[--i] != '=');

    gchar *filename = g_strdup_printf ("/tmp/mu%s.html", self->priv->source+i+1);
    self->priv->fptr = fopen (filename, "w");

    curl_easy_setopt (self->priv->curl, CURLOPT_URL, self->priv->source);

    curl_easy_setopt (self->priv->curl, CURLOPT_WRITEFUNCTION, (curl_write_callback) megaupload_download_write_data);
    curl_easy_setopt (self->priv->curl, CURLOPT_WRITEDATA, self);

    curl_easy_setopt (self->priv->curl, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt (self->priv->curl, CURLOPT_PROGRESSFUNCTION, (curl_progress_callback) megaupload_download_progress);
    curl_easy_setopt (self->priv->curl, CURLOPT_PROGRESSDATA, self);

    self->priv->state = DOWNLOAD_STATE_RUNNING;
    self->priv->stage = MEGAUPLOAD_STAGE_DFIRST;
    _emit_download_state_changed (DOWNLOAD (self), self->priv->state);

    curl_easy_perform (self->priv->curl);
    fclose (self->priv->fptr);

    megaupload_download_parse_captcha (filename, &self->priv->cap);
    g_free (filename);

    curl_easy_setopt (self->priv->curl, CURLOPT_URL, self->priv->cap.img_addr);

    self->priv->stage = MEGAUPLOAD_STAGE_DSECOND;
    self->priv->img_loader = gdk_pixbuf_loader_new_with_type ("gif", NULL);
    filename = g_strdup_printf ("/tmp/mu%s.gif", self->priv->source+i+1);
    self->priv->fptr = fopen (filename, "w");
    g_free (filename);

    curl_easy_perform (self->priv->curl);

    fclose (self->priv->fptr);

    GError *err = NULL;
    gdk_pixbuf_loader_close (self->priv->img_loader, &err);
    if (err) {
        g_print ("Error processing img: %s\n", err->message);
        g_error_free (err);
        err = NULL;
    }

    GdkPixbuf *img = gdk_pixbuf_loader_get_pixbuf (self->priv->img_loader);
    GtkWidget *img_w = gtk_image_new_from_pixbuf (img);

    megaupload_download_get_captcha (img_w, &self->priv->cap);

    g_object_unref (img_w);

    filename = g_strdup_printf ("/tmp/mu%s-2.html", self->priv->source+i+1);
    self->priv->fptr = fopen (filename, "w");

    curl_easy_setopt (self->priv->curl, CURLOPT_VERBOSE, 1);
    curl_easy_setopt (self->priv->curl, CURLOPT_URL, self->priv->source);

    gchar *str = g_strdup_printf ("captcha=%s&captchacode=%s&megavar=%s",
        self->priv->cap.captcha, self->priv->cap.captchacode, self->priv->cap.megavar);
    curl_easy_setopt (self->priv->curl, CURLOPT_POST, 1);
    curl_easy_setopt (self->priv->curl, CURLOPT_POSTFIELDS, str);
    curl_easy_setopt (self->priv->curl, CURLOPT_REFERER, self->priv->source);

    self->priv->stage = MEGAUPLOAD_STAGE_DTHIRD;
    _emit_download_state_changed (DOWNLOAD (self), self->priv->state);

    curl_easy_perform (self->priv->curl);
    g_free (str);
    fclose (self->priv->fptr);

    gchar *name = megaupload_download_parse_dl_link (filename);
    g_free (filename);

    gchar *newdest = NULL;
    if (self->priv->dest[0] == '/') {
        newdest = g_strdup (self->priv->dest);
    } else if (self->priv->dest[0] == '~') {
        newdest = g_build_filename (g_get_home_dir (), self->priv->dest+2, NULL);
    } else {
        newdest = g_build_filename (g_get_tmp_dir (), self->priv->dest, NULL);
    }

    if (g_file_test (newdest, G_FILE_TEST_IS_DIR)) {
        gint len = strlen (name);
        while (name[--len] != '/');
        newdest = g_build_filename (newdest, name + len, NULL);
    }

    g_free (self->priv->dest);
    self->priv->dest = newdest;

    g_print ("URL: %s\n", name);
    g_print ("DEST: %s\n", self->priv->dest);
/*
    // Get file length in a HEAD request
    curl_easy_setopt (self->priv->curl, CURLOPT_URL, name);
    curl_easy_setopt (self->priv->curl, CURLOPT_HTTPGET, 1);
    curl_easy_setopt (self->priv->curl, CURLOPT_NOBODY, 1);
    curl_easy_setopt (self->priv->curl, CURLOPT_REFERER, self->priv->source);

    curl_easy_perform (self->priv->curl);
    g_print ("Post Perform\n");

    gdouble cl;
    curl_easy_getinfo (self->priv->curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &cl);

    struct stat ostat;
    g_stat (self->priv->dest, &ostat);

    g_print ("CL: %f\n", cl);
    g_print ("STAT: %d\n", ostat.st_size);

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
*/
    self->priv->fptr = fopen (self->priv->dest, "w");

    curl_easy_setopt (self->priv->curl, CURLOPT_URL, name);
    curl_easy_setopt (self->priv->curl, CURLOPT_HTTPGET, 1);
    curl_easy_setopt (self->priv->curl, CURLOPT_NOBODY, 0);

    self->priv->stage = MEGAUPLOAD_STAGE_DFILE;
    _emit_download_state_changed (DOWNLOAD (self), self->priv->state);

    g_print ("Starting Download\n");
    curl_easy_perform (self->priv->curl);
    g_print ("Stopped Download\n");
    g_free (name);

    fclose (self->priv->fptr);

    self->priv->state = DOWNLOAD_STATE_COMPLETED;
    self->priv->stage = MEGAUPLOAD_STATE_NONE;
    _emit_download_state_changed (DOWNLOAD (self), self->priv->state);
}
