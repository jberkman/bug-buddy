/* bug-buddy bug submitting program
 *
 * Copyright (C) The Free Software Foundation
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

#include "bug-buddy.h"

/* libglade callbacks */
gboolean on_nature_page_next (GtkWidget *, GtkWidget *);
gboolean on_attach_page_next (GtkWidget *, GtkWidget *);
gboolean on_core_page_back (GtkWidget *, GtkWidget *);
gboolean on_less_page_prepare (GtkWidget *, GtkWidget *);
gboolean on_less_page_back (GtkWidget *, GtkWidget *);
gboolean on_misc_page_back (GtkWidget *, GtkWidget *);
gboolean on_the_druid_cancel (GtkWidget *);
gboolean on_complete_page_prepare (GtkWidget *, GtkWidget *);
gboolean on_complete_page_finish (GtkWidget *, GtkWidget *);
gboolean on_system_page_prepare (GtkWidget *, GtkWidget *);
gboolean on_gnome_page_prepare (GtkWidget *, GtkWidget *);
gboolean on_contact_page_next (GtkWidget *, GtkWidget *);
void on_version_list_select_row (GtkCList *list, gint row, gint col,
				 GdkEventButton *event, gpointer udata);
void on_version_edit_activate (GtkEditable *editable, gpointer user_data);
void on_version_apply_clicked (GtkButton *button, gpointer udata);
void on_file_radio_toggled (GtkWidget *radio, gpointer data);
gboolean on_action_page_next (GtkWidget *page, GtkWidget *druid);

extern const char *packages[];

const gchar *severity[] = { N_("normal"),
			    N_("critical"),
			    N_("severe"),
			    N_("wishlist"),
			    NULL };

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

static ListData list_data[] = {
	{ N_("Operating System"), { "uname -a" } },
	{ N_("Distribution"),
	  { "( [ -f /etc/slackware-version ] && echo \"Slackware\") || "
	    "( [ -f /etc/debian_version ] && echo \"Debian\") || "
	    "( [ -f /etc/redhat-release ] && echo \"Red Hat\") || "
	    "( [ -f /etc/SuSE-release ]   && echo \"SuSE\") || "
	    "echo \"\"" } },
	{ N_("Distribution version"),
	  { "( [ -f /etc/slackware-version ] && cat /etc/slackware-version) || "
	    "( [ -f /etc/debian_version ] && cat /etc/debian_version) || "
	    "( [ -f /etc/redhat-release ] && cat /etc/redhat-release) || "
	    "( [ -f /etc/SuSE-release ]   && head -1 /etc/SuSE-release) || "
	    "echo \"\"" } },
	{ N_("C library"), { "rpm -q glibc",  "rpm -q libc" } },
	{ N_("C Compiler"), { "gcc --version", "cc -V" } },
	{ N_("glib"), { "glib-config --version", "rpm -q glib" } },
	{ N_("GTK+"), { "gtk-config --version", "rpm -q gtk+" } },
	{ N_("ORBit"), { "orbit-config --version", "rpm -q ORBit" } },
	{ N_("gnome-libs"), { "gnome-config --version", "rpm -q gnome-libs" } },
	{ N_("gnome-core"), { "gnome-config --modversion applets", "rpm -q gnome-core" } },
	{ NULL }
};

static const struct poptOption options[] = {
	{ "name",        0, POPT_ARG_STRING, &popt_data.name,        
	  0, N_("Contact's name"),          N_("NAME") },
	{ "email",       0, POPT_ARG_STRING, &popt_data.email,       
	  0, N_("Contact's email address"), N_("EMAIL") },
	{ "package",     0, POPT_ARG_STRING, &popt_data.package,     
	  0, N_("Package of the program"),  N_("PACKAGE") },
	{ "package-ver", 0, POPT_ARG_STRING, &popt_data.package_ver, 
	  0, N_("Version of the package"),  N_("VERSION") },
	{ "appname",     0, POPT_ARG_STRING, &popt_data.app_file,    
	  0, N_("Crashed file name"),       N_("FILE") },
	{ "pid",         0, POPT_ARG_STRING, &popt_data.pid,         
	  0, N_("PID of crashed app"),      N_("PID") },
	{ "core",        0, POPT_ARG_STRING, &popt_data.core_file,   
	  0, N_("core file from app"),      N_("FILE") },
	{ NULL } 
};

