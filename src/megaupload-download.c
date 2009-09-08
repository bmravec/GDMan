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

#include "megaupload-download.h"

#include "http-download.h"
#include "download.h"

static void download_init (DownloadInterface *iface);
G_DEFINE_TYPE_WITH_CODE (MegauploadDownload, megaupload_download, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (DOWNLOAD_TYPE, download_init)
)

enum {
    MEGAUPLOAD_STAGE_DFIRST = 0,
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
    gchar *source, *dest, *title;
    gchar *first_file, *second_file, *third_file;

    Download *down;

    gint state, stage;

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

static void megaupload_parse_captcha (gchar *filename, MUCaptcha *cap);

static void megaupload_start_element (GMarkupParseContext *context,
    const gchar *element_name, const gchar **attribute_names,
    const gchar **attribute_values, gpointer user_data, GError **error);

static void on_pos_changed (Download *download, MegauploadDownload *self);
static void on_state_changed (Download *download, guint state, MegauploadDownload *self);

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

    gint i = strlen (source);
    while (source[i--] != '=');

    g_print ("Title: %s\n", source+i+2);
    self->priv->title = g_strdup (source+i+2);
    self->priv->first_file = g_strdup_printf ("/tmp/mu%s.html", source+i+2);

    self->priv->down = http_download_new (source, self->priv->first_file, FALSE);
    g_signal_connect (self->priv->down, "position-changed", G_CALLBACK (on_pos_changed), self);
    g_signal_connect (self->priv->down, "state-changed", G_CALLBACK (on_state_changed), self);

    download_start (self->priv->down);

    self->priv->stage = MEGAUPLOAD_STAGE_DFIRST;

    return DOWNLOAD (self);
}

gchar*
megaupload_download_get_title (Download *self)
{
    MegauploadDownloadPrivate *priv = MEGAUPLOAD_DOWNLOAD (self)->priv;

    switch (priv->stage) {
        case MEGAUPLOAD_STAGE_DFIRST:
            return g_strdup_printf ("Stage 1 / 4: %s", download_get_title (priv->down));
            break;
        case MEGAUPLOAD_STAGE_DSECOND:
            return g_strdup_printf ("Stage 2 / 4: %s", download_get_title (priv->down));
            break;
        case MEGAUPLOAD_STAGE_DTHIRD:
            return g_strdup_printf ("Stage 3 / 4: %s", download_get_title (priv->down));
            break;
        case MEGAUPLOAD_STAGE_DFILE:
            return g_strdup_printf ("Stage 4 / 4: %s", download_get_title (priv->down));
            break;
        default:
            return NULL;
    }
}

gint
megaupload_download_get_size_total (Download *self)
{
    return download_get_size_total (MEGAUPLOAD_DOWNLOAD (self)->priv->down);
}

gint
megaupload_download_get_size_completed (Download *self)
{
    return download_get_size_completed (MEGAUPLOAD_DOWNLOAD (self)->priv->down);
}

gint
megaupload_download_get_time_total (Download *self)
{

}

gint
megaupload_download_get_time_remaining (Download *self)
{
    return download_get_time_remaining (MEGAUPLOAD_DOWNLOAD (self)->priv->down);
}

gint
megaupload_download_get_state (Download *self)
{
//    return MEGAUPLOAD_DOWNLOAD (self)->priv->state;
}

gboolean
megaupload_download_start (Download *self)
{
/*
    g_thread_create ((GThreadFunc) megaupload_download_main,
        MEGAUPLOAD_DOWNLOAD (self), FALSE, NULL);
    g_timeout_add (1000, (GSourceFunc) on_timeout, self);
*/
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

}

static void
on_pos_changed (Download *download, MegauploadDownload *self)
{
    _emit_download_position_changed (DOWNLOAD (self));
}

