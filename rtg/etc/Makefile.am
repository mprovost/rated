#
# Makefile to install RTG support files.
#

top_builddir=..
etcdir=/etc

ETC          = rtg.conf routers createdb

EXTRA_DIST   = createdb.in rtg.conf routers rtg.png rtgback.png

all:

install: installdirs
	@for i in $(ETC) ; do $(INSTALL) $(srcdir)/$$i $(prefix)$(etcdir) ; echo "install: installed $$i in $(prefix)$(etcdir)" ; done

installdirs:
	$(mkinstalldirs) $(prefix)$(etcdir)
