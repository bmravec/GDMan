/*
 *      download.h
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

#ifndef __DOWNLOAD_H__
#define __DOWNLOAD_H__

#include <gtk/gtk.h>

#define DOWNLOAD_TYPE (download_get_type ())
#define DOWNLOAD(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), DOWNLOAD_TYPE, Download))
#define IS_DOWNLOAD(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), DOWNLOAD_TYPE))
#define DOWNLOAD_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), DOWNLOAD_TYPE, DownloadInterface))

G_BEGIN_DECLS

typedef struct _Download Download;
typedef struct _DownloadInterface DownloadInterface;

struct _DownloadInterface {
    GTypeInterface parent;

    gchar* (*get_title) (Download *self);

    gint   (*get_size_tot) (Download *self);
    gint   (*get_size_rem) (Download *self);

    gint   (*get_time_tot) (Download *self);
    gint   (*get_time_rem) (Download *self);

    gint   (*get_percentage) (Download *self);
    gboolean (*is_completed) (Download *self);
};

GType download_get_type (void);

gchar *download_get_title (Download *self);

gint download_get_size_total (Download *self);
gint download_get_size_remaining (Download *self);

gint download_get_time_total (Download *self);
gint download_get_time_remaining (Download *self);

gint download_get_percentage (Download *self);
gboolean download_is_complete (Download *self);

G_END_DECLS

#endif /* __DOWNLOAD_H__ */
