/* bug-buddy bug submitting program
 *
 * Copyright (C) Jacob Berkman
 *
 * Author:  Jacob Berkman  <jberkman@andrew.cmu.edu>
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


static gint debian_bts_init (xmlNodePtr node);
static void debian_bts_denit (void);
static gint debian_bts_doit (void);
static const char *debian_bts_get_email (void);

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
	char *s, *p, *line, *appmap, *file, *row[2] = { NULL };
	char last_letter='\0';
	int fd, count=0;
	GtkCTreeNode *last_root = NULL;
	w = GET_WIDGET ("package_entry2");
	cur = node->childs;
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

			combo = GET_WIDGET ("miggie_combo");

			w = APP_FILE;
			s = gtk_entry_get_text (GTK_ENTRY (w));
			p = g_strdup (popt_data.package);
			appmap = xmlGetProp (cur, "appmap");

			if (!p && strlen (s) && appmap)
				p = get_package_from_appname (appmap, s);

			xmlFree (appmap);
			if (!p)
				p = g_strdup ("general");

			w = CTREE_COMBO (combo)->entry;
			gtk_entry_set_text (GTK_ENTRY (w), p);
			g_free (p);

			w = CTREE_COMBO (combo)->ctree;
			gtk_clist_clear (GTK_CLIST (w));
			gtk_clist_freeze (GTK_CLIST (w));
			while ((row[0] = get_line_from_fd (fd))) {
				if (row[0][0] != last_letter ||
				    count > 20) {
					last_root = MAKE_ROOT(w, row);
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

	w = GET_WIDGET ("packages_href");
	line = g_strdup_printf ("%s/db/ix/packages.html",
				debian_data.web);
	gnome_href_set_url (GNOME_HREF (w), line);
	g_free (line);

	w = GET_WIDGET ("desc_href");
	line = g_strdup_printf ("%s/Developer.html#severities",
				debian_data.web);
	gnome_href_set_url (GNOME_HREF (w), line);
	g_free (line);

	w = GET_WIDGET ("existing_href");
	line = g_strdup_printf ("%s/db/ix/psummary.html",
				debian_data.web);
	gnome_href_set_url (GNOME_HREF (w), line);
	g_free (line);

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

static gboolean
debian_bts_doit ()
{
	GtkWidget *w;
	gchar *s, *s2, *s3, *subject;
	char *command;
	FILE *fp = stdout;
	int bugnum, i;

	if (druid_data.bug_type == BUG_NEW) {
		s2 = g_strconcat ("submit", debian_data.email, NULL);
	} else {	
		w = GET_WIDGET ("bug_number");
		bugnum = gtk_spin_button_get_value_as_int (
			GTK_SPIN_BUTTON (w));
		s2 = g_strdup_printf ("%d%s", bugnum, debian_data.email);
	}
		
	w = GET_WIDGET ("email_entry");
	s = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);

	switch (druid_data.submit_type) {
	case SUBMIT_TO_SELF:
		g_free (s2);
		s2 = g_strdup (s);
		/* fall through */
	case SUBMIT_REPORT:
		w = GET_WIDGET ("mailer_entry");
		s3 = gtk_entry_get_text (GTK_ENTRY (w));
		command = g_strconcat (s3, " -i -t", NULL);
		g_message (_("about to run '%s'"), command);
		fp = popen (command, "w");
		if (!fp) {
			g_free (s);
			g_free (s2);
			g_free (command);
			s = g_strdup_printf (_("Unable to start mail program:\n"
					       "'%s'"), s3);
			w = gnome_error_dialog (s);
			g_free (s);
			gnome_dialog_run_and_close (GNOME_DIALOG (w));
			return TRUE;
		}
		g_free (command);
		break;
	case SUBMIT_FILE:
		w = GET_WIDGET ("file_entry");
		s3 = gtk_entry_get_text (GTK_ENTRY (w));
		fp = fopen (s3, "w");
		if (!fp) {
			g_free (s);
			g_free (s2);
			s = g_strdup_printf (_("Unable to open file:\n"
						      "'%s'"), s3);
			w = gnome_error_dialog (s);
			g_free (s);
			gnome_dialog_run_and_close (GNOME_DIALOG (w));
			return TRUE;
		}
		break;
#ifdef SUBMIT_NONE
	case SUBMIT_NONE:
		g_free (s);
		g_free (s2);
		return FALSE;
#endif
	default:
		g_assert_not_reached ();
		return FALSE;
	}

	fprintf (fp, "To: %s\n", s2);
	g_free (s2);

	w = GET_WIDGET ("name_entry");
	s2 = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);
	fprintf (fp, "From: %s <%s>\n", s2, s);
	g_free (s);
	g_free (s2);

	w = GET_WIDGET ("desc_entry");
	subject = s = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);
	fprintf (fp, "Subject: %s\nX-Mailer: %s %s\n", s, PACKAGE, VERSION);

	w = GET_WIDGET ("miggie_combo");
	w = CTREE_COMBO (w)->entry;
	s = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);
	if (!strlen(s)) {
		g_free (s);
		s = g_strdup ("general");
	}
	fprintf (fp, 
		 "\nPackage: %s\n"
		 "Severity: %s\n", s, severity[druid_data.severity]);
	g_free (s);
	
	w = glade_xml_get_widget (druid_data.xml, "version_entry");
	s = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);
	fprintf (fp, 
		 "Version: %s\n\n"
		 ">Synopsis: %s\n"
		 ">Class: %s\n", 
		 s, subject, 
		 bug_class[druid_data.bug_class][1]);
	g_free (s);
	g_free (subject);

	w = VERSION_LIST;
	for (i = 0; i < GTK_CLIST (w)->rows; i++) {
		s = s2 = NULL;
		gtk_clist_get_text (GTK_CLIST (w), i, 0, &s);
		gtk_clist_get_text (GTK_CLIST (w), i, 1, &s2);
		
		if (s && strlen (s) &&
		    s2 && strlen (s2))
			fprintf (fp, "%s: %s\n", s, s2);
	}

	w = glade_xml_get_widget (druid_data.xml, "desc_area");
	s = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);
	if (s && strlen (s)) {
		fprintf (fp, "\n\n>Description:\n");
		write_line_widthv (fp, s);
	}
	g_free (s);

	w = glade_xml_get_widget (druid_data.xml, "repeat_area");
	s = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);
	if (s && strlen(s)) {
		fprintf (fp, "\n\n>How-To-Repeat:\n");
		write_line_widthv (fp, s);
	}
	g_free (s);

	w = glade_xml_get_widget (druid_data.xml, "gdb_text");
	s = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);
	if (s && strlen(s))
		fprintf (fp, "\n\nDebugging information:\n%s\n", s);
	g_free (s);

	w = GET_WIDGET ("include_entry");
	s = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);
	if (s && g_file_exists (s)) {
		char line[1024];
		FILE *fp2 = fopen (s, "r");
		if (fp2) {
			fprintf (fp, "\n\n--- Included file '%s' ---\n\n", s);
			while (fgets (line, 1023, fp2)) {
				fprintf (fp, "%s", line);
			}
			fprintf (fp, "\n\n--- End of file ---\n\n");
			fclose (fp2);
		}
	}
	g_free (s);

	if (druid_data.submit_type == SUBMIT_FILE)
		fclose (fp);
	else
		pclose (fp);
#if 0
	g_message (_("Subprocess exited with status %d"), status);
#endif
	return FALSE;
}

