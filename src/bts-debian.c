/* bug-buddy bug submitting program
 *
 * Copyright (C) 1999, 2000 Jacob Berkman
 * Copyright 2000 Helix Code, Inc.
 *
 * Author:  Jacob Berkman  <jacob@helixcode.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gnome-xml/tree.h>
#include <gnome-xml/parser.h>
#include <gnome-xml/xmlmemory.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include <gnome.h>

#include "glade-druid.h"
#include "bug-buddy.h"
#include "util.h"
#include "bts.h"

#include "ctree-combo.h"

#define MAKE_LEAF(w, r, rw) gtk_ctree_insert_node (GTK_CTREE (w), r, NULL, rw,\
                                                   0, NULL, NULL, NULL, NULL,\
                                                   TRUE, FALSE)

#define MAKE_ROOT(w, rw) gtk_ctree_insert_node (GTK_CTREE (w), NULL, NULL, rw,\
                                                0, NULL, NULL, NULL, NULL,\
                                                FALSE, FALSE)

#define GET_TEXT(w)    (gtk_editable_get_chars (GTK_EDITABLE ((w)), 0, -1))
#define APPEND_TEXT(s) (gtk_editable_insert_text (edit, (s), strlen ((s)), &pos))

static gint debian_bts_init (xmlNodePtr node);
static void debian_bts_denit (void);
static void debian_bts_doit (GtkEditable *edit);
static const char *debian_bts_get_email (void);
void on_already_href_clicked (GtkWidget *w, gpointer data);

GtkWidget *make_miggie_combo (gchar *widget_name, gchar *s1,
			      gchar *s2, gint i1, gint i2);

BugTrackingSystem debian_bts = {
	debian_bts_init,
	debian_bts_denit,
	debian_bts_doit,
	debian_bts_get_email
};

static struct {
	char *email;
	char *web;
	GList *packages;
} debian_data;

static const char *
debian_bts_get_email ()
{
	return debian_data.email;
}

GtkWidget *
make_miggie_combo (gchar *widget_name, gchar *s1,
		   gchar *s2, gint i1, gint i2)
{
	return ctree_combo_new (i1, i2);
}

void
on_already_href_clicked (GtkWidget *w, gpointer data)
{
	char *package, *url;

	package = GET_TEXT (CTREE_COMBO (GET_WIDGET ("miggie_combo"))->entry);
	url = g_strdup_printf ("%s/db/pa/l%s.html", debian_data.web, package);
	gnome_url_show (url);
	g_free (url);
	g_free (package);
}

/* ugly stupid dumb stuff stolen from the bug reporting web page */
static gchar *
get_package_from_appname (const char *appmap, const char *appname)
{
	FILE *fp;
	gchar *package, *file;
	char buf[1024];
	GHashTable *table;

	if (appmap[0] == '/')
		file = g_strdup (appmap);
	else
		file = g_strconcat (BUDDY_DATADIR "/", appmap, NULL);
	fp = fopen (file, "r");
	if (!fp) {
		g_warning ("Could not open bugfile '%s'", file);
		g_free (file);
		return NULL;
	}
	g_free (file);
	table = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_freeze (table);

	while (fgets (buf, 1024, fp)) {
		package = strchr (buf, ' ');
		if (!package)
			continue;
		*package = '\0';

		g_hash_table_insert (table, 
				     g_strdup (buf), 
				     g_strdup (g_strstrip (package+1)));
	}

	fclose (fp);

	g_hash_table_thaw (table);
	package = g_strdup (g_hash_table_lookup (table,
						 g_filename_pointer (appname)));
	
	destroy_hash_table (table, TRUE);

	return package;
}

