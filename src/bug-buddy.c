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

#include <gnome.h>
#include <glade/glade.h>

#include <signal.h>
#include <ctype.h>

#include "bug-buddy.h"
#include "util.h"
#include "distro-redhat.h"
#include "distro-debian.h"

/* libglade callbacks */
gboolean on_nature_page_next  (GtkWidget *, GtkWidget *);
gboolean on_less_page_prepare (GtkWidget *, GtkWidget *);
gboolean on_less_page_next    (GtkWidget *, GtkWidget *);
gboolean on_misc_page_back    (GtkWidget *, GtkWidget *);
gboolean on_the_druid_cancel  (GtkWidget *);
gboolean on_complete_page_prepare (GtkWidget *, GtkWidget *);
gboolean on_complete_page_finish  (GtkWidget *, GtkWidget *);
gboolean on_system_page_prepare   (GtkWidget *, GtkWidget *);
gboolean on_gnome_page_prepare    (GtkWidget *, GtkWidget *);
gboolean on_contact_page_next     (GtkWidget *, GtkWidget *);
void on_version_list_select_row   (GtkCList *list, gint row, gint col,
				   GdkEventButton *event, gpointer udata);
void update_selected_row      (GtkWidget *widget, gpointer data);
void on_version_apply_clicked (GtkButton *button, gpointer data);
void on_file_radio_toggled    (GtkWidget *radio, gpointer data);
gboolean on_action_page_next  (GtkWidget *page, GtkWidget *druid);
gboolean on_action_page_back  (GtkWidget *page, GtkWidget *druid);
GtkWidget *make_anim (gchar *widget_name, gchar *string1, 
		      gchar *string2, gint int1, gint int2);
GtkWidget *make_pixmap_button (gchar *widget_name, gchar *string1, 
			       gchar *string2, gint int1, gint int2);
void on_newreport_radio_toggled (GtkWidget *w, gpointer data);
void on_existing_radio_toggled (GtkWidget *w, gpointer data);



#define LINE_WIDTH 72

extern const char *packages[];

const gchar *severity[] = { 
	N_("critical"),
	N_("grave"),
	N_("normal"),
	N_("wishlist"),
	NULL };

const gchar *bug_class[][2] = {
	{ N_("software bug"),      "sw-bug" },
	{ N_("documentation bug"), "doc-bug" },
	{ N_("change request"),    "change-request" },
	{ N_("support"),           "support" },
	{ NULL, NULL } };

struct {
	/* contact page */
	gchar *name;
	gchar *email;
	
	/* package page */
	gchar *package;
	gchar *package_ver;
	
	/* dialog page */
	gchar *app_file;
	gchar *pid;
	
	/* core page */
	gchar *core_file;
} popt_data;

DruidData druid_data;

static Distribution distros[] = {
	{ "Slackware", "/etc/slackware-version", &debian_phy },
	{ "Debian",    "/etc/debian_version",    &debian_phy },
	{ "Red Hat",   "/etc/redhat-release",    &redhat_phy },
	{ "SuSE",      "/etc/SuSE-release",      &redhat_phy },
	{ "Mandrake",  "/etc/mandrake-release",  &redhat_phy },
	{ NULL }
};

static Package package[] = {
	{ N_("System"),    "uname -a" },
	{ N_("C library"),  NULL,                    "glibc", "libc6", },
	{ N_("C compiler"), "gcc --version",          NULL,    NULL, "cc -V", },
	{ N_("glib"),       "glib-config --version",  "glib",  "libglib1.2" },
	{ N_("GTK+"),       "gtk-config --version",   "gtk+",  "libgtk1.2" },
	{ N_("ORBit"),      "orbit-config --version", "ORBit", "liborbit0" },
	{ N_("gnome-libs"), "gnome-config --version", "gnome-libs", "gnome-libs-data" },
	{ N_("gnome-core"),  "gnome-config --modversion applets "
	                     "| grep -v gnome-libs "
	                     "| sed -e 's^applets-^gnome-core ^g'",
	  "gnome-core", "gnome-core" },
	{ NULL }
};

