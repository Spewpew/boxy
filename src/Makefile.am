bin_PROGRAMS=boxy
CLEANFILES=boxy.h main.c
BUILT_SOURCES=boxy.h main.c
boxy.h main.c:$(srcdir)/loe.pl $(srcdir)/main.loe.pl.c
	$(srcdir)/loe.pl --mutate=$(srcdir)/main.loe.pl.c:main.c --mutate=$(srcdir)/boxy.loe.pl.h:boxy.h
boxy_SOURCES=boxy.h main.c
boxy_CFLAGS=$(SDL2_CFLAGS) $(SDL2_IMAGE_CFLAGS) 
boxy_LDADD=$(SDL2_LIBS) $(SDL2_IMAGE_LIBS) 
EXTRA_DIST=loe.pl main.loe.pl.c boxy.loe.pl.h configure.ac.shadow Makefile.am.shadow
dist-hook:
if SHADOW
	mv $(distdir)/Makefile.am.shadow $(distdir)/Makefile.am
	mv $(distdir)/configure.ac.shadow $(top_distdir)/configure.ac
	rm $(distdir)/loe.pl $(distdir)/main.loe.pl.c $(distdir)/boxy.loe.pl.h
	cd $(distdir)/..; autoreconf -i
endif
