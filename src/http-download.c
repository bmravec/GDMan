/*
 *      http-download.c
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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "http-download.h"

#include "download.h"

static void download_init (DownloadInterface *iface);
G_DEFINE_TYPE_WITH_CODE (HttpDownload, http_download, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (DOWNLOAD_TYPE, download_init)
)

struct _HttpDownloadPrivate {
    gchar *source, *dest;

    gchar *title;
    gchar *host, *path;
    gint port;

    gint size, completed;
    gint rate;

    gint prev_time, prev_comp;

    gboolean allow_bytes;
    gboolean running;

    gint total_time;

    gint state;
};

static gchar *http_download_get_title (Download *self);
static gint http_download_get_size_total (Download *self);
static gint http_download_get_size_completed (Download *self);
static gint http_download_get_time_total (Download *self);
static gint http_download_get_time_remaining (Download *self);
static gboolean http_download_get_state (Download *self);
static gboolean http_download_start (Download *self);
static gboolean http_download_stop (Download *self);
static gboolean http_download_cancel (Download *self);
static gboolean http_download_pause (Download *self);
gpointer http_download_main (HttpDownload *self);

static int socket_connect (char *host, int port);

static void
download_init (DownloadInterface *iface)
{
    iface->get_title = http_download_get_title;

    iface->get_size_tot = http_download_get_size_total;
    iface->get_size_comp = http_download_get_size_completed;

    iface->get_time_tot = http_download_get_time_total;
    iface->get_time_rem = http_download_get_time_remaining;

    iface->get_state = http_download_get_state;

    iface->start = http_download_start;
    iface->stop = http_download_stop;
    iface->cancel = http_download_cancel;
    iface->pause = http_download_pause;
}

static void
http_download_finalize (GObject *object)
{
    HttpDownload *self = HTTP_DOWNLOAD (object);

    G_OBJECT_CLASS (http_download_parent_class)->finalize (object);
}

static void
http_download_class_init (HttpDownloadClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (HttpDownloadPrivate));

    object_class->finalize = http_download_finalize;
}

static void
http_download_init (HttpDownload *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), HTTP_DOWNLOAD_TYPE, HttpDownloadPrivate);

    self->priv->allow_bytes = FALSE;
    self->priv->source = NULL;
    self->priv->dest = NULL;
    self->priv->host = NULL;
    self->priv->path = NULL;

    self->priv->size = -1;
    self->priv->completed = 0;
    self->priv->total_time = 0;
}

Download*
http_download_new (const gchar *source, const gchar *dest)
{
    HttpDownload *self = g_object_new (HTTP_DOWNLOAD_TYPE, NULL);

    self->priv->source = g_strdup (source);

    if (dest[0] == '/') {
        self->priv->dest = g_strdup (dest);
    } else if (dest[0] == '~') {
        self->priv->dest = g_build_filename (g_get_home_dir (), dest+2);
    } else {
        self->priv->dest = g_build_filename (g_get_tmp_dir (), dest);
    }

    self->priv->title = g_path_get_basename (dest);

    int sockfd, i;

    gchar **proto_strs = g_strsplit (self->priv->source, "://", 2);
    gchar **host_strs = g_strsplit (proto_strs[1], "/", 2);
    gchar **port_strs = g_strsplit (host_strs[0], ":", 2);

    if (port_strs[1]) {
        self->priv->port = atoi (port_strs[1]);
    } else {
        self->priv->port = 80;
    }

    self->priv->host = g_strdup (port_strs[0]);
    self->priv->path = g_strdup (host_strs[1]);

    g_strfreev (port_strs);
    g_strfreev (host_strs);
    g_strfreev (proto_strs);

    sockfd = socket_connect (self->priv->host, self->priv->port);

    gchar *request = g_strdup_printf ("HEAD /%s HTTP/1.1\nHost: %s\n\n",
        self->priv->path, self->priv->host);
    write (sockfd, request, strlen (request));
    g_free (request);

    gchar *response = g_new0 (gchar, 5000);

    read (sockfd, response, 4999);

//    g_print ("Response:\n%s\n-----------------------", response);

    gchar **hd = g_strsplit (response, "\n", 0);
    for (i = 0; hd[i]; i++) {
        if (g_str_has_prefix (hd[i], "Accept-Ranges:")) {
            self->priv->allow_bytes = TRUE;
        } else if (g_str_has_prefix (hd[i], "Content-Length:")) {
            self->priv->size = atoi (hd[i]+16);
        }
    }
    g_strfreev (hd);

    close (sockfd);

    g_print ("Host: %s\n", self->priv->host);
    g_print ("Path: %s\n", self->priv->path);
    g_print ("Port: %d\n", self->priv->port);
    g_print ("Size: %d\n", self->priv->size);
    g_print ("Byts: %s\n", self->priv->allow_bytes ? "True" : "False");

    return DOWNLOAD (self);
}

static int
socket_connect (char *host, int port)
{
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        g_print ("ERROR opening socket");
        return -1;
    }

    server = gethostbyname(host);
    if (server == NULL) {
        g_print ("ERROR, no such host\n");
        return -1;
    }

    bzero ((char *) &serv_addr, sizeof (serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy ((char *)server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons (port);

    if (connect(sockfd, (const struct sockaddr*) &serv_addr, sizeof (serv_addr)) < 0) {
        g_print ("ERROR connecting");
        return;
    }

    return sockfd;
}

gchar*
http_download_get_title (Download *self)
{
    return HTTP_DOWNLOAD (self)->priv->title;
}

gint
http_download_get_size_total (Download *self)
{
    return HTTP_DOWNLOAD (self)->priv->size;
}

gint
http_download_get_size_completed (Download *self)
{
    return HTTP_DOWNLOAD (self)->priv->completed;
}

gint
http_download_get_time_total (Download *self)
{
    return HTTP_DOWNLOAD (self)->priv->total_time;
}

gint
http_download_get_time_remaining (Download *self)
{
    HttpDownloadPrivate *priv = HTTP_DOWNLOAD (self)->priv;

    if (priv->rate != 0) {
        int nt = (priv->prev_time + 3 * (priv->size - priv->completed) / priv->rate) / 4;

        priv->prev_time = nt;

        return nt;
    } else {
        return 0;
    }
}

gint
http_download_get_state (Download *self)
{
    return HTTP_DOWNLOAD (self)->priv->state;
}

gboolean
on_timeout (HttpDownload *self)
{
    self->priv->total_time++;

    int diff = (self->priv->rate + 3 * (self->priv->completed - self->priv->prev_comp)) / 4;

    self->priv->rate = diff;
    self->priv->prev_comp = self->priv->completed;

    _emit_download_position_changed (DOWNLOAD (self));

    if (self->priv->state == DOWNLOAD_STATE_COMPLETED) {
        return FALSE;
    }

    return TRUE;
}

gboolean
http_download_start (Download *self)
{
    g_thread_create ((GThreadFunc) http_download_main,
        HTTP_DOWNLOAD (self), FALSE, NULL);
    g_timeout_add (1000, (GSourceFunc) on_timeout, self);
}

gboolean
http_download_stop (Download *self)
{

}

gboolean
http_download_cancel (Download *self)
{

}

gboolean
http_download_pause (Download *self)
{

}

gpointer
http_download_main (HttpDownload *self)
{
    int sockfd, n, i;
    FILE *fptr;

    self->priv->state = DOWNLOAD_STATE_RUNNING;

    sockfd = socket_connect (self->priv->host, self->priv->port);

    gchar *request = g_strdup_printf ("GET /%s HTTP/1.1\nHost: %s\n\n",
        self->priv->path, self->priv->host);
    write (sockfd, request, strlen (request));
    g_free (request);

    gchar *response = g_new0 (gchar, 5*1024);
    n = read (sockfd, response, 5*1024-1);

    fptr = fopen (self->priv->dest, "w");

    for (i = 1; i < n; i++) {
        if (response[i] == '\n' && response[i-1] == '\r' &&
            response[i-2] == '\n' && response[i-3] == '\r') {
            fwrite (response+i+1, 1, n-i-1, fptr);
            self->priv->completed += n - i - 1;
            break;
        }
    }

    while (self->priv->state == DOWNLOAD_STATE_RUNNING) {
        n = read (sockfd, response, 1024);

        if (n == 0) {
            break;
        }

        fwrite (response, 1, n, fptr);

        self->priv->completed += n;
    }

    self->priv->state = DOWNLOAD_STATE_COMPLETED;

    fclose (fptr);
    close (sockfd);
}