static const struct poptOption options[] = {
	{ "name",        0, POPT_ARG_STRING, &popt_data.name,        0, N_("Contact's name"),          N_("NAME") },
	{ "email",       0, POPT_ARG_STRING, &popt_data.email,       0, N_("Contact's email address"), N_("EMAIL") },
	{ "package",     0, POPT_ARG_STRING, &popt_data.package,     0, N_("Package of the program"),  N_("PACKAGE") },
	{ "package-ver", 0, POPT_ARG_STRING, &popt_data.package_ver, 0, N_("Version of the package"),  N_("VERSION") },
	{ "appname",     0, POPT_ARG_STRING, &popt_data.app_file,    0, N_("Crashed file name"),       N_("FILE") },
	{ "pid",         0, POPT_ARG_STRING, &popt_data.pid,         0, N_("PID of crashed app"),      N_("PID") },
	{ "core",        0, POPT_ARG_STRING, &popt_data.core_file,   0, N_("core file from app"),      N_("FILE") },
	{ NULL } 
};

static void
save_entry (const char *name, const char *name2, const char *save_path)
{
	GtkWidget *w;
	gchar *s;

	w = glade_xml_get_widget (druid_data.xml, name);
	s = gtk_entry_get_text (GTK_ENTRY (w));
	gnome_config_set_string (save_path, s);
	w = glade_xml_get_widget (druid_data.xml, name2);
	if (GNOME_IS_FILE_ENTRY (w))
		w = gnome_file_entry_gnome_entry (GNOME_FILE_ENTRY (w));
	gnome_entry_prepend_history (GNOME_ENTRY (w), TRUE, s);
	gnome_entry_save_history (GNOME_ENTRY (w));
}

static void
save_config ()
{

	save_entry ("name_entry",   "name_entry2",   "/bug-buddy/last/name");
	save_entry ("email_entry",  "email_entry2",  "/bug-buddy/last/email");
	save_entry ("file_entry",   "file_entry2",   "/bug-buddy/last/bugfile");
	save_entry ("mailer_entry", "mailer_entry2", "/bug-buddy/last/mailer");
	
	gnome_config_set_int ("/bug-buddy/last/submittype", 
			      druid_data.submit_type);

	gnome_config_sync ();
}

static void
load_entry (const char *name, const char *name2,
	    const char *path, const char *def)
{
	GtkWidget *w;
	gchar *s;

	w = glade_xml_get_widget (druid_data.xml, name);
	s = gnome_config_get_string (path);
	if (s)
		gtk_entry_set_text (GTK_ENTRY (w), s);
	else if (def)
		gtk_entry_set_text (GTK_ENTRY (w), def);
	g_free (s);
	w = glade_xml_get_widget (druid_data.xml, name2);
	if (GNOME_IS_FILE_ENTRY (w))
		w = gnome_file_entry_gnome_entry (GNOME_FILE_ENTRY (w));
	gnome_entry_load_history (GNOME_ENTRY (w));
}

static void
load_config ()
{
	char *sendmail;

	load_entry ("name_entry", "name_entry2",
		    "/bug-buddy/last/name", g_get_real_name ());

	load_entry ("email_entry", "email_entry2",
		    "/bug-buddy/last/email", g_get_user_name ());

	load_entry ("file_entry", "file_entry2",
		    "/bug-buddy/last/bugfile", NULL);

	sendmail = gnome_is_program_in_path ("sendmail");
	if (!sendmail) {
		if (g_file_exists ("/usr/sbin/sendmail"))
			sendmail = g_strdup ("/usr/sbin/sendmail");
		else if (g_file_exists ("/usr/lib/sendmail"))
			sendmail = g_strdup ("/usr/lib/sendmail");
	}
	load_entry ("mailer_entry", "mailer_entry2",
		    "/bug-buddy/last/mailer", sendmail);
	g_free (sendmail);

	druid_data.submit_type = 
		gnome_config_get_int ("/bug-buddy/last/submittype");
}

static gboolean
update_crash_type (GtkWidget *w, gpointer data)
{
	CrashType new_type = GPOINTER_TO_INT (data);
	GtkWidget *segv, *core;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
		druid_data.crash_type = new_type;

	segv = glade_xml_get_widget (druid_data.xml, "segv_table");
	core = glade_xml_get_widget (druid_data.xml, "core_box");

	switch (new_type) {
	case CRASH_DIALOG:
		gtk_widget_show (segv);
		gtk_widget_hide (core);
		break;
	case CRASH_CORE:
		gtk_widget_show (core);
		gtk_widget_hide (segv);
		break;
	case CRASH_NONE:
		gtk_widget_hide (core);
		gtk_widget_hide (segv);
		break;
	}
	
	return FALSE;
}

static gboolean
update_submit_type (GtkWidget *w, gpointer data)
{
	CrashType new_type = GPOINTER_TO_INT (data);
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
		druid_data.submit_type = new_type;
	
	return FALSE;
}

