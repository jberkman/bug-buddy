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

#include "gdb-buddy.h"

/* libglade callbacks */
gboolean on_nature_page_next (GtkWidget *, GtkWidget *);
gboolean on_attach_page_next (GtkWidget *, GtkWidget *);
gboolean on_core_page_back (GtkWidget *, GtkWidget *);
gboolean on_less_page_prepare (GtkWidget *, GtkWidget *);
gboolean on_less_page_back (GtkWidget *, GtkWidget *);
gboolean on_misc_page_back (GtkWidget *, GtkWidget *);
gboolean on_the_druid_cancel (GtkWidget *);
gboolean on_complete_page_finish (GtkWidget *, GtkWidget *);
gboolean on_system_page_prepare (GtkWidget *, GtkWidget *);
gboolean on_gnome_page_prepare (GtkWidget *, GtkWidget *);
gboolean on_contact_page_next (GtkWidget *, GtkWidget *);

extern const char *packages[];

#define COMMAND_SIZE 5

const gchar *severity[] = { N_("normal"),
			    N_("critical"),
			    N_("severe"),
			    N_("wishlist"),
			    NULL };

typedef enum {
	CRASH_DIALOG,
	CRASH_CORE,
	CRASH_NONE
} CrashType;

typedef enum {
	SUBMIT_REPORT,
	SUBMIT_TO_SELF,
	SUBMIT_NONE
} SubmitType;	

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

struct {
	GtkWidget *nature;
	GtkWidget *attach;
	GtkWidget *core;
	GtkWidget *less;
	GtkWidget *misc;
	
	GtkWidget *gdb_less;
	GtkWidget *app_file;
	GtkWidget *pid;
	GtkWidget *core_file;

	gchar *mail_cmd;
	CrashType crash_type;
	SubmitType submit_type;
	int severity;

	GladeXML *xml;
} druid_data;

typedef struct {
	const gchar *xml_key;
	const gchar *mail_header;
	const gchar *cmds[COMMAND_SIZE];
	GtkWidget *widget;
} EntryData;

static EntryData sys_data[] = {
	{ "os_entry",      "Operating System: %s\n", { "uname -a" } },
	{ "distro_entry",  "Distribution: %s\n",
	  { "( [ -f /etc/debian_version ] && cat /etc/debian_version) ||"
	    "( [ -f /etc/redhat-release ] && cat /etc/redhat-release) ||"
	    "( [ -f /etc/SuSE-release ]   && head -1 /etc/SuSE-release) ||"
	    "echo \"\"" } },
	{ "libc_entry",    "libc: %s\n", { "rpm -q glibc",  "rpm -q libc" } },
	{ "cc_entry", "C compiler: %s\n\n", { "gcc --version", "cc -V" } },
	{ NULL }
};

static EntryData gnome_data[] = {
	{ "glib_entry", "glib: %s\n", 
	  { "glib-config --version", "rpm -q glib" } },
	{ "gtk_entry",     "gtk+: %s\n",
	  { "gtk-config --version", "rpm -q gtk+" } },
	{ "orbit_entry",   "ORBit: %s\n",
	  { "orbit-config --version", "rpm -q ORBit" } },
	{ "libs_entry",    "gnome-libs: %s\n",
	  { "gnome-config --version", "rpm -q gnome-libs" } },
	{ "core_entry",    "gnome-core: %s\n",
	  { "gnome-config --modversion applets", "rpm -q gnome-core" } },
	{ NULL }
};

static const struct poptOption options[] = {
	{ "name",        0, POPT_ARG_STRING, &popt_data.name,        0, N_("Contact's name"),          N_("NAME")},
	{ "email",       0, POPT_ARG_STRING, &popt_data.email,       0, N_("Contact's email address"), N_("EMAIL")},

	{ "package",     0, POPT_ARG_STRING, &popt_data.package,     0, N_("Package of the program"),  N_("PACKAGE")},
	{ "package-ver", 0, POPT_ARG_STRING, &popt_data.package_ver, 0, N_("Version of the package"),  N_("VERSION")},
	
	{ "appname",     0, POPT_ARG_STRING, &popt_data.app_file,    0, N_("Crashed file name"),       N_("FILE")},
	{ "pid",         0, POPT_ARG_STRING, &popt_data.pid,         0, N_("PID of crashed app"),      N_("PID")},
	{ "core",        0, POPT_ARG_STRING, &popt_data.core_file,   0, N_("core file from app"),      N_("FILE")},
	{NULL } };

