/*
 *      download-group.c
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

#include "download-group.h"

#include "download.h"

G_DEFINE_TYPE (DownloadGroup, download_group, G_TYPE_OBJECT)

struct _DownloadGroupPrivate {
    GPtrArray *downloads;
    gint state;
    gchar *name;
};

static void
on_state_changed (Download *down, gint state, DownloadGroup *self)
{
    if (state != DOWNLOAD_STATE_QUEUED && state != DOWNLOAD_STATE_RUNNING) {
        gint i;
        for (i = 0; i < self->priv->downloads->len; i++) {
            Download *d = DOWNLOAD (self->priv->downloads->pdata[i]);
            g_print ("SC State: %d\n", download_get_state (d));
            if (down == d) continue;
            if (download_get_state (d) == DOWNLOAD_STATE_QUEUED) {
                download_start (d);
                g_print ("State Changed Start\n");
                return;
            }
        }
    }
}

static void
download_group_finalize (GObject *object)
{
    DownloadGroup *self = DOWNLOAD_GROUP (object);

    G_OBJECT_CLASS (download_group_parent_class)->finalize (object);
}

static void
download_group_class_init (DownloadGroupClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (DownloadGroupPrivate));

    object_class->finalize = download_group_finalize;
}

static void
download_group_init (DownloadGroup *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), DOWNLOAD_GROUP_TYPE, DownloadGroupPrivate);

    self->priv->downloads = g_ptr_array_new ();
}

DownloadGroup*
download_group_new (const gchar *name)
{
    DownloadGroup *self = g_object_new (DOWNLOAD_GROUP_TYPE, NULL);

    self->priv->name = g_strdup (name);

    return self;
}

void
download_group_add (DownloadGroup *self, Download *d)
{
    g_ptr_array_add (self->priv->downloads, d);
    g_object_ref (d);

    g_signal_connect (d, "state-changed", G_CALLBACK (on_state_changed), self);
}

void
download_group_remove (DownloadGroup *self, Download *d)
{
    if (g_ptr_array_remove (self->priv->downloads, d)) {
        g_object_unref (d);
    }
}

void
download_group_queue (DownloadGroup *self, Download *d)
{
    gint i;
    gboolean found = FALSE;

    for (i = 0; i < self->priv->downloads->len; i++) {
        if (self->priv->downloads->pdata[i] == d) {
            found = TRUE;
            break;
        }
    }

    if (!found) {
        download_group_add (self, d);
    }

    if (download_get_state (d) != DOWNLOAD_STATE_QUEUED) {
        return;
    }

    for (i = 0; i < self->priv->downloads->len; i++) {
        if (d == self->priv->downloads->pdata[i]) continue;
        gint state = download_get_state (DOWNLOAD (self->priv->downloads->pdata[i]));
        if (state == DOWNLOAD_STATE_QUEUED || state == DOWNLOAD_STATE_RUNNING) {
            download_queue (d);
            return;
        }
    }

    download_start (d);
}
