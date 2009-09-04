/*
 *      manager.c
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

#include <glib.h>
#include <glib/gthread.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include "manager.h"
#include "manager-glue.h"

G_DEFINE_TYPE(Manager, manager, G_TYPE_OBJECT)

struct _ManagerPrivate {
    GMainLoop *loop;
    guint ref_cnt;

    DBusGConnection *conn;
    DBusGProxy *proxy;

    GHashTable *mos;
};

static guint signal_add;
static guint signal_remove;

static void
manager_finalize (GObject *object)
{
    Manager *self = MANAGER (object);

    G_OBJECT_CLASS (manager_parent_class)->finalize (object);
}

static void
manager_class_init (ManagerClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (ManagerPrivate));

    object_class->finalize = manager_finalize;

    signal_add = g_signal_new ("add-entry", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__UINT,
        G_TYPE_NONE, 1, G_TYPE_UINT);

    signal_remove = g_signal_new ("remove-entry", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__UINT,
        G_TYPE_NONE, 1, G_TYPE_UINT);

    dbus_g_object_type_install_info (MANAGER_TYPE,
                                     &dbus_glib_manager_object_info);
}

static void
manager_init (Manager *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), MANAGER_TYPE, ManagerPrivate);

    self->priv->ref_cnt = 0;
    self->priv->loop = g_main_loop_new (NULL, FALSE);

    self->priv->conn = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
    self->priv->proxy = dbus_g_proxy_new_for_name (self->priv->conn,
        DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

    org_freedesktop_DBus_request_name (self->priv->proxy,
        MANAGER_DBUS_SERVICE, DBUS_NAME_FLAG_DO_NOT_QUEUE, NULL, NULL);

    self->priv->mos = g_hash_table_new_full (g_str_hash,
        g_str_equal, g_free, g_object_unref);

    dbus_g_connection_register_g_object (self->priv->conn,
        MANAGER_DBUS_PATH, G_OBJECT (self));
}

Manager*
manager_new ()
{
    return g_object_new (MANAGER_TYPE, NULL);
}

void
manager_run (Manager *self)
{
   g_main_loop_run (self->priv->loop);
}

void
manager_stop (Manager *self)
{
   g_main_loop_quit (self->priv->loop);
}

gboolean
manager_add_download (Manager *self, gchar *url, gchar *dest, GError **error)
{

}


int
main (int argc, char *argv[])
{
    Manager *gdb;

    g_type_init ();

//    dbus_g_thread_init ();

    gdb = manager_new ();

    manager_run (gdb);
}