static gchar *
get_text_from_entry (EntryData *entry)
{
	g_return_val_if_fail (entry, NULL);
	if (!entry->widget)
		entry->widget = glade_xml_get_widget (druid_data.xml, entry->xml_key);
	return gtk_entry_get_text (GTK_ENTRY (entry->widget));
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
			get_trace_from_pair (GNOME_LESS (druid_data.gdb_less), 
					     app, extra);
		}
		break;
	case CRASH_CORE:
		extra = gtk_entry_get_text (GTK_ENTRY (druid_data.core_file));
		if ((old_type != CRASH_CORE) ||
		    (!old_extra || strcmp (extra, old_extra))) {
			get_trace_from_core (GNOME_LESS (druid_data.gdb_less), 
					     extra);
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
	w = gnome_error_dialog ("Please enter your name and email address");
	return TRUE;
}

gboolean
on_complete_page_finish (GtkWidget *page, GtkWidget *druid)
{
	GtkWidget *w;
	gchar *s, *s2, *s3;
	FILE *fp = stdout;
	EntryData *entry;
	int i;

	w = glade_xml_get_widget (druid_data.xml, "email_entry");
	s = gtk_entry_get_text (GTK_ENTRY (w));

	switch (druid_data.submit_type) {
	case SUBMIT_REPORT:
		s2 = "submit@bugs.gnome.org";
		break;
	case SUBMIT_TO_SELF:
		s2 = s;
		break;
	case SUBMIT_NONE:
		return FALSE;
	default:
		g_assert_not_reached ();
		return FALSE;
	}

	s3 = g_strconcat (druid_data.mail_cmd, s2, NULL);
	g_message ("about to run '%s'", s3);
	fp = popen (s3, "w");
	if (!fp) {
		gchar *s = g_strdup_printf (_("Unable to start mail program:"
					      "'%s'"), druid_data.mail_cmd);
		GtkWidget *d = gnome_error_dialog (s);
		g_free (s);
		gnome_dialog_run_and_close (GNOME_DIALOG (d));
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
	fprintf (fp, "\nPackage: %s\n", s);

	fprintf (fp, "Severity: %s\n", severity[druid_data.severity]);

	w = glade_xml_get_widget (druid_data.xml, "version_entry");
	s = gtk_entry_get_text (GTK_ENTRY (w));
	fprintf (fp, "Version: %s\n\n", s);

	for (entry = sys_data; entry->xml_key; entry++) {	
		if (!entry->mail_header)
			continue;
		s = get_text_from_entry (entry);
		fprintf (fp, entry->mail_header, s);
	}

	for (entry = gnome_data; entry->xml_key; entry++) {	
		if (!entry->mail_header)
			continue;
		s = get_text_from_entry (entry);
		fprintf (fp, entry->mail_header, s);
	}

	w = glade_xml_get_widget (druid_data.xml, "extra_area");
	s = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);
	fprintf (fp, "\nExtra information:\n\n%s\n", s);
	g_free (s);

	w = glade_xml_get_widget (druid_data.xml, "gdb_less");      
	fprintf (fp, "\nDebugging information:\n\n");
	fflush (fp);
	gnome_less_write_fd (GNOME_LESS (w), fileno (fp));

	fclose (fp);

	w = glade_xml_get_widget (druid_data.xml, "name_entry2");
	gnome_entry_save_history (GNOME_ENTRY (w));

	w = glade_xml_get_widget (druid_data.xml, "email_entry2");
	gnome_entry_save_history (GNOME_ENTRY (w));

	gtk_main_quit ();

	return FALSE;
}

