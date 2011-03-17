## Makefile.am -- Process this file with automake to produce Makefile.in

AM_YFLAGS= -d
BUILT_SOURCES = ratedtarg.h ratedtarg.c
CLEANFILES = ratedtarg.c ratedtarg.h

EXTRA_LTLIBRARIES = libratedmysql.la libratedpgsql.la
# the ltlibraries variable is AC_SUBSTd from configure
lib_LTLIBRARIES = $(ltlibraries)

libratedmysql_la_SOURCES  = libratedmysql.c
libratedmysql_la_CPPFLAGS = $(MYSQL_CPPFLAGS)
libratedmysql_la_LDFLAGS  = -rpath '$(libdir)'
libratedmysql_la_LIBADD    = $(MYSQL_LIB)

libratedpgsql_la_SOURCES  = libratedpgsql.c
libratedpgsql_la_LDFLAGS  = -rpath '$(libdir)'

rated_SOURCES  = ratedsnmp.c rateddbi.c rated.c ratedutil.c ratedtarg.y ratedgram.l ratedhash.c
#rated_CPPFLAGS = $(SNMP_CPPFLAGS)
rated_LDFLAGS  = -rpath '$(libdir) $(SNMP_LIB)'
#rated_LDADD    = $(LEXLIB) $(SNMP_LIB)

include_HEADERS = rated.h common.h rateddbi.h

bin_PROGRAMS = rated
