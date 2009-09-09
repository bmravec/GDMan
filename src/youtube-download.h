/*
 *      youtube-download.h
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

#ifndef __YOUTUBE_DOWNLOAD_H__
#define __YOUTUBE_DOWNLOAD_H__

#include <glib-object.h>

#include "download.h"

#define YOUTUBE_DOWNLOAD_TYPE (youtube_download_get_type ())
#define YOUTUBE_DOWNLOAD(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), YOUTUBE_DOWNLOAD_TYPE, YoutubeDownload))
#define YOUTUBE_DOWNLOAD_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), YOUTUBE_DOWNLOAD_TYPE, YoutubeDownloadClass))
#define IS_YOUTUBE_DOWNLOAD(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), YOUTUBE_DOWNLOAD_TYPE))
#define IS_YOUTUBE_DOWNLOAD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), YOUTUBE_DOWNLOAD_TYPE))
#define YOUTUBE_DOWNLOAD_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), YOUTUBE_DOWNLOAD_TYPE, YoutubeDownloadClass))

G_BEGIN_DECLS

typedef struct _YoutubeDownload YoutubeDownload;
typedef struct _YoutubeDownloadClass YoutubeDownloadClass;
typedef struct _YoutubeDownloadPrivate YoutubeDownloadPrivate;

struct _YoutubeDownload {
    GObject parent;

    YoutubeDownloadPrivate *priv;
};

struct _YoutubeDownloadClass {
    GObjectClass parent;
};

Download *youtube_download_new (const gchar *source, const gchar *dest);
Download *youtube_download_new_from_file (const gchar *filename);

GType youtube_download_get_type (void);

G_END_DECLS

#endif /* __YOUTUBE_DOWNLOAD_H__ */