static void
save_config ()
{
	GtkWidget *w;
	gchar *s;

	w = glade_xml_get_widget (druid_data.xml, "name_entry");
	s = gtk_entry_get_text (GTK_ENTRY (w));
	gnome_config_set_string ("/bug-buddy/last/name", s);
	w = glade_xml_get_widget (druid_data.xml, "name_entry2");
	gnome_entry_prepend_history (GNOME_ENTRY (w), TRUE, s);
	gnome_entry_save_history (GNOME_ENTRY (w));

	w = glade_xml_get_widget (druid_data.xml, "email_entry");
	s = gtk_entry_get_text (GTK_ENTRY (w));
	gnome_config_set_string ("/bug-buddy/last/email", s);
	w = glade_xml_get_widget (druid_data.xml, "email_entry2");
	gnome_entry_prepend_history (GNOME_ENTRY (w), TRUE, s);
	gnome_entry_save_history (GNOME_ENTRY (w));

	w = glade_xml_get_widget (druid_data.xml, "file_entry");
	s = gtk_entry_get_text (GTK_ENTRY (w));
	gnome_config_set_string ("/bug-buddy/last/bugfile", s);

	w = glade_xml_get_widget (druid_data.xml, "file_entry2");
	w = gnome_file_entry_gnome_entry (GNOME_FILE_ENTRY (w));
	gnome_entry_save_history (GNOME_ENTRY (w));

	gnome_config_set_int ("/bug-buddy/last/submittype", 
			      druid_data.submit_type);

	gnome_config_sync ();
}

static void
load_config ()
{
	GtkWidget *w;
	gchar *s;

	w = glade_xml_get_widget (druid_data.xml, "name_entry");
	s = gnome_config_get_string ("/bug-buddy/last/name");
	if (s)
		gtk_entry_set_text (GTK_ENTRY (w), s);
	g_free (s);
	w = glade_xml_get_widget (druid_data.xml, "name_entry2");
	gnome_entry_load_history (GNOME_ENTRY (w));

	w = glade_xml_get_widget (druid_data.xml, "email_entry");
	s = gnome_config_get_string ("/bug-buddy/last/email");
	if (s)
		gtk_entry_set_text (GTK_ENTRY (w), s);
	g_free (s);
	w = glade_xml_get_widget (druid_data.xml, "email_entry2");
	gnome_entry_load_history (GNOME_ENTRY (w));

	w = glade_xml_get_widget (druid_data.xml, "file_entry");
	s = gnome_config_get_string ("/bug-buddy/last/bugfile");
	if (s)
		gtk_entry_set_text (GTK_ENTRY (w), s);
	g_free (s);

	w = glade_xml_get_widget (druid_data.xml, "file_entry2");
	w = gnome_file_entry_gnome_entry (GNOME_FILE_ENTRY (w));
	gnome_entry_load_history (GNOME_ENTRY (w));

	druid_data.submit_type = 
		gnome_config_get_int ("/bug-buddy/last/submittype=0");
	
}

static gboolean
update_crash_type (GtkWidget *w, gpointer data)
{
	CrashType new_type = GPOINTER_TO_INT (data);
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
		druid_data.crash_type = new_type;
	
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
	GtkWidget *newpage;
	switch (druid_data.crash_type) {
	case CRASH_DIALOG:
		return FALSE;
		break;
	case CRASH_CORE:
		newpage = druid_data.core;
		break;
	default:
		newpage = druid_data.misc;
		break;
	}
	gnome_druid_set_page (GNOME_DRUID (druid),
			      GNOME_DRUID_PAGE (newpage));
	return TRUE;
}

