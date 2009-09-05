/*
 *      http-download.h
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

#ifndef __HTTP_DOWNLOAD_H__
#define __HTTP_DOWNLOAD_H__

#include <glib-object.h>

#include "download.h"

#define HTTP_DOWNLOAD_TYPE (http_download_get_type ())
#define HTTP_DOWNLOAD(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), HTTP_DOWNLOAD_TYPE, HttpDownload))
#define HTTP_DOWNLOAD_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), HTTP_DOWNLOAD_TYPE, HttpDownloadClass))
#define IS_HTTP_DOWNLOAD(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), HTTP_DOWNLOAD_TYPE))
#define IS_HTTP_DOWNLOAD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HTTP_DOWNLOAD_TYPE))
#define HTTP_DOWNLOAD_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), HTTP_DOWNLOAD_TYPE, HttpDownloadClass))

G_BEGIN_DECLS

typedef struct _HttpDownload HttpDownload;
typedef struct _HttpDownloadClass HttpDownloadClass;
typedef struct _HttpDownloadPrivate HttpDownloadPrivate;

struct _HttpDownload {
    GObject parent;

    HttpDownloadPrivate *priv;
};

struct _HttpDownloadClass {
    GObjectClass parent;
};

Download *http_download_new (const gchar *source, const gchar *dest);
GType http_download_get_type (void);

G_END_DECLS

#endif /* __HTTP_DOWNLOAD_H__ */
