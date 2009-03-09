## Makefile.am -- Process this file with automake to produce Makefile.in

AM_YFLAGS= -d
BUILT_SOURCES = rtgtarg.h rtgtarg.c
CLEANFILES = rtgtarg.c rtgtarg.h

EXTRA_LTLIBRARIES = librtgmysql.la librtgpgsql.la
# the ltlibraries variable is AC_SUBSTd from configure
lib_LTLIBRARIES = $(ltlibraries)

librtgmysql_la_SOURCES  = librtgmysql.c
librtgmysql_la_CPPFLAGS = $(MYSQL_CPPFLAGS)
librtgmysql_la_LDFLAGS  = -rpath '$(libdir)'
librtgmysql_la_LIBADD    = $(MYSQL_LIB)

librtgpgsql_la_SOURCES  = librtgpgsql.c
librtgpgsql_la_CPPFLAGS = $(PGSQL_CPPFLAGS)
librtgpgsql_la_LDFLAGS  = -rpath '$(libdir)'
librtgpgsql_la_LIBADD    = $(PGSQL_LIB)

rtgpoll_SOURCES  = rtgsnmp.c rtgdbi.c rtgpoll.c rtgutil.c rtgtarg.y rtggram.l rtghash.c
rtgpoll_CPPFLAGS = $(SNMP_CPPFLAGS)
rtgpoll_LDFLAGS  = -rpath '$(libdir)'
rtgpoll_LDADD    = $(LEXLIB) $(SNMP_LIB)

include_HEADERS = rtg.h common.h rtgdbi.h

bin_PROGRAMS = rtgpoll
