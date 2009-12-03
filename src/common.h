#ifndef RTG_COMMON_H
#define RTG_COMMON_H 1

#if HAVE_CONFIG_H
#  include "config.h"
#endif

/* Eliminate redefines due to 3rd party packages */
#ifdef PACKAGE_BUGREPORT
# define RTG_PACKAGE_BUGREPORT PACKAGE_BUGREPORT
# define RTG_PACKAGE_NAME PACKAGE_NAME
# define RTG_PACKAGE_STRING PACKAGE_STRING
# define RTG_PACKAGE_TARNAME PACKAGE_TARNAME
# define RTG_PACKAGE_VERSION PACKAGE_VERSION
# undef PACKAGE_BUGREPORT
# undef PACKAGE_NAME
# undef PACKAGE_STRING
# undef PACKAGE_TARNAME
# undef PACKAGE_VERSION
#endif

#define _GNU_SOURCE
#include <stdio.h>

#if STDC_HEADERS
#  include <stdlib.h>
#  include <errno.h>
#  include <limits.h>
#  include <string.h>
#  include <stdarg.h>
#  include <assert.h>
#elif HAVE_STRINGS_H
#  include <strings.h>
#endif /*STDC_HEADERS*/

#if HAVE_UNISTD_H
#  include <sys/types.h>
#  include <unistd.h>
#endif

#if HAVE_STDINT_H
#  include <stdint.h>
#endif

#if HAVE_MATH_H
#  include <math.h>
#endif

#if HAVE_NETINET_IN_H
#  include <netinet/in.h>
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifndef HAVE_LIBPTHREAD
# define HAVE_LIBPTHREAD 0
#else
# include <pthread.h>
#endif

#include <signal.h>

#if HAVE_SYSLOG_H
# include <syslog.h>
#endif

#endif /* RTG_COMMON_H */
