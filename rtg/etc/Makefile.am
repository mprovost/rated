#
# Makefile to install RTG support files.
#

top_builddir=..
etcdir=/etc
webdir=/web
reportsdir=/bin

ETC          = rtg.conf rtgtargmkr.pl routers createdb BER.pm \
               SNMP_Session.pm SNMP_util.pm
WEB          = rtg.php 95.php view.php common.php rtg.png rtgback.png
REPORTS      = report.pl 95.pl

EXTRA_DIST   = rtgtargmkr.pl.in report.pl 95.pl createdb.in \
               BER.pm SNMP_Session.pm SNMP_util.pm rtg.conf \
	       routers rtg.php 95.php view.php common.php.in rtg.png rtgback.png

all:

install: installdirs
	@for i in $(ETC) ; do $(INSTALL) $(srcdir)/$$i $(prefix)$(etcdir) ; echo "install: installed $$i in $(prefix)$(etcdir)" ; done
	@for i in $(WEB) ; do $(INSTALL) $(srcdir)/$$i $(prefix)$(webdir) ; echo "install: installed $$i in $(prefix)$(webdir)" ; done
	@for i in $(REPORTS) ; do $(INSTALL) $(srcdir)/$$i $(prefix)$(reportsdir) ; echo "install: installed $$i in $(prefix)$(reportsdir)" ; done

installdirs:
	$(mkinstalldirs) $(prefix)$(etcdir)
	$(mkinstalldirs) $(prefix)$(webdir)
	$(mkinstalldirs) $(prefix)$(reportsdir)
