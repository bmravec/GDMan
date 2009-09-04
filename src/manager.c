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

#include <gtk/gtk.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include "manager.h"
#include "manager-glue.h"

G_DEFINE_TYPE(Manager, manager, G_TYPE_OBJECT)

struct _ManagerPrivate {
    GtkBuilder *builder;

    GtkWidget *window;
    GtkWidget *view;
    GtkWidget *status;

    GtkTreeModel *store;

    GtkStatusIcon *icon;

    DBusGConnection *conn;
    DBusGProxy *proxy;

    guint new_id;
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

    self->priv->builder = gtk_builder_new ();
    gtk_builder_add_from_file (self->priv->builder,
        // SHAREDIR "/ui/main.ui", NULL);
        "data/ui/main.ui", NULL);

    self->priv->window = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "main_window"));
    self->priv->view = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "main_view"));
    self->priv->status = GTK_WIDGET (gtk_builder_get_object (self->priv->builder, "main_status"));

    self->priv->store = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_OBJECT));
    gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->view), self->priv->store);

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    renderer = gtk_cell_renderer_progress_new ();
    column = gtk_tree_view_column_new_with_attributes ("Progress", renderer, NULL);
//    gtk_tree_view_column_set_cell_data_func (column, renderer,
//        (GtkTreeCellDataFunc) status_column_func, NULL, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->view), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Download", renderer, NULL);
//    gtk_tree_view_column_set_cell_data_func (column, renderer,
//        (GtkTreeCellDataFunc) status_column_func, NULL, NULL);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->view), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("remaining", renderer, NULL);
//    gtk_tree_view_column_set_cell_data_func (column, renderer,
//        (GtkTreeCellDataFunc) status_column_func, NULL, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->view), column);

    GtkTreeIter iter;
    gtk_list_store_append (GTK_LIST_STORE (self->priv->store), &iter);

    g_signal_connect (self->priv->window, "destroy", G_CALLBACK (manager_stop), NULL);

    self->priv->icon = gtk_status_icon_new_from_stock (GTK_STOCK_GO_DOWN);

    gtk_widget_show_all (self->priv->window);

    self->priv->new_id = 1;

    self->priv->conn = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
    self->priv->proxy = dbus_g_proxy_new_for_name (self->priv->conn,
        DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

    org_freedesktop_DBus_request_name (self->priv->proxy,
        MANAGER_DBUS_SERVICE, DBUS_NAME_FLAG_DO_NOT_QUEUE, NULL, NULL);

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
   gtk_main ();
}

void
manager_stop (Manager *self)
{
    gtk_main_quit ();
}

gboolean
manager_add_download (Manager *self, gchar *url, gchar *dest, guint *ident, GError **error)
{
    g_print ("Manager Add Download %s -> %s\n", url, dest);

    *ident = self->priv->new_id++;
}

int
main (int argc, char *argv[])
{
    Manager *manager;

    gtk_init (&argc, &argv);

    manager = manager_new ();

    manager_run (manager);
}
