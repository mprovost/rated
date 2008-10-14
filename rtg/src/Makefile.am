## Makefile.am -- Process this file with automake to produce Makefile.in

AM_YFLAGS= -d
BUILT_SOURCES = rtgtarg.h rtgtarg.c
CLEANFILES = rtgtarg.c rtgtarg.h

EXTRA_LTLIBRARIES = librtgmysql.la librtgpgsql.la
# the ltlibraries variable is AC_SUBSTd from configure
lib_LTLIBRARIES = $(ltlibraries)

librtgmysql_la_SOURCES = librtgmysql.c
librtgmysql_la_LDFLAGS = -rpath '$(libdir)'

librtgpgsql_la_SOURCES = librtgpgsql.c
librtgpgsql_la_LDFLAGS = -rpath '$(libdir)'

#librtgpgsql_la_SOURCES = librtgpgsql.c
#librtgpgsql_la_LDFLAGS = -rpath '$(libdir)'

rtgpoll_SOURCES = rtgsnmp.c rtgdbi.c rtgpoll.c rtgutil.c \
                  rtgtarg.y rtggram.l rtghash.c
rtgpoll_LDFLAGS = -rpath '$(libdir)'

include_HEADERS = rtg.h rtgplot.h common.h rtgdbi.h

rtgpoll_LDADD = $(LEXLIB)

bin_PROGRAMS = rtgpoll
