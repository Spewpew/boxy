bin_PROGRAMS=boxy
CLEANFILES=main.c
BUILT_SOURCES=main.c
main.c:$(srcdir)/loe.pl $(srcdir)/main.loe.pl.c
	$(srcdir)/loe.pl --mutate=$(srcdir)/main.loe.pl.c:main.c
boxy_SOURCES=main.c
boxy_CFLAGS=$(SDL2_CFLAGS) $(SDL2_IMAGE_CFLAGS) 
boxy_LDADD=$(SDL2_LIBS) $(SDL2_IMAGE_LIBS) 
EXTRA_DIST=loe.pl main.loe.pl.c configure.ac.shadow Makefile.am.shadow
dist-hook:
if SHADOW
	mv $(distdir)/Makefile.am.shadow $(distdir)/Makefile.am
	mv $(distdir)/configure.ac.shadow $(top_distdir)/configure.ac
	rm $(distdir)/loe.pl $(distdir)/main.loe.pl.c
	cd $(distdir)/..; autoreconf -i
endif
