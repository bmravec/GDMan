/*
 *      megaupload-manager.h
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

#ifndef __MEGAUPLOAD_MANAGER_H__
#define __MEGAUPLOAD_MANAGER_H__

#include <glib-object.h>

#define MEGAUPLOAD_MANAGER_TYPE (megaupload_manager_get_type ())
#define MEGAUPLOAD_MANAGER(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), MEGAUPLOAD_MANAGER_TYPE, MegauploadManager))
#define MEGAUPLOAD_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MEGAUPLOAD_MANAGER_TYPE, MegauploadManagerClass))
#define IS_MEGAUPLOAD_MANAGER(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), MEGAUPLOAD_MANAGER_TYPE))
#define IS_MEGAUPLOAD_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MEGAUPLOAD_MANAGER_TYPE))
#define MEGAUPLOAD_MANAGER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), MEGAUPLOAD_MANAGER_TYPE, MegauploadManagerClass))

G_BEGIN_DECLS

typedef struct _MegauploadManager MegauploadManager;
typedef struct _MegauploadManagerClass MegauploadManagerClass;
typedef struct _MegauploadManagerPrivate MegauploadManagerPrivate;

struct _MegauploadManager {
    GObject parent;

    MegauploadManagerPrivate *priv;
};

struct _MegauploadManagerClass {
    GObjectClass parent;
};

MegauploadManager *megaupload_manager_new ();
GType megaupload_manager_get_type (void);

G_END_DECLS

#endif /* __MEGAUPLOAD_MANAGER_H__ */
