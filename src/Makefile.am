INCLUDES = $(PROG_CFLAGS) $(GTK_CFLAGS) \
    -DSHARE_DIR=\"$(pkgdatadir)\"

bin_PROGRAMS = gdman

gdman_LDADD = $(PROG_LIBS) $(GTK_LIBS)
gdman_SOURCES = \
    shell.c shell.h

EXTRA_DIST=
CLEANFILES=