gboolean
on_nature_page_next (GtkWidget *page, GtkWidget *druid)
{
	if (druid_data.crash_type != CRASH_NONE)
		return FALSE;

	gnome_druid_set_page (GNOME_DRUID (druid),
			      GNOME_DRUID_PAGE (druid_data.action));
	return TRUE;
}

static void
on_refresh_button_clicked (GtkWidget *w, gpointer data)
{
	druid_data.explicit_dirty = TRUE;
	start_gdb ();
}

gboolean
on_less_page_prepare (GtkWidget *page, GtkWidget *druid)
{
	start_gdb ();
	return FALSE;
}

static void
on_stop_button_clicked (GtkWidget *button, gpointer data)
{
	GtkWidget *w;
	if (!druid_data.fd)
		return;

	w = gnome_question_dialog (_("gdb has not finished getting the"
				     "debugging information.\n"
				     "Kill the gdb process (the stack trace"
				     "will be incomplete)?"),
				   NULL, NULL);

	if (GNOME_YES == gnome_dialog_run_and_close (GNOME_DIALOG (w))) {
		if (druid_data.gdb_pid == 0) {
			g_warning (_("gdb has already exited"));
			return;
		}
		kill (druid_data.gdb_pid, SIGTERM);
		stop_gdb ();		
		kill (druid_data.app_pid, SIGCONT);
		druid_data.explicit_dirty = TRUE;
	}
}

gboolean
on_action_page_back (GtkWidget *page, GtkWidget *druid)
{
	if (druid_data.crash_type != CRASH_NONE)
		return FALSE;

	gnome_druid_set_page (GNOME_DRUID (druid),
			      GNOME_DRUID_PAGE (druid_data.nature));
	return TRUE;
}

static void
on_about_button_clicked (GtkWidget *button, gpointer data)
{
	static GtkWidget *about, *href;
	static const char *authors[] = {
		"Jacob Berkman  <jberkman@andrew.cmu.edu>",
		NULL
	};

	if (about) {
		gdk_window_show (about->window);
		gdk_window_raise (about->window);
		return;
	}
		
	about = gnome_about_new (_(PACKAGE), VERSION,
				 _("Copyright (C) 1999 Jacob Berkman"),
				 authors,
				 _("The GNOME graphical bug reporting tool"),
				 "bug-buddy.png");
	gtk_signal_connect (GTK_OBJECT (about), "destroy",
			    GTK_SIGNAL_FUNC (gtk_widget_destroyed),
			    &about);

	href = gnome_href_new ("http://www.andrew.cmu.edu/~jberkman/bug-buddy/",
			       _("The lame bug-buddy web page"));
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (about)->vbox),
			    href, FALSE, FALSE, 0);

	gtk_widget_show (href);
	gtk_widget_show (about);
}

gboolean
on_the_druid_cancel (GtkWidget *w)
{
	save_config ();
	gtk_main_quit ();
	return FALSE;
}

gboolean
on_contact_page_next (GtkWidget *page, GtkWidget *druid)
{
	GtkWidget *w;
	gchar *s;

	w = glade_xml_get_widget (druid_data.xml, "name_entry");
	s = gtk_entry_get_text (GTK_ENTRY (w));
	if (!s || strlen(s) == 0)
		goto contact_failed;
       
	w = glade_xml_get_widget (druid_data.xml, "email_entry");
	s = gtk_entry_get_text (GTK_ENTRY (w));
	if (!s || strlen(s) == 0)
		goto contact_failed;

	w = glade_xml_get_widget (druid_data.xml, "mailer_entry");
	s = gtk_entry_get_text (GTK_ENTRY (w));
	if (!s || strlen (s) == 0 || !g_file_exists (s)) {
		if (druid_data.submit_type == SUBMIT_TO_SELF ||
		    druid_data.submit_type == SUBMIT_REPORT) {
			GtkWidget *d;
			char *m;
			m = g_strdup_printf (_("'%s' doesn't seem to exist.  "
					       "You won't be able to actually\n"
					       "submit a but report, but you will "
					       "be able to save it to a file.\n\n"
					       "Specify a new location for sendmail?"),
					     s);	
			d = gnome_question_dialog (m, NULL, NULL);
			g_free (m);
			if (GNOME_YES == gnome_dialog_run_and_close (GNOME_DIALOG (d)))
				return TRUE;
		}
			    
		w = glade_xml_get_widget (druid_data.xml, "submit_radio");
		gtk_widget_hide (w);
		w = glade_xml_get_widget (druid_data.xml, "email_radio");
		gtk_widget_hide (w);		
		w = glade_xml_get_widget (druid_data.xml, "file_radio");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
					      TRUE);
		return FALSE;
	}

	w = glade_xml_get_widget (druid_data.xml, "submit_radio");
	gtk_widget_show (w);
	w = glade_xml_get_widget (druid_data.xml, "email_radio");
	gtk_widget_show (w);

	return FALSE;

 contact_failed:
	gnome_error_dialog (_("Please enter your name and email address"));
	return TRUE;
}

