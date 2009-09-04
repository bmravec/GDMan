servicedir = $(datadir)/dbus-1/services
service_in_files = org.gnome.GDMan.service.in
service_DATA = org.gnome.GDMan.service

org.gnome.GDMan.service: org.gnome.GDMan.service.in
	sed -e "s|\@bindir\@|$(bindir)|" $< > $@

INCLUDES = $(GLIB_CFLAGS) $(GTK_CFLAGS) $(DBUS_CFLAGS) \
    -DSHARE_DIR=\"$(pkgdatadir)\"

manager-glue.h: manager.xml
	$(DBUSBINDINGTOOL) --mode=glib-server --output=$@ --prefix=manager $^

bin_PROGRAMS = gdman

gdman_LDADD = $(GLIB_LIBS) $(GTK_LIBS) $(DBUS_LIBS)
gdman_SOURCES = \
    manager.c manager.h manager-glue.h

BUILT_SOURCES = manager-glue.h

EXTRA_DIST=manager.xml $(service_in_files)
CLEANFILES=manager-glue.h $(service_DATA)
