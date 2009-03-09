## Makefile.am -- Process this file with automake to produce Makefile.in

AM_YFLAGS= -d
BUILT_SOURCES = ratedtarg.h ratedtarg.c
CLEANFILES = ratedtarg.c ratedtarg.h

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

rated_SOURCES  = ratedsnmp.c rateddbi.c rated.c ratedutil.c ratedtarg.y ratedgram.l ratedhash.c
rated_CPPFLAGS = $(SNMP_CPPFLAGS)
rated_LDFLAGS  = -rpath '$(libdir)'
rated_LDADD    = $(LEXLIB) $(SNMP_LIB)

include_HEADERS = rated.h common.h rateddbi.h

bin_PROGRAMS = rated
