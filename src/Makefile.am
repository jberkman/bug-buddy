buddydir = $(datadir)/bug-buddy

INCLUDES = \
	-I$(top_srcdir)/intl -I$(top_builddir)/intl	\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	$(BUG_BUDDY_CFLAGS) -I$(includedir)		\
	-DBUDDY_DATADIR=\""$(buddydir)"\"		\
        -DBUDDY_ICONDIR=\""$(datadir)/pixmaps"\"	\
	-D_GNU_SOURCE


bin_PROGRAMS = bug-buddy

bug_buddy_SOURCES = \
	bug-buddy.c		\
	bug-buddy.h		\
	bugzilla.c		\
	bugzilla.h		\
	config.c		\
	gdb-buddy.c		\
	libglade-buddy.h	\
	united-states-of-bug-buddy.c

bug_buddy_LDFLAGS = \
	-export-dynamic

bug_buddy_LDADD = \
	$(BUG_BUDDY_LIBS)

noinst_PROGRAMS = gnome-crash

gnome_crash_SOURCES = gnome-crash.c

gnome_crash_LDADD = \
	$(GNOME_CRASH_LIBS)

# taken from gnumeric/Makefile.am
bug-buddy.keys: bug-buddy.keys.in Makefile
	sed -e 's^\@icondir\@^$(datadir)/pixmaps^g' < $(srcdir)/bug-buddy.keys.in > bug-buddy.keys.tmp \
	&& mv bug-buddy.keys.tmp bug-buddy.keys 

mimedir = $(datadir)/mime-info
mime_DATA = \
	bug-buddy.keys	\
	bug-buddy.mime

buddy_DATA = \
	bug-buddy.glade2	\
	gdb-cmd

bug-buddy.glade2: bug-buddy.glade $(LIBGLADE_CONVERT)
	$(LIBGLADE_CONVERT) $(srcdir)/bug-buddy.glade > bug-buddy.glade.tmp \
	&& mv bug-buddy.glade.tmp bug-buddy.glade2

utilsdir = $(datadir)/gnome/apps/Utilities
utils_in_files = bug-buddy.desktop.in
utils_DATA = $(utils_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	$(utils_DATA)	\
	gdb-cmd		\
	bug-buddy.glade	\
	bug-buddy.mime	\
	bug-buddy.keys.in