/* too much duplication here; fix later */
gboolean
on_complete_page_prepare (GtkWidget *page, GtkWidget *druid)
{
	gchar *name, *from, *to, *package, *subject;
	gchar *text;
	GtkWidget *w;
	int bugnum;
	to = SUBMIT_ADDRESS;

	w = glade_xml_get_widget (druid_data.xml, "email_entry");
	from = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);

	to = NULL;

	switch (druid_data.submit_type) {
	case SUBMIT_TO_SELF:
		to = g_strdup (from);
		break;
	case SUBMIT_REPORT:
		if (druid_data.bug_type == BUG_NEW) {
			to = g_strdup ("submit" SUBMIT_ADDRESS);
			break;
		}

		w = glade_xml_get_widget (druid_data.xml, "bug_number");
		bugnum = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (w));
		to = g_strdup_printf ("%d" SUBMIT_ADDRESS, bugnum);
		break;
	case SUBMIT_FILE:
		w = glade_xml_get_widget (druid_data.xml, "file_entry");
		to = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);
		break;
	case SUBMIT_NONE:
		gnome_druid_page_finish_set_text (GNOME_DRUID_PAGE_FINISH (page),
						  _("Click \"Finish\" to exit bug-buddy.\n"
						    "No bug report will be submitted."));
		return FALSE;
	default:
		g_assert_not_reached ();
		return FALSE;
	}

	w = glade_xml_get_widget (druid_data.xml, "name_entry");
	name = gtk_entry_get_text (GTK_ENTRY (w));

	w = glade_xml_get_widget (druid_data.xml, "desc_entry");
	subject = gtk_entry_get_text (GTK_ENTRY (w));

	w = glade_xml_get_widget (druid_data.xml, "package_entry");
	package = gtk_entry_get_text (GTK_ENTRY (w));
	if (!strlen(package))
		package = "general";
	
	if (druid_data.submit_type == SUBMIT_FILE)
		text = g_strdup_printf (_("You are about to save a bug report in '%s'.\n"
					  "This can later be used to report a bug in '%s'."),
					to, package);
					  
	else
		text = g_strdup_printf (_("You are about to report a bug in '%s'.\n\n"
					  "This will be submitted via email:\n"
					  "To: %s\n"
					  "From: %s <%s>\n"
					  "Subject: %s\n\n"
					  "Clicking on 'Finish' will submit the bug report."),
					package, to, name, from, subject);

	gnome_druid_page_finish_set_text (GNOME_DRUID_PAGE_FINISH (page), text);
	g_free (text);
	return FALSE;
}

static void
write_line_width (FILE *fp, char *s)
{
	gchar *sp;
	if (!s)
		return;

	if (strlen (s) < LINE_WIDTH) {
		fprintf (fp, "%s\n", s);
		return;
	}

	for (sp = s+LINE_WIDTH; sp > s && !isspace (*sp); sp--)
		;

	if (s == sp)
		sp = strpbrk (s+LINE_WIDTH, "\t\n ");
       
	if (sp)
		*sp = '\0';

	fprintf (fp, "%s\n", s);

	if (sp)
		write_line_width (fp, sp+1);
}

static void
write_line_widthv (FILE *fp, const char *s)
{
	int i;
	gchar **sv = g_strsplit (s, "\n", 0);
	
	for (i = 0; sv[i]; i++)
		write_line_width (fp, sv[i]);
	
	g_strfreev (sv);
}