gboolean
on_attach_page_next (GtkWidget *page, GtkWidget *druid)
{
	gnome_druid_set_page (GNOME_DRUID (druid),
			      GNOME_DRUID_PAGE (druid_data.less));
	return TRUE;
}

gboolean
on_core_page_back (GtkWidget *page, GtkWidget *druid)
{
	gnome_druid_set_page (GNOME_DRUID (druid),
			      GNOME_DRUID_PAGE (druid_data.nature));
	return TRUE;
}

gboolean
on_less_page_prepare (GtkWidget *page, GtkWidget *druid)
{
	static gchar *old_app = NULL;
	static gchar *old_extra = NULL;
	static CrashType old_type = -1;
	
	gchar *app = NULL, *extra = NULL;

	g_message (_("obtaining stack trace..."));
	
	switch (druid_data.crash_type) {
	case CRASH_DIALOG:
		app = gtk_entry_get_text (GTK_ENTRY (druid_data.app_file));
		extra = gtk_entry_get_text (GTK_ENTRY (druid_data.pid));
		if ((old_type != CRASH_DIALOG) ||
		    (!old_app || strcmp (app, old_app)) ||
		    (!old_extra || strcmp (extra, old_extra))) {
			get_trace_from_pair (app, extra);
		}
		break;
	case CRASH_CORE:
		extra = gtk_entry_get_text (GTK_ENTRY (druid_data.core_file));
		if ((old_type != CRASH_CORE) ||
		    (!old_extra || strcmp (extra, old_extra))) {
			get_trace_from_core (extra);
		}
		break;
	default:
		g_assert_not_reached ();
		break;
	}

	g_free (old_extra);
	old_extra = g_strdup (extra);

	g_free (old_app);
	old_app = g_strdup (app);

	old_type = druid_data.crash_type;

	return FALSE;
}

gboolean
on_less_page_back (GtkWidget *page, GtkWidget *druid)
{
	GtkWidget *newpage = NULL;
	switch (druid_data.crash_type) {
	case CRASH_DIALOG:
		newpage = druid_data.attach;
		break;
	case CRASH_CORE:
		newpage = druid_data.core;
		break;
	default:
		g_assert_not_reached ();
		break;
	}
	gnome_druid_set_page (GNOME_DRUID (druid),
			      GNOME_DRUID_PAGE (newpage));
	return TRUE;
}

gboolean
on_misc_page_back (GtkWidget *page, GtkWidget *druid)
{
	if (druid_data.crash_type != CRASH_NONE)
		return FALSE;

	gnome_druid_set_page (GNOME_DRUID (druid),
			      GNOME_DRUID_PAGE (druid_data.nature));
	return TRUE;
}

