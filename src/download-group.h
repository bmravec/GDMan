/*
 *      download-group.h
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

#ifndef __DOWNLOAD_GROUP_H__
#define __DOWNLOAD_GROUP_H__

#include <glib-object.h>

#include "download.h"

#define DOWNLOAD_GROUP_TYPE (download_group_get_type ())
#define DOWNLOAD_GROUP(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), DOWNLOAD_GROUP_TYPE, DownloadGroup))
#define DOWNLOAD_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), DOWNLOAD_GROUP_TYPE, DownloadGroupClass))
#define IS_DOWNLOAD_GROUP(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), DOWNLOAD_GROUP_TYPE))
#define IS_DOWNLOAD_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DOWNLOAD_GROUP_TYPE))
#define DOWNLOAD_GROUP_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DOWNLOAD_GROUP_TYPE, DownloadGroupClass))

G_BEGIN_DECLS

typedef struct _DownloadGroup DownloadGroup;
typedef struct _DownloadGroupClass DownloadGroupClass;
typedef struct _DownloadGroupPrivate DownloadGroupPrivate;

struct _DownloadGroup {
    GObject parent;

    DownloadGroupPrivate *priv;
};

struct _DownloadGroupClass {
    GObjectClass parent;
};

DownloadGroup *download_group_new (const gchar *name);

void download_group_add (DownloadGroup *self, Download *d);
void download_group_remove (DownloadGroup *self, Download *d);

void download_group_queue (DownloadGroup *self, Download *d);

GType download_group_get_type (void);

G_END_DECLS

#endif /* __DOWNLOAD_GROUP_H__ */