gboolean
on_complete_page_finish (GtkWidget *page, GtkWidget *druid)
{
	GtkWidget *w;
	gchar *s, *s2, *s3, *subject;
	char *command;
	FILE *fp = stdout;
	int status, bugnum, i;

	if (druid_data.bug_type == BUG_NEW) {
		s2 = g_strdup ("submit" SUBMIT_ADDRESS);
	} else {	
		w = glade_xml_get_widget (druid_data.xml, "bug_number");
		bugnum = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (w));
		s2 = g_strdup_printf ("%d" SUBMIT_ADDRESS, bugnum);
	}
		
	w = glade_xml_get_widget (druid_data.xml, "email_entry");
	s = gtk_entry_get_text (GTK_ENTRY (w));

	switch (druid_data.submit_type) {
	case SUBMIT_TO_SELF:
		s2 = s;
		/* fall through */
	case SUBMIT_REPORT:
		w = glade_xml_get_widget (druid_data.xml, "mailer_entry");
		s3 = gtk_entry_get_text (GTK_ENTRY (w));
		command = g_strconcat (s3, " -i -t", NULL);
		g_message (_("about to run '%s'"), command);
		fp = popen (command, "w");	       
		if (!fp) {
			s = g_strdup_printf (_("Unable to start mail program:\n"
					       "'%s'"), s3);
			w = gnome_error_dialog (s);
			g_free (s);
			g_free (command);
			gnome_dialog_run_and_close (GNOME_DIALOG (w));
			return FALSE;
		}
		g_free (command);
		break;
	case SUBMIT_FILE:
		w = glade_xml_get_widget (druid_data.xml, "file_entry");
		s3 = gtk_entry_get_text (GTK_ENTRY (w));
		fp = fopen (s3, "w");
		if (!fp) {
			s = g_strdup_printf (_("Unable to open file:\n"
						      "'%s'"), s3);
			w = gnome_error_dialog (s);
			g_free (s);
			gnome_dialog_run_and_close (GNOME_DIALOG (w));
			return FALSE;
		}
		break;
	case SUBMIT_NONE:
		save_config ();
		gtk_main_quit ();
		return FALSE;
	default:
		g_assert_not_reached ();
		return FALSE;
	}

	fprintf (fp, "To: %s\n", s2);

	w = glade_xml_get_widget (druid_data.xml, "name_entry");
	s2 = gtk_entry_get_text (GTK_ENTRY (w));
	fprintf (fp, "From: %s <%s>\n", s2, s);

	w = glade_xml_get_widget (druid_data.xml, "desc_entry");
	subject = s = gtk_entry_get_text (GTK_ENTRY (w));
	fprintf (fp, "Subject: %s\nX-Mailer: %s %s\n", s, PACKAGE, VERSION);

	w = glade_xml_get_widget (druid_data.xml, "package_entry");
	s = gtk_entry_get_text (GTK_ENTRY (w));
	if (!strlen(s))
		s = "general";
	fprintf (fp, 
		 "\nPackage: %s\n"
		 "Severity: %s\n", s, severity[druid_data.severity]);

	w = glade_xml_get_widget (druid_data.xml, "version_entry");
	s = gtk_entry_get_text (GTK_ENTRY (w));
	fprintf (fp, 
		 "Version: %s\n\n"
		 ">Synopsis: %s\n"
		 ">Class: %s\n", s, subject, bug_class[druid_data.bug_class][1]);

	for (i = 0; i < GTK_CLIST (druid_data.version_list)->rows; i++) {
		s = s2 = NULL;
		gtk_clist_get_text (GTK_CLIST (druid_data.version_list),
				    i, 0, &s);
		gtk_clist_get_text (GTK_CLIST (druid_data.version_list),
				    i, 1, &s2);
		
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

	status = pclose (fp);
#if 0
	g_message (_("Subprocess exited with status %d"), status);
#endif
	save_config ();
	gtk_main_quit ();

	return FALSE;
}

void
on_version_list_select_row (GtkCList *list, gint row, gint col,
			    GdkEventButton *event, gpointer udata)
{	
	gchar *s;
	druid_data.selected_row = row;
	if (gtk_clist_get_text (list, row, 1, &s))
		gtk_entry_set_text (GTK_ENTRY (druid_data.version_edit), s);
	else
		gtk_editable_delete_text (GTK_EDITABLE (druid_data.version_edit),
					  0, -1);
	gtk_clist_get_text (list, row, 0, &s);
	gtk_label_set_text (GTK_LABEL (druid_data.version_label), _(s));
}

void
update_selected_row (GtkWidget *w, gpointer data)
{
	gchar *s;
	gint row;
	if (druid_data.selected_row == -1)
		return;
	row = druid_data.selected_row;
	s = gtk_entry_get_text (GTK_ENTRY (druid_data.version_edit));
	gtk_clist_set_text (GTK_CLIST (druid_data.version_list), row, 1, s);
}

void
on_file_radio_toggled (GtkWidget *radio, gpointer data)
{
	static GtkWidget *entry2 = NULL;
	int on = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radio));
	if (!entry2)
		entry2 = glade_xml_get_widget (druid_data.xml,
					       "file_entry2");
	
	gtk_widget_set_sensitive (GTK_WIDGET (entry2), on);
}

