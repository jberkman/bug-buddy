INCLUDES = \
	-I$(top_srcdir)/intl -I$(top_builddir)/intl	\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir) $(LIBGLADE_CFLAGS)

bin_PROGRAMS = bug-buddy

bug_buddy_SOURCES = \
	bug-buddy.c	\
	gdb-buddy.c	\
	bug-buddy.h	\
	packages.c

bug_buddy_LDADD = \
	$(LIBGLADE_LIBS)	\
	$(INTLLIBS)

noinst_PROGRAMS = gnome-crash

gnome_crash_SOURCES = gnome-crash.c

gnome_crash_LDADD = \
	$(GNOME_LIBDIR) \
	$(GNOMEUI_LIBS)

# taken from gnumeric/Makefile.am
bug-buddy.keys: bug-buddy.keys.in Makefile
	sed -e 's^\@icondir\@^$(appicondir)^g' < $(srcdir)/bug-buddy.keys.in > bug-buddy.keys.tmp \
	&& mv bug-buddy.keys.tmp bug-buddy.keys 

mimedir = $(datadir)/mime-info
mime_DATA = \
	bug-buddy.keys	\
	bug-buddy.mime

buddydir = $(datadir)/bug-buddy
buddy_DATA = \
	bug-buddy.glade	\
	gdb-cmd

utilsdir = $(datadir)/gnome/apps/Utilities
utils_DATA = bug-buddy.desktop

EXTRA_DIST = \
	glade-strings.c	\
	$(buddy_DATA)	\
	$(utils_DATA)	\
	bug-buddy.mime	\
	bug-buddy.keys.in