gboolean
on_the_druid_cancel (GtkWidget *w)
{
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

	to = SUBMIT_ADDRESS;

	w = glade_xml_get_widget (druid_data.xml, "email_entry");
	from = gtk_entry_get_text (GTK_ENTRY (w));

	switch (druid_data.submit_type) {
	case SUBMIT_TO_SELF:
		to = from;
		break;
	case SUBMIT_REPORT:
		break;
	case SUBMIT_FILE:
		w = glade_xml_get_widget (druid_data.xml, "file_entry");
		to = gtk_entry_get_text (GTK_ENTRY (w));
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

gboolean
on_complete_page_finish (GtkWidget *page, GtkWidget *druid)
{
	GtkWidget *w;
	gchar *s, *s2, *s3;
	FILE *fp = stdout;
	ListData *data;	

	s2 = SUBMIT_ADDRESS;

	w = glade_xml_get_widget (druid_data.xml, "email_entry");
	s = gtk_entry_get_text (GTK_ENTRY (w));

	switch (druid_data.submit_type) {
	case SUBMIT_TO_SELF:
		s2 = s;
		/* fall through */
	case SUBMIT_REPORT:
		s3 = g_strconcat (druid_data.mail_cmd, s2, NULL);
		g_message ("about to run '%s'", s3);
		fp = popen (s3, "w");
		if (!fp) {
			s = g_strdup_printf (_("Unable to start mail program:\n"
					       "'%s'"), druid_data.mail_cmd);
			w = gnome_error_dialog (s);
			g_free (s);
			gnome_dialog_run_and_close (GNOME_DIALOG (w));
			return FALSE;
		}
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

	fprintf (fp, "From %s\nTo: %s\n", s, s2);

	w = glade_xml_get_widget (druid_data.xml, "name_entry");
	s2 = gtk_entry_get_text (GTK_ENTRY (w));
	fprintf (fp, "From: %s <%s>\n", s2, s);

	w = glade_xml_get_widget (druid_data.xml, "desc_entry");
	s = gtk_entry_get_text (GTK_ENTRY (w));
	fprintf (fp, "Subject: %s\nX-Mailer: %s %s\n", s, PACKAGE, VERSION);

	w = glade_xml_get_widget (druid_data.xml, "package_entry");
	s = gtk_entry_get_text (GTK_ENTRY (w));
	if (!strlen(s))
		s = "general";
	fprintf (fp, "\nPackage: %s\n", s);

	fprintf (fp, "Severity: %s\n", severity[druid_data.severity]);

	w = glade_xml_get_widget (druid_data.xml, "version_entry");
	s = gtk_entry_get_text (GTK_ENTRY (w));
	fprintf (fp, "Version: %s\n\n", s);

	for (data  = list_data; data->label; data++) {
		s = NULL;
		gtk_clist_get_text (GTK_CLIST (druid_data.version_list),
				    data->row, 1, &s);
		if (s && strlen (s))
			fprintf (fp, "%s: %s\n", data->label, s);
	}

	w = glade_xml_get_widget (druid_data.xml, "repeat_area");
	s = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);
	fprintf (fp, "\nHow to repeat:\n\n%s\n", s);
	g_free (s);

	w = glade_xml_get_widget (druid_data.xml, "extra_area");
	s = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);
	fprintf (fp, "\nExtra information:\n\n%s\n", s);
	g_free (s);

	w = glade_xml_get_widget (druid_data.xml, "gdb_less");      
	fprintf (fp, "\nDebugging information:\n\n");
	fflush (fp);
	gnome_less_write_fd (GNOME_LESS (w), fileno (fp));

	fclose (fp);

	save_config ();

	gtk_main_quit ();

	return FALSE;
}

void
on_version_list_select_row (GtkCList *list, gint row, gint col,
			    GdkEventButton *event, gpointer udata)
{	
	gchar *s;
	druid_data.selected_data = gtk_clist_get_row_data (list, row);
	gtk_clist_get_text (list, row, 1, &s);
	gtk_entry_set_text (GTK_ENTRY (druid_data.version_edit), s);
	gtk_clist_get_text (list, row, 0, &s);
	gtk_label_set_text (GTK_LABEL (druid_data.version_label), _(s));
}

static void
update_selected_row ()
{
	gchar *s;
	gint row;
	if (!druid_data.selected_data)
		return;
	row = druid_data.selected_data->row;
	s = gtk_entry_get_text (GTK_ENTRY (druid_data.version_edit));
	gtk_clist_set_text (GTK_CLIST (druid_data.version_list), row, 1, s);}

void
on_version_edit_activate (GtkEditable *editable, gpointer user_data)
{
	update_selected_row ();
}

void
on_version_apply_clicked (GtkButton *button, gpointer udata)
{
	update_selected_row ();
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
	GtkWidget *entry;
	char *s;

	if (druid_data.submit_type != SUBMIT_FILE)
		return FALSE;

	entry = glade_xml_get_widget (druid_data.xml,
				      "file_entry");
	
	s = gtk_entry_get_text (GTK_ENTRY (entry));

	if (s && strlen (s))
		return FALSE;

	gnome_error_dialog (_("Please choose a file to save to."));
	return TRUE;
}

static gchar *
get_data_from_command (const gchar *cmd)
{
	static gchar buf[1024];
	FILE *fp;

	fp = popen (cmd, "r");

	if (!fp) {
		g_message (_("Could not run command '%s'"), cmd);
		return NULL;
	}

	if (!fgets (buf, 1024, fp)) {
		g_message (_("Error reading input of '%s'"), cmd);
		return NULL;
	} 

	fclose (fp);

	return g_strchomp (buf);	
}

static gboolean
set_severity (GtkWidget *w, gpointer data)
{
	druid_data.severity = GPOINTER_TO_INT (data);
	return FALSE;
}

static void
init_ui (GladeXML *xml)
{
	GtkWidget *w, *m;
	ListData *data;
	gchar *row[3] = { NULL };
	gchar *s;
	int i;

	glade_xml_signal_autoconnect(xml);

	load_config ();

	w = glade_xml_get_widget (xml, "the_druid");
	druid_data.the_druid = w;

	w = glade_xml_get_widget (xml, "package_entry2");
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
	w = glade_xml_get_widget (xml, "severity_option");
	gtk_option_menu_set_menu (GTK_OPTION_MENU (w), m);

	/* dialog crash page */
	w = glade_xml_get_widget (xml, "app_file");
	if (popt_data.app_file)
		s = popt_data.app_file;
	else
		s = getenv ("GNOME_CRASHED_APPNAME");

	if (s) {
		gtk_entry_set_text (GTK_ENTRY (w), s);
		druid_data.crash_type = CRASH_DIALOG;
	}
	druid_data.app_file = w;

	w = glade_xml_get_widget (xml, "crashed_pid");
	if (popt_data.pid)
		s = popt_data.pid;
	else
		s = getenv ("GNOME_CRASHED_PID");

	if (s) {
		gtk_entry_set_text (GTK_ENTRY (w), s);
		druid_data.crash_type = CRASH_DIALOG;
	}
	druid_data.pid = w;

	/* core crash page */
	w = glade_xml_get_widget (xml, "core_file");
	if (popt_data.core_file) {
		gtk_entry_set_text (GTK_ENTRY (w), popt_data.core_file);
		druid_data.crash_type = CRASH_CORE;
	}
	druid_data.core_file = w;


	/* init radio buttons */
	w = glade_xml_get_widget (xml, "dialog_radio");
	if (druid_data.crash_type == CRASH_DIALOG)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
					      TRUE);
	gtk_signal_connect (GTK_OBJECT (w), "toggled",
			    GTK_SIGNAL_FUNC (update_crash_type),
			    GINT_TO_POINTER (CRASH_DIALOG));

	w = glade_xml_get_widget (xml, "core_radio");
	if (druid_data.crash_type == CRASH_CORE)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), 
					      TRUE);
	gtk_signal_connect (GTK_OBJECT (w), "toggled",
			    GTK_SIGNAL_FUNC (update_crash_type),
			    GINT_TO_POINTER (CRASH_CORE));

	w = glade_xml_get_widget (xml, "no_crash_radio");
	gtk_signal_connect (GTK_OBJECT (w), "toggled",
			    GTK_SIGNAL_FUNC (update_crash_type),
			    GINT_TO_POINTER (CRASH_NONE));

	w = glade_xml_get_widget (xml, "submit_radio");
	if (druid_data.submit_type == SUBMIT_REPORT)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
					      TRUE);
	gtk_signal_connect (GTK_OBJECT (w), "toggled",
			    GTK_SIGNAL_FUNC (update_submit_type),
			    GINT_TO_POINTER (SUBMIT_REPORT));

	w = glade_xml_get_widget (xml, "email_radio");
	if (druid_data.submit_type == SUBMIT_TO_SELF)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
					      TRUE);
	gtk_signal_connect (GTK_OBJECT (w), "toggled",
			    GTK_SIGNAL_FUNC (update_submit_type),
			    GINT_TO_POINTER (SUBMIT_TO_SELF));

	w = glade_xml_get_widget (xml, "noaction_radio");
	if (druid_data.submit_type == SUBMIT_NONE)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
					      TRUE);
	gtk_signal_connect (GTK_OBJECT (w), "toggled",
			    GTK_SIGNAL_FUNC (update_submit_type),
			    GINT_TO_POINTER (SUBMIT_NONE));
	
	w = glade_xml_get_widget (xml, "file_radio");
	if (druid_data.submit_type == SUBMIT_FILE)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
					      TRUE);
	gtk_signal_connect (GTK_OBJECT (w), "toggled",
			    GTK_SIGNAL_FUNC (update_submit_type),
			    GINT_TO_POINTER (SUBMIT_FILE));	

	/* system config page */
	druid_data.version_edit = glade_xml_get_widget (xml, "version_edit");
	druid_data.version_label = glade_xml_get_widget (xml, "version_label");
	druid_data.version_list = w =
		glade_xml_get_widget (xml, "version_list");
	for (data = list_data; data->label; data++) {
		for (i = 0; data->cmds[i] && !row[1]; i++)
			row[1] = get_data_from_command (data->cmds[i]);
		if (!row[1]) {
			data->row = -1;
			continue;
		}
		row[0] = _(data->label);
		data->row = gtk_clist_append (GTK_CLIST (w), row);
		gtk_clist_set_row_data (GTK_CLIST (w), data->row, data);
		row[1] = NULL;
	}

	/* less page */
	druid_data.gdb_text = glade_xml_get_widget (xml, "gdb_text");

	druid_data.nature = glade_xml_get_widget (xml, "nature_page");
	druid_data.attach = glade_xml_get_widget (xml, "attach_page");
	druid_data.core = glade_xml_get_widget (xml, "core_page");
	druid_data.less = glade_xml_get_widget (xml, "less_page");
	druid_data.misc = glade_xml_get_widget (xml, "misc_page");
}

