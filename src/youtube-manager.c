/*
 *      youtube-manager.c
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

#include "youtube-manager.h"

G_DEFINE_TYPE (YoutubeManager, youtube_manager, G_TYPE_OBJECT)

struct _YoutubeManagerPrivate {
    GPtrArray *downloads;
};

static void
youtube_manager_finalize (GObject *object)
{
    YoutubeManager *self = YOUTUBE_MANAGER (object);

    G_OBJECT_CLASS (youtube_manager_parent_class)->finalize (object);
}

static void
youtube_manager_class_init (YoutubeManagerClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (YoutubeManagerPrivate));

    object_class->finalize = youtube_manager_finalize;
}

static void
youtube_manager_init (YoutubeManager *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), YOUTUBE_MANAGER_TYPE, YoutubeManagerPrivate);
}

YoutubeManager*
youtube_manager_new ()
{
    YoutubeManager *self = g_object_new (YOUTUBE_MANAGER_TYPE, NULL);

    return self;
}
