buddydir = $(datadir)/bug-buddy

INCLUDES = \
	-I$(top_srcdir)/intl -I$(top_builddir)/intl	\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir) $(LIBGLADE_CFLAGS)		\
	-DBUDDY_DATADIR=\""$(buddydir)"\"		\
        -DBUDDY_ICONDIR=\""$(datadir)/pixmaps"\"


bin_PROGRAMS = bug-buddy

bug_buddy_SOURCES = \
	bts.c		\
	bts.h		\
	bts-debian.c	\
	bug-buddy.c	\
	bug-buddy.h	\
	distro.h	\
	distro-debian.c	\
	distro-redhat.c	\
	gdb-buddy.c	\
	glade-druid.h	\
	util.c		\
	util.h

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
	sed -e 's^\@iconsdir\@^$(datadir)/pixmaps^g' < $(srcdir)/bug-buddy.keys.in > bug-buddy.keys.tmp \
	&& mv bug-buddy.keys.tmp bug-buddy.keys 

mimedir = $(datadir)/mime-info
mime_DATA = \
	bug-buddy.keys	\
	bug-buddy.mime

debian-packages: Makefile.am
	grep -h ^Package $(DEBIAN_ROOT)/var/state/apt/lists/*_Packages | \
	sort -r | awk '{ print $$2 }' > debian-packages

gnome-packages: Makefile.am
	sort -r $(top_srcdir)/debbugs/Maintainers | \
	awk '{ print $$1 }' > gnome-packages

buddy_DATA = \
	bug-buddy.glade	\
	prog.bugmap	\
	gdb-cmd		\
	gnome-packages	\
	debian-packages

utilsdir = $(datadir)/gnome/apps/Utilities
utils_DATA = bug-buddy.desktop

xmldir = $(datadir)/bug-buddy/xml
xml_DATA = \
	GNOME.bts	\
	Debian.bts

EXTRA_DIST = \
	glade-strings.c	\
	$(buddy_DATA)	\
	$(utils_DATA)	\
	$(xml_DATA)	\
	bug-buddy.mime	\
	bug-buddy.keys.in