static gchar *
get_data_from_command (const gchar *cmd)
{
	static gchar buf[1024];
	FILE *fp;
	gint len;

	/*g_message (_("about to run `%s`"), cmd);*/
	
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
init_page (EntryData *entry)
{
	gchar *s;
	int i, j;	
	
	g_return_val_if_fail (entry, FALSE);

	for (; entry->xml_key; entry++) {
		s = NULL;
		if (!entry->widget)
			entry->widget = glade_xml_get_widget (druid_data.xml, 
							      entry->xml_key);
		for (j=0; entry->cmds[j] && !s; j++)
			s = get_data_from_command (entry->cmds[j]);
		if (s && entry->widget)
			gtk_entry_set_text (GTK_ENTRY (entry->widget), s);
	}
	return TRUE;
}

gboolean
on_system_page_prepare (GtkWidget *page, GtkWidget *druid)
{
	static gboolean done = FALSE;
	if (!done)
		done = init_page (sys_data);
	return FALSE;
}

gboolean
on_gnome_page_prepare (GtkWidget *page, GtkWidget *druid)
{
	static gboolean done = FALSE;
	if (!done)
		done = init_page (gnome_data);
	return FALSE;
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
	gchar *s;
	int i;

	glade_xml_signal_autoconnect(xml);

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

	w = glade_xml_get_widget (xml, "dialog_radio");
	gtk_signal_connect (GTK_OBJECT (w), "toggled",
			    GTK_SIGNAL_FUNC (update_crash_type),
			    GINT_TO_POINTER (CRASH_DIALOG));

	w = glade_xml_get_widget (xml, "core_radio");
	gtk_signal_connect (GTK_OBJECT (w), "toggled",
			    GTK_SIGNAL_FUNC (update_crash_type),
			    GINT_TO_POINTER (CRASH_CORE));

	w = glade_xml_get_widget (xml, "no_crash_radio");
	gtk_signal_connect (GTK_OBJECT (w), "toggled",
			    GTK_SIGNAL_FUNC (update_crash_type),
			    GINT_TO_POINTER (CRASH_NONE));

	w = glade_xml_get_widget (xml, "submit_radio");
	gtk_signal_connect (GTK_OBJECT (w), "toggled",
			    GTK_SIGNAL_FUNC (update_submit_type),
			    GINT_TO_POINTER (SUBMIT_REPORT));

	w = glade_xml_get_widget (xml, "email_radio");
	gtk_signal_connect (GTK_OBJECT (w), "toggled",
			    GTK_SIGNAL_FUNC (update_submit_type),
			    GINT_TO_POINTER (SUBMIT_TO_SELF));

	w = glade_xml_get_widget (xml, "noaction_radio");
	gtk_signal_connect (GTK_OBJECT (w), "toggled",
			    GTK_SIGNAL_FUNC (update_submit_type),
			    GINT_TO_POINTER (SUBMIT_NONE));
	
	w = glade_xml_get_widget (xml, "name_entry2");
	gnome_entry_load_history (GNOME_ENTRY (w));

	w = glade_xml_get_widget (xml, "email_entry2");
	gnome_entry_load_history (GNOME_ENTRY (w));

	/* dialog crash page */
	w = glade_xml_get_widget (xml, "app_file");
	s = getenv ("GNOME_CRASHED_APPNAME");
	if (popt_data.app_file)
		gtk_entry_set_text (GTK_ENTRY (w), popt_data.app_file);
	else if (s)
		gtk_entry_set_text (GTK_ENTRY (w), s);
	druid_data.app_file = w;

	w = glade_xml_get_widget (xml, "crashed_pid");
	s = getenv ("GNOME_CRASHED_PID");
	if (popt_data.pid)
		gtk_entry_set_text (GTK_ENTRY (w), popt_data.pid);
	else if (s)
		gtk_entry_set_text (GTK_ENTRY (w), s);
	druid_data.pid = w;


        /* core crash page */
	w = glade_xml_get_widget (xml, "core_file");
	if (popt_data.core_file)
		gtk_entry_set_text (GTK_ENTRY (w), popt_data.core_file);
	druid_data.core_file = w;

	/* less page */
	druid_data.gdb_less = glade_xml_get_widget (xml, "gdb_less");

	druid_data.nature = glade_xml_get_widget (xml, "nature_page");
	druid_data.attach = glade_xml_get_widget (xml, "attach_page");
	druid_data.core = glade_xml_get_widget (xml, "core_page");
	druid_data.less = glade_xml_get_widget (xml, "less_page");
	druid_data.misc = glade_xml_get_widget (xml, "misc_page");
}

int
main (int argc, char *argv[])
{
	gchar *xml_path;

	memset (&popt_data, 0, sizeof (popt_data));
	memset (&druid_data, 0, sizeof (druid_data));

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

	gtk_main ();

	return 0;
}
