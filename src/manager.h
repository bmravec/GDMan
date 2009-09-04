/*
 *      manager.h
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

#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <glib-object.h>

#define MANAGER_DBUS_SERVICE "org.gnome.GDMan"
#define MANAGER_DBUS_PATH "/org/gnome/GDMan/Manager"

#define MANAGER_TYPE (manager_get_type ())
#define MANAGER(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), MANAGER_TYPE, Manager))
#define MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MANAGER_TYPE, ManagerClass))
#define IS_MANAGER(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), MANAGER_TYPE))
#define IS_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MANAGER_TYPE))
#define MANAGER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), MANAGER_TYPE, ManagerClass))

G_BEGIN_DECLS

typedef struct _Manager Manager;
typedef struct _ManagerClass ManagerClass;
typedef struct _ManagerPrivate ManagerPrivate;

struct _Manager {
    GObject parent;

    ManagerPrivate *priv;
};

struct _ManagerClass {
    GObjectClass parent;
};


Manager *manager_new ();
GType manager_get_type (void);

void manager_run (Manager *self);
void manager_stop (Manager *self);

gboolean manager_add_download (Manager *self, gchar *url, gchar *dest, guint *ident, GError **error);

G_END_DECLS

#endif /* __MANAGER_H__ */
