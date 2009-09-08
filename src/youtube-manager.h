/*
 *      youtube-manager.h
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

#ifndef __YOUTUBE_MANAGER_H__
#define __YOUTUBE_MANAGER_H__

#include <glib-object.h>

#define YOUTUBE_MANAGER_TYPE (youtube_manager_get_type ())
#define YOUTUBE_MANAGER(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), YOUTUBE_MANAGER_TYPE, YoutubeManager))
#define YOUTUBE_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), YOUTUBE_MANAGER_TYPE, YoutubeManagerClass))
#define IS_YOUTUBE_MANAGER(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), YOUTUBE_MANAGER_TYPE))
#define IS_YOUTUBE_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), YOUTUBE_MANAGER_TYPE))
#define YOUTUBE_MANAGER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), YOUTUBE_MANAGER_TYPE, YoutubeManagerClass))

G_BEGIN_DECLS

typedef struct _YoutubeManager YoutubeManager;
typedef struct _YoutubeManagerClass YoutubeManagerClass;
typedef struct _YoutubeManagerPrivate YoutubeManagerPrivate;

struct _YoutubeManager {
    GObject parent;

    YoutubeManagerPrivate *priv;
};

struct _YoutubeManagerClass {
    GObjectClass parent;
};

YoutubeManager *youtube_manager_new ();
GType youtube_manager_get_type ();

G_END_DECLS

#endif /* __YOUTUBE_MANAGER_H__ */
