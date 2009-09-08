/*
 *      megaupload-download.h
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

#ifndef __MEGAUPLOAD_DOWNLOAD_H__
#define __MEGAUPLOAD_DOWNLOAD_H__

#include <glib-object.h>

#include "download.h"

#define MEGAUPLOAD_DOWNLOAD_TYPE (megaupload_download_get_type ())
#define MEGAUPLOAD_DOWNLOAD(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), MEGAUPLOAD_DOWNLOAD_TYPE, MegauploadDownload))
#define MEGAUPLOAD_DOWNLOAD_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MEGAUPLOAD_DOWNLOAD_TYPE, MegauploadDownloadClass))
#define IS_MEGAUPLOAD_DOWNLOAD(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), MEGAUPLOAD_DOWNLOAD_TYPE))
#define IS_MEGAUPLOAD_DOWNLOAD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MEGAUPLOAD_DOWNLOAD_TYPE))
#define MEGAUPLOAD_DOWNLOAD_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), MEGAUPLOAD_DOWNLOAD_TYPE, MegauploadDownloadClass))

G_BEGIN_DECLS

typedef struct _MegauploadDownload MegauploadDownload;
typedef struct _MegauploadDownloadClass MegauploadDownloadClass;
typedef struct _MegauploadDownloadPrivate MegauploadDownloadPrivate;

struct _MegauploadDownload {
    GObject parent;

    MegauploadDownloadPrivate *priv;
};

struct _MegauploadDownloadClass {
    GObjectClass parent;
};

Download *megaupload_download_new (const gchar *source, const gchar *dest);
GType megaupload_download_get_type (void);

G_END_DECLS

#endif /* __MEGAUPLOAD_DOWNLOAD_H__ */