gboolean
on_action_page_next (GtkWidget *page, GtkWidget *druid)
{
	GtkWidget *entry, *d;
	char *s, *title;

	if (druid_data.submit_type != SUBMIT_FILE)
		return FALSE;

	entry = glade_xml_get_widget (druid_data.xml,
				      "file_entry");
	
	s = gtk_entry_get_text (GTK_ENTRY (entry));

	if (!(s && strlen (s))) {
		gnome_error_dialog (_("Please choose a file to save to."));
		return TRUE;
	}

	if (!g_file_exists (s))
		return FALSE;
		
	title = g_strdup_printf (_("The file '%s' already exists.\n"
				   "Overwrite this file?"), s);

	d = gnome_question_dialog (title, NULL, NULL);
	g_free (title);

	return (GNOME_NO == gnome_dialog_run_and_close (GNOME_DIALOG (d)));
}

void
on_newreport_radio_toggled (GtkWidget *w, gpointer data)
{
	GtkWidget *w2;
	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
		return;

	w2 = glade_xml_get_widget (druid_data.xml, "newbug_table");
	gtk_widget_show (w2);

	w2 = glade_xml_get_widget (druid_data.xml, "existing_box");
	gtk_widget_hide (w2);

	druid_data.bug_type = BUG_NEW;
}
void
on_existing_radio_toggled (GtkWidget *w, gpointer data)
{
	GtkWidget *w2;
	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
		return;

	w2 = glade_xml_get_widget (druid_data.xml, "newbug_table");
	gtk_widget_hide (w2);

	w2 = glade_xml_get_widget (druid_data.xml, "existing_box");
	gtk_widget_show (w2);

	druid_data.bug_type = BUG_EXISTING;
}

static gboolean
set_severity (GtkWidget *w, gpointer data)
{
	druid_data.severity = GPOINTER_TO_INT (data);
	return FALSE;
}

static gboolean
set_bug_class (GtkWidget *w, gpointer data)
{
	druid_data.bug_class = GPOINTER_TO_INT (data);
	return FALSE;
}


GtkWidget *
make_pixmap_button (gchar *widget_name, gchar *text, 
		    gchar *s2, int type, int i2)
{
	GtkWidget *p, *w;

	/* this is slightly bad but should be ok */
	p = gnome_stock_pixmap_widget (NULL, text);
	w = gnome_pixmap_button (p, text);
	GTK_WIDGET_SET_FLAGS (w, GTK_CAN_DEFAULT);
	gtk_container_set_border_width (GTK_CONTAINER (w), GNOME_PAD_SMALL);
	return w;
}

GtkWidget *
make_anim (gchar *widget_name, gchar *imgname, 
	   gchar *string2, gint size, gint freq)
{
	gchar *pixmap;
	druid_data.gdb_anim = gnome_animator_new_with_size (48, 48);
	gnome_animator_set_loop_type (GNOME_ANIMATOR (druid_data.gdb_anim),
				      GNOME_ANIMATOR_LOOP_RESTART);
	pixmap = g_strconcat (BUDDY_DATADIR, imgname, NULL);
	gnome_animator_append_frames_from_file (GNOME_ANIMATOR (druid_data.gdb_anim),
						pixmap, 0, 0, freq, size);
	g_free (pixmap);
	return druid_data.gdb_anim;
}

/* ugly stupid dumb stuff stolen from the bug reporting web page */
static gchar *
get_package_from_appname (const char *appname)
{
	FILE *fp;
	gchar *package;
	char buf[1024];
	GHashTable *table;

	fp = fopen (BUDDY_DATADIR "/prog.bugmap", "r");
	if (!fp) {
		g_warning ("Could not open bugfile '"
			   BUDDY_DATADIR
			   "/prog.bugmap' for reading.");
		return NULL;
	}

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

static void 
init_toggle (const char *name, int test, int data, GtkSignalFunc func)
{
	GtkWidget *w;
	w = glade_xml_get_widget (druid_data.xml, name);
	if (test == data)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
					      TRUE);
	gtk_signal_connect (GTK_OBJECT (w), "toggled", func,
			    GINT_TO_POINTER (data));
}