int
main (int argc, char *argv[])
{
	GtkWidget *window;
	gchar *xml_path;

	memset (&druid_data, 0, sizeof (druid_data));

	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	gnome_init_with_popt_table (PACKAGE, VERSION, argc, argv, options, 0, NULL);
	glade_gnome_init ();


	/* from gnome-bug */
	if (g_file_exists ("/usr/sbin/sendmail"))
		druid_data.mail_cmd = "/usr/sbin/sendmail -t ";
	else if (g_file_exists ("/usr/lib/sendmail"))
		druid_data.mail_cmd = "/usr/lib/sendmail -t ";
	else if ( !(druid_data.mail_cmd = 
		    gnome_is_program_in_path ("rmail ")) ) {
		GtkWidget *d = gnome_error_dialog (_("Could not find a mail program.\n"));
		gnome_dialog_run_and_close (GNOME_DIALOG (d));
		return 0;
	}


	xml_path = gnome_datadir_file ("bug-buddy/bug-buddy.glade");
	if (!xml_path) {
		GtkWidget *d = gnome_error_dialog (_("Could not find bug-buddy.glade file.\n"
						     "Please make sure bug-buddy was installed correctly."));
		gnome_dialog_run_and_close (GNOME_DIALOG (d));
		return 0;
	}
	druid_data.xml = glade_xml_new (xml_path , "druid_window");

	if (!druid_data.xml) {
		GtkWidget *d;
		gchar *s;
		s = g_strdup_printf (_("XML file '%s' could not be loaded"), xml_path);
		d = gnome_error_dialog (s);
		gnome_dialog_run_and_close (GNOME_DIALOG (d));
		return 0;
	}
	g_free (xml_path);

	init_ui (druid_data.xml);

	window = glade_xml_get_widget (druid_data.xml, "druid_window");
	gtk_widget_show (window);

	gtk_main ();

	return 0;
}
