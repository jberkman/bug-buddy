SUBDIRS = po pixmaps src # docs

agnomedir = $(datadir)/bug-buddy/bugzilla/gnome
agnome_DATA =

aximiandir = $(datadir)/bug-buddy/bugzilla/ximian
aximian_DATA =

bugzilladir = $(datadir)/bug-buddy
bugzilla_DATA = \
	bugzilla/gnome.bugzilla	\
	bugzilla/gnome/config	\
	bugzilla/gnome/products	\
	\
	bugzilla/ximian.bugzilla	\
	bugzilla/ximian/config		\
	bugzilla/ximian/products	\
	bugzilla/ximian/mostfreq

EXTRA_DIST = \
	bug-buddy.spec.in	\
	bug-buddy.spec		\
	intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in	\
	COPYING.ximian-logos	\
	$(bugzilla_DATA)

# this should probably be run at dist time or something
update-bugzilla:
	cd $(top_srcdir)/bugzilla/ximian/ \
	&& for file in products config mostfreq; do \
		wget "http://bugzilla.ximian.com/bugzilla-$$file.xml" -O $$file.tmp \
		&& (cmp -s $$file.tmp $$file || cp -v $$file.tmp $$file) \
		&& rm -f $$file.tmp; \
	done
	cd $(top_srcdir)/bugzilla/gnome/ \
	&& for file in products config; do \
		wget "http://bugzilla.gnome.org/bugzilla.gnome.org/bugzilla-$$file.xml" -O $$file.tmp \
		&& (cmp -s $$file.tmp $$file || cp -v $$file.tmp $$file) \
		&& rm -f $$file.tmp; \
	done