static void
init_ui ()
{
	GtkWidget *w, *m, *segv, *core;
	gchar *row[3] = { NULL };
	gchar *s, *p = NULL;
	int i;

	glade_xml_signal_autoconnect (druid_data.xml);

	load_config ();	

	w = glade_xml_get_widget (druid_data.xml, "the_druid");
	druid_data.the_druid = w;

	w = glade_xml_get_widget (druid_data.xml, "package_entry2");
	for (i = 0; packages[i]; i++)
		gnome_entry_prepend_history (GNOME_ENTRY (w), FALSE,
					     packages[i]);

	m = gtk_menu_new ();
	for (i = 0; severity[i]; i++) {
		w = gtk_menu_item_new_with_label (_(severity[i]));
		gtk_signal_connect (GTK_OBJECT (w), "activate",
				    GTK_SIGNAL_FUNC (set_severity),
				    GINT_TO_POINTER (i));
		gtk_widget_show (w);
		gtk_menu_append (GTK_MENU (m), w);
	}

	w = glade_xml_get_widget (druid_data.xml, "severity_option");
	gtk_option_menu_set_menu (GTK_OPTION_MENU (w), m);
	gtk_option_menu_set_history (GTK_OPTION_MENU (w), 2);
	druid_data.severity = 2;

	m = gtk_menu_new ();
	for (i = 0; bug_class[i][0]; i++) {
		w = gtk_menu_item_new_with_label (_(bug_class[i][0]));
		gtk_signal_connect (GTK_OBJECT (w), "activate",
				    GTK_SIGNAL_FUNC (set_bug_class),
				    GINT_TO_POINTER (i));
		gtk_widget_show (w);
		gtk_menu_append (GTK_MENU (m), w);
	}

	w = glade_xml_get_widget (druid_data.xml, "class_option");
	gtk_option_menu_set_menu (GTK_OPTION_MENU (w), m);

	/* dialog crash page */
	w = druid_data.app_file = 
		glade_xml_get_widget (druid_data.xml, "app_file");
	s = popt_data.app_file;
	if (!s) {
		s = getenv ("GNOME_CRASHED_APPNAME");
		if (s)
			g_warning (_("$GNOME_CRASHED_APPNAME is deprecated.\n"
				     "Please use the --appname command line"
				     "argument instead."));
	}

	if (s)
		gtk_entry_set_text (GTK_ENTRY (w), s);

	p = g_strdup (popt_data.package);
	if (!p && s)
		p = get_package_from_appname (s);

	if (!p)
		p = g_strdup ("general");

	w = glade_xml_get_widget (druid_data.xml, 
				  "package_entry");
	gtk_entry_set_text (GTK_ENTRY (w), p);
	g_free (p);

	w = druid_data.pid =
		glade_xml_get_widget (druid_data.xml, "crashed_pid");
	s = popt_data.pid;
	if (!s) {
		s = getenv ("GNOME_CRASHED_PID");
		if (s)
			g_warning (_("$GNOME_CRASHED_PID is deprecated.\n "
				     "Please use the --pid command line"
				     "argument instead."));
	}

	if (s) {
		gtk_entry_set_text (GTK_ENTRY (w), s);
		druid_data.crash_type = CRASH_DIALOG;
	}

	/* core crash page */
	w = druid_data.core_file = 
		glade_xml_get_widget (druid_data.xml, "core_file");
	if (popt_data.core_file) {
		gtk_entry_set_text (GTK_ENTRY (w), popt_data.core_file);
		druid_data.crash_type = CRASH_CORE;
	}

	/* package version */
	if (popt_data.package_ver) {
		w = glade_xml_get_widget (druid_data.xml, "version_entry");
		gtk_entry_set_text (GTK_ENTRY (w), popt_data.package_ver);
	}

	/* init radio buttons */
	init_toggle ("dialog_radio", druid_data.crash_type, CRASH_DIALOG, 
		     GTK_SIGNAL_FUNC (update_crash_type));

	init_toggle ("core_radio", druid_data.crash_type, CRASH_CORE,
		     GTK_SIGNAL_FUNC (update_crash_type));

	init_toggle ("no_crash_radio", druid_data.crash_type, CRASH_NONE,
		     GTK_SIGNAL_FUNC (update_crash_type));

	init_toggle ("submit_radio", druid_data.submit_type, SUBMIT_REPORT, 
		     GTK_SIGNAL_FUNC (update_submit_type));

	init_toggle ("email_radio", druid_data.submit_type, SUBMIT_TO_SELF,
		     GTK_SIGNAL_FUNC (update_submit_type));

	init_toggle ("noaction_radio", druid_data.submit_type, SUBMIT_NONE,
		     GTK_SIGNAL_FUNC (update_submit_type));

	init_toggle ("file_radio", druid_data.submit_type, SUBMIT_FILE,
		     GTK_SIGNAL_FUNC (update_submit_type));

	segv = glade_xml_get_widget (druid_data.xml, "segv_table");
	core = glade_xml_get_widget (druid_data.xml, "core_box");

	switch (druid_data.crash_type) {
	case CRASH_DIALOG:
		gtk_widget_show (segv);
		gtk_widget_hide (core);
		break;
	case CRASH_CORE:
		gtk_widget_show (core);
		gtk_widget_hide (segv);
		break;
	case CRASH_NONE:
		gtk_widget_hide (core);
		gtk_widget_hide (segv);
		break;
	}

	/* system config page */
	druid_data.version_edit =
		glade_xml_get_widget (druid_data.xml, "version_edit");
	druid_data.version_label = 
		glade_xml_get_widget (druid_data.xml, "version_label");
	druid_data.version_list = w =
		glade_xml_get_widget (druid_data.xml, "version_list");

	for (i = 0; distros[i].name; i++) {
		if (!g_file_exists (distros[i].version_file))
			continue;
		row[0] = _("Distribution");
		row[1] = distros[i].phylum->version (&distros[i]);
		gtk_clist_append (GTK_CLIST (w), row);
		druid_data.distro = &distros[i];
		break;
	}

	/* so we have like 3 passages, but it is better */
	for (i = 0; package[i].name; i++) {
		if (!package[i].pre_command)
			continue;
		package[i].version = get_line_from_command (package[i].pre_command);
	}

	if (druid_data.distro)
		druid_data.distro->phylum->packager (package);

	for (i = 0; package[i].name; i++) {
		if (!package[i].version &&
		    package[i].post_command)
			package[i].version = 
				get_line_from_command (package[i].post_command);
		row[0] = _(package[i].name);
		row[1] = package[i].version;
		gtk_clist_append (GTK_CLIST (w), row);
	}

	w = glade_xml_get_widget (druid_data.xml, "about_button");
	gtk_signal_connect (GTK_OBJECT (w), "clicked",
			    GTK_SIGNAL_FUNC (on_about_button_clicked),
			    NULL);

	/* less page */
	druid_data.stop_button = w = 
		glade_xml_get_widget (druid_data.xml, "stop_button");
	gtk_signal_connect (GTK_OBJECT (w), "clicked",
			    GTK_SIGNAL_FUNC (on_stop_button_clicked),
			    NULL);
	gtk_widget_set_sensitive (GTK_WIDGET (w), FALSE);

	druid_data.refresh_button = w =
		glade_xml_get_widget (druid_data.xml, "refresh_button");
	gtk_signal_connect (GTK_OBJECT (w), "clicked",
			    GTK_SIGNAL_FUNC (on_refresh_button_clicked),
			    NULL);
	
	/* other stuff */
	druid_data.gdb_text = glade_xml_get_widget (druid_data.xml, 
						    "gdb_text");
	druid_data.nature   = glade_xml_get_widget (druid_data.xml,
						    "nature_page");
	druid_data.attach   = glade_xml_get_widget (druid_data.xml,
						    "attach_page");
	druid_data.core     = glade_xml_get_widget (druid_data.xml,
						    "core_page");
	druid_data.less     = glade_xml_get_widget (druid_data.xml, 
						    "less_page");
	druid_data.action   = glade_xml_get_widget (druid_data.xml, 
						    "action_page");	
}