static void
megaupload_parse_captcha (gchar *filename, MUCaptcha *cap)
{
    GMarkupParser get_captcha = {
        .start_element = megaupload_start_element,
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
megaupload_get_captcha (MegauploadDownload *self)
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons ("Enter Captcha", NULL,
        GTK_DIALOG_MODAL,
        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
        GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
        NULL);

    GtkWidget *cont = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

    GtkWidget *vbox = gtk_vbox_new (FALSE, 0);

    GtkWidget *field = gtk_entry_new ();

    GtkWidget *img = gtk_image_new_from_file (self->priv->second_file);

    gtk_box_pack_start (GTK_BOX (vbox), img, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), field, FALSE, FALSE, 0);

    gtk_container_add (GTK_CONTAINER (cont), vbox);

    gtk_widget_show_all (dialog);

    gint resp = gtk_dialog_run (GTK_DIALOG (dialog));

    if (resp == GTK_RESPONSE_ACCEPT) {
        self->priv->cap.captcha = g_strdup (gtk_entry_get_text (GTK_ENTRY (field)));
    }

    gtk_widget_destroy (dialog);
}

static gchar*
megaupload_parse_dl_link (gchar *filename)
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

static gboolean
megaupload_display_dialog (MegauploadDownload *self)
{
    megaupload_get_captcha (self);

    g_object_unref (self->priv->down);

    self->priv->third_file = g_strdup_printf ("/tmp/mu%s-2.html", self->priv->title);
    self->priv->down = http_download_new (self->priv->source, self->priv->third_file, FALSE);
    g_signal_connect (self->priv->down, "position-changed", G_CALLBACK (on_pos_changed), self);
    g_signal_connect (self->priv->down, "state-changed", G_CALLBACK (on_state_changed), self);

    gchar *str = g_strdup_printf ("captcha=%s&captchacode=%s&megavar=%s",
        self->priv->cap.captcha, self->priv->cap.captchacode, self->priv->cap.megavar);

    http_download_set_post (HTTP_DOWNLOAD (self->priv->down), str);

    self->priv->stage = MEGAUPLOAD_STAGE_DTHIRD;

    download_start (self->priv->down);

    return FALSE;
}

static void
on_state_changed (Download *download, guint state, MegauploadDownload *self)
{
    gchar *str;
    gint len;

    if (state == DOWNLOAD_STATE_COMPLETED) {
        switch (self->priv->stage) {
            case MEGAUPLOAD_STAGE_DFIRST:
                megaupload_parse_captcha (self->priv->first_file, &self->priv->cap);

                g_object_unref (self->priv->down);

                self->priv->second_file = g_strdup_printf ("/tmp/mu%s.gif", self->priv->title);

                self->priv->down = http_download_new (self->priv->cap.img_addr, self->priv->second_file, FALSE);
                g_signal_connect (self->priv->down, "position-changed", G_CALLBACK (on_pos_changed), self);
                g_signal_connect (self->priv->down, "state-changed", G_CALLBACK (on_state_changed), self);

                download_start (self->priv->down);

                self->priv->stage = MEGAUPLOAD_STAGE_DSECOND;

                break;
            case MEGAUPLOAD_STAGE_DSECOND:
                gdk_threads_add_timeout (250, (GSourceFunc) megaupload_display_dialog, self);

                break;
            case MEGAUPLOAD_STAGE_DTHIRD:
                g_object_unref (self->priv->down);

                str = megaupload_parse_dl_link (self->priv->third_file);

                self->priv->down = http_download_new (str, self->priv->dest, FALSE);

                g_signal_connect (self->priv->down, "position-changed", G_CALLBACK (on_pos_changed), self);
                g_signal_connect (self->priv->down, "state-changed", G_CALLBACK (on_state_changed), self);

                self->priv->stage = MEGAUPLOAD_STAGE_DFILE;

                http_download_set_referer (HTTP_DOWNLOAD (self->priv->down), self->priv->source);

                download_start (self->priv->down);

                break;
            case MEGAUPLOAD_STAGE_DFILE:

                break;
        }
    }
}

static void
megaupload_start_element (GMarkupParseContext *context,
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