static gint
debian_bts_init (xmlNodePtr node)
{
	xmlNodePtr cur;
	GtkWidget *w, *combo;
	char *s, *p, *line, *appmap, *file, *row[2] = { NULL }, *pa;
	char last_letter='\0';
	int fd, count=0;
	GtkCTreeNode *last_root = NULL;
	w = GET_WIDGET ("package_entry2");
	cur = node->childs;


	combo = GET_WIDGET ("miggie_combo");
	/*gtk_entry_set_text (GTK_ENTRY (CTREE_COMBO (combo)->entry), "general");*/
	gtk_clist_clear    (GTK_CLIST (CTREE_COMBO (combo)->ctree));

	while (cur) {
		switch (cur->name[0]) {
		case 'e':
			if (strcmp (cur->name, "email")) break;
			debian_data.email = xmlNodeGetContent (cur);
			break;
		case 'w':
			if (strcmp (cur->name, "web")) break;
			debian_data.web = xmlNodeGetContent (cur);
			break;
		case 'p':
			if (strcmp (cur->name, "packages-list")) break;
			file = xmlNodeGetContent (cur);
			line = g_strconcat (BUDDY_DATADIR "/", file, NULL);
			xmlFree (file);
			fd = open (line, O_RDONLY);
			g_free (line);
			if (fd == -1) break;

			w = APP_FILE;
			s = gtk_entry_get_text (GTK_ENTRY (w));
			p = g_strdup (popt_data.package);
			appmap = xmlGetProp (cur, "appmap");

			pa = gtk_entry_get_text (GTK_ENTRY (CTREE_COMBO (GET_WIDGET ("miggie_combo"))->entry));

			/*
			  if (the string is general or there is no string)
			  	look it up;
				if (it wasn't found)
					use general;
			*/

			if (!pa[0] || !strcmp (pa, "general")) {
				if (!p && appmap && s)
					p = get_package_from_appname (appmap, s);
				gtk_entry_set_text (GTK_ENTRY (CTREE_COMBO (combo)->entry), p ? p : "general");
			}
			g_free (p);
			
			xmlFree (appmap);

			w = GTK_CLIST (CTREE_COMBO (combo)->ctree);
			gtk_clist_freeze (GTK_CLIST (w));
			while ((row[0] = get_line_from_fd (fd))) {
				if (row[0][0] != last_letter ||
				    count > 20) {
					char *s = g_strdup_printf ("%.8s...", row[0]);
					char *t = row[0];
					row[0] = s;
					last_root = MAKE_ROOT(w, row);
					g_free (row[0]);
					row[0] = t;
					last_letter = row[0][0];
					count = 0;
				}
				MAKE_LEAF (w, last_root, row);
				++count;
				g_free (row[0]);
			}
			gtk_clist_thaw (GTK_CLIST (w));
			break;
		default:
			g_warning ("unknown node: %s", cur->name);
		}
		cur = cur->next;
	}

	{
		char *hrefs[] = { 
			"packages_href", "packages_label", "%s/db/ix/packages.html",
			"desc_href",     "desc_label",     "%s/Developer.html#severities",
			"existing_href", "existing_label", "%s/db/ix/psummary.html",
			NULL
		};
		int i;
		GtkWidget *w2;
		for (i=0; hrefs[i]; i+=3) {
			w  = GET_WIDGET (hrefs[i]);
			w2 = GET_WIDGET (hrefs[i+1]);
			if (debian_data.web) {
				gtk_widget_hide (w2);
				gtk_widget_show (w);
				line = g_strdup_printf (hrefs[i+2],
							debian_data.web);
				gnome_href_set_url (GNOME_HREF (w), line);
				g_free (line);
			} else {
				gtk_widget_hide (w);
				gtk_widget_show (w2);
			}
		}
		w2 = GET_WIDGET ("already_href");
		if (debian_data.web)
			gtk_widget_show (w2);
		else
			gtk_widget_hide (w2);
	}

	return 0;
}

static void 
debian_bts_denit ()
{
	xmlFree (debian_data.email);
	debian_data.email = NULL;

	xmlFree (debian_data.web);
	debian_data.web = NULL;

	g_list_foreach (debian_data.packages, (GFunc)g_free, NULL);
	g_list_free (debian_data.packages);
	debian_data.packages = NULL;
}

static void
debian_bts_doit (GtkEditable *edit)
{
	GtkWidget *w;
	char *s, *s2, *s3;
	char *from_name, *from_email, *subject, *package, *version;
	int i, pos = 0;

	w = GET_WIDGET ("miggie_combo");

	from_name  = GET_TEXT (GET_WIDGET ("name_entry"));
	from_email = GET_TEXT (GET_WIDGET ("email_entry"));
	subject    = GET_TEXT (GET_WIDGET ("desc_entry"));
	package    = GET_TEXT (CTREE_COMBO (w)->entry);
	version    = GET_TEXT (GET_WIDGET ("version_entry"));

	s = g_strdup_printf ("From: %s <%s>\n"
			     "Subject:  %s\nX-Mailer: "PACKAGE" "VERSION"\n\n"
			     "Package:  %s\n"
			     "Severity: %s\n"
			     "Version:  %s\n"
			     "Synopsis: %s\n"
			     "Class:    %s\n\n",
			     from_name, from_email,
			     subject, package,
			     severity[druid_data.severity],
			     version, subject,
			     bug_class[druid_data.bug_class][1]);
	g_free (from_name);
	g_free (from_email);
	g_free (subject);
	g_free (package);
	g_free (version);

	APPEND_TEXT (s);

	w = VERSION_LIST;
	for (i = 0; i < GTK_CLIST (w)->rows; i++) {
		s = s2 = NULL;
		gtk_clist_get_text (GTK_CLIST (w), i, 0, &s);
		gtk_clist_get_text (GTK_CLIST (w), i, 1, &s2);
		
		if (s && s2 && strlen (s) && strlen (s2)) {
			s3 = g_strdup_printf ("%s: %s\n", s, s2);
			APPEND_TEXT (s3);
			g_free (s3);
		}
	}

	s = GET_TEXT (GET_WIDGET ("desc_area"));
	if (s && strlen (s)) {
		APPEND_TEXT ("\n\nDescription:\n");
		append_widthv (edit, s, &pos);
	}
	g_free (s);

	s = GET_TEXT (GET_WIDGET ("gdb_text"));
	if (s && strlen (s)) {
		APPEND_TEXT ("\n\nDebugging information:\n");
		APPEND_TEXT (s);
	}
	g_free (s);

	s = GET_TEXT (GET_WIDGET ("include_entry"));
	if (s && g_file_exists (s)) {
		char line[1024];
		FILE *fp = fopen (s, "r");
		if (!fp) goto no_file;
		APPEND_TEXT ("\n\n--- Included file ---\n\n");
		while (fgets (line, 1023, fp))
			APPEND_TEXT (line);
		APPEND_TEXT ("\n\n--- End of file ---\n\n");
		fclose (fp);
	}
 no_file:
	g_free (s);
}