int
main (int argc, char *argv[])
{
	GtkWidget *w;
	gchar *xml_path, *s;

	memset (&druid_data, 0, sizeof (druid_data));
	memset (&popt_data,  0, sizeof (popt_data));

	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	gnome_init_with_popt_table (PACKAGE, VERSION, argc, argv, 
				    options, 0, NULL);
	glade_gnome_init ();

	xml_path = BUDDY_DATADIR "/bug-buddy.glade";
	if (!g_file_exists (xml_path)) {
		s = g_strdup_printf (_("Could not find our glade file '%s'\n"
				       "Please make sure bug-buddy was "
				       "installed correctly."), xml_path);
		w = gnome_error_dialog (s);
		gnome_dialog_run_and_close (GNOME_DIALOG (w));
		return 0;
	}
	druid_data.xml = glade_xml_new (xml_path , "druid_window");

	if (!druid_data.xml) {
		s = g_strdup_printf (_("XML file '%s' could not be loaded"), 
				     xml_path);
		w = gnome_error_dialog (s);
		gnome_dialog_run_and_close (GNOME_DIALOG (w));
		return 0;
	}

	init_ui (druid_data.xml);

	w = glade_xml_get_widget (druid_data.xml, "druid_window");
	gtk_widget_show (w);

	gtk_main ();

	return 0;
}
