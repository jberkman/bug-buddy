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

#include <gnome.h>
#include <libgnomeui/gnome-window-icon.h>
#include <glade/glade.h>

#include <gnome-xml/tree.h>
#include <gnome-xml/parser.h>

#include <signal.h>
#include <dirent.h>

#include "bug-buddy.h"
#include "util.h"
#include "distro.h"
#include "glade-druid.h"
#include "bts.h"
#include "ctree-combo.h"

/* libglade callbacks */
gboolean on_front_page_next (GtkWidget *, GnomeDruid *);
gboolean delete_me (GtkWidget *, GdkEventAny *, gpointer data);
gboolean on_nature_page_next  (GtkWidget *, GnomeDruid *);
gboolean on_desc_page_next (GtkWidget *, GnomeDruid *);
gboolean on_less_page_prepare (GtkWidget *, GtkWidget *);
gboolean on_less_page_next    (GtkWidget *, GtkWidget *);
gboolean on_misc_page_back    (GtkWidget *, GtkWidget *);
gboolean on_the_druid_cancel  (GtkWidget *);
gboolean on_complete_page_prepare (GtkWidget *, GtkWidget *);
gboolean on_complete_page_finish  (GtkWidget *, GtkWidget *);
gboolean on_gnome_page_prepare    (GtkWidget *, GtkWidget *);
gboolean on_version_page_back (GtkWidget *page, GnomeDruid *druid);
gboolean on_contact_page_next     (GtkWidget *, GtkWidget *);
void on_version_list_select_row   (GtkCList *list, gint row, gint col,
				   GdkEventButton *event, gpointer udata);

void update_selected_row      (GtkWidget *widget, gpointer data);
void on_version_apply_clicked (GtkButton *button, gpointer data);
void on_file_radio_toggled    (GtkWidget *radio, gpointer data);
gboolean on_action_page_next  (GtkWidget *page, GtkWidget *druid);
gboolean on_action_page_back  (GtkWidget *page, GnomeDruid *druid);
gboolean on_action_page_prepare (GtkWidget *page, GnomeDruid *druid);
GtkWidget *make_anim (gchar *widget_name, gchar *string1, 
		      gchar *string2, gint int1, gint int2);
GtkWidget *make_pixmap_button (gchar *widget_name, gchar *string1, 
			       gchar *string2, gint int1, gint int2);
gboolean on_more_page_next (GnomeDruidPage *page, GnomeDruid *druid);
gboolean on_contact_page_prepare (GnomeDruidPage *page, GnomeDruid *druid);
gboolean on_version_page_prepare (GnomeDruidPage *page, GnomeDruid *druid);

const gchar *severity[] = { 
	N_("critical"),
	N_("grave"),
	N_("normal"),
	N_("wishlist"),
	NULL 
};

const gchar *bug_class[][2] = {
	{ N_("software bug"),      "sw-bug" },
	{ N_("documentation bug"), "doc-bug" },
	{ N_("change request"),    "change-request" },
	{ N_("support"),           "support" },
	{ NULL, NULL }
};

const gchar *submit_type[] = {
	N_("Submit bug report"),
	N_("Only send report to yourself"),
	N_("Save report to file"),
	NULL
};

const gchar *crash_type[] = {
	N_("Collect debugging information from a crashed application"),
	N_("Collect debugging information from a core file"),
	N_("Skip this step"),
	NULL
};

const gchar *bug_type[] = {
	N_("Create a new bug report"),
	N_("Add more information to existing report"),
	NULL
};

PoptData popt_data;

DruidData druid_data;

static const struct poptOption options[] = {
	{ "name",        0, POPT_ARG_STRING, &popt_data.name,        0, N_("Name of contact"),                 N_("NAME") },
	{ "email",       0, POPT_ARG_STRING, &popt_data.email,       0, N_("Email address of contact"),        N_("EMAIL") },
	{ "package",     0, POPT_ARG_STRING, &popt_data.package,     0, N_("Package containing the program"),  N_("PACKAGE") },
	{ "package-ver", 0, POPT_ARG_STRING, &popt_data.package_ver, 0, N_("Version of the package"),          N_("VERSION") },
	{ "appname",     0, POPT_ARG_STRING, &popt_data.app_file,    0, N_("File name of crashed program"),    N_("FILE") },
	{ "pid",         0, POPT_ARG_STRING, &popt_data.pid,         0, N_("PID of crashed program"),          N_("PID") },
	{ "core",        0, POPT_ARG_STRING, &popt_data.core_file,   0, N_("Core file from program"),          N_("FILE") },
	{ NULL } 
};

#if 0
static void
set_icon_on_window (GtkWidget *w, gpointer data)
{
	GdkPixbuf *pixbuf;
	static GdkPixmap *pixmap;
	static GdkBitmap *bitmap;
	
	pixbuf = gdk_pixbuf_new_from_file (BUDDY_ICONDIR"/bug-buddy.png");
	if (!pixbuf)
		return;

	gdk_pixbuf_render_pixmap_and_mask (pixbuf, &pixmap, &bitmap, 128);
	gdk_window_set_icon (w->window, NULL, pixmap, bitmap);
}
#endif

static void
save_entry (const char *name, const char *name2, const char *save_path)
{
	GtkWidget *w;
	gchar *s;

	w = GET_WIDGET (name);
	s = gtk_entry_get_text (GTK_ENTRY (w));
	gnome_config_set_string (save_path, s);
	w = GET_WIDGET (name2);
	if (GNOME_IS_FILE_ENTRY (w))
		w = gnome_file_entry_gnome_entry (GNOME_FILE_ENTRY (w));
	gnome_entry_prepend_history (GNOME_ENTRY (w), TRUE, s);
	gnome_entry_save_history (GNOME_ENTRY (w));
}

static void
save_config ()
{
	GtkWidget *w;
	gboolean b;

	save_entry ("name_entry",   "name_entry2",   "/bug-buddy/last/name");
	save_entry ("email_entry",  "email_entry2",  "/bug-buddy/last/email");
	save_entry ("file_entry",   "file_entry2",   "/bug-buddy/last/bugfile");
	save_entry ("mailer_entry", "mailer_entry2", "/bug-buddy/last/mailer");

	w = gnome_file_entry_gnome_entry (
		GNOME_FILE_ENTRY (GET_WIDGET ("include_entry2")));
	gnome_entry_save_history (GNOME_ENTRY (w));

	b = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (SKIP_CONF));
	gnome_config_set_bool ("/bug-buddy/last/skip_conf", b);
	gnome_config_set_int ("/bug-buddy/last/submittype", 
			      druid_data.submit_type);
	b = gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (GET_WIDGET ("cc_toggle")));
	gnome_config_set_bool ("/bug-buddy/last/cc", b);
			       
	gnome_config_sync ();
}

static gboolean
load_entry (const char *name, const char *name2,
	    const char *path, const char *def)
{
	GtkWidget *w;
	gboolean is_default;
	gchar *s;

	w = GET_WIDGET (name);
	s = gnome_config_get_string_with_default (path, &is_default);
	if (s)
		gtk_entry_set_text (GTK_ENTRY (w), s);
	else if (def)
		gtk_entry_set_text (GTK_ENTRY (w), def);
	g_free (s);
	w = GET_WIDGET (name2);
	if (GNOME_IS_FILE_ENTRY (w))
		w = gnome_file_entry_gnome_entry (GNOME_FILE_ENTRY (w));
	gnome_entry_load_history (GNOME_ENTRY (w));
	return is_default;
}

static void
load_config ()
{
	GtkWidget *w;
	char *sendmail;
	gboolean b;

	load_entry ("name_entry", "name_entry2",
		    "/bug-buddy/last/name", g_get_real_name ());

	druid_data.default_email = 
		load_entry ("email_entry", "email_entry2",
			    "/bug-buddy/last/email", g_get_user_name ());

	load_entry ("file_entry", "file_entry2",
		    "/bug-buddy/last/bugfile", NULL);

	w = gnome_file_entry_gnome_entry (
		GNOME_FILE_ENTRY (GET_WIDGET ("include_entry2")));
	gnome_entry_load_history (GNOME_ENTRY (w));

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

	b = gnome_config_get_bool ("/bug-buddy/last/skip_conf=0");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (SKIP_CONF), b);
	druid_data.submit_type = 
		gnome_config_get_int ("/bug-buddy/last/submittype");

	gtk_toggle_button_set_active (
		GTK_TOGGLE_BUTTON (GET_WIDGET ("cc_toggle")),
		gnome_config_get_bool ("/bug-buddy/last/cc=0"));
}

static gboolean
update_crash_type (GtkWidget *w, gpointer data)
{
	CrashType new_type = GPOINTER_TO_INT (data);
	GtkWidget *segv, *core;

	druid_data.crash_type = new_type;

	segv = GET_WIDGET ("segv_table");
	core = GET_WIDGET ("core_box");

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
	SubmitType new_type = GPOINTER_TO_INT (data);
	GtkWidget *box, *toggle, *entry;

	druid_data.submit_type = new_type;

	box    = GET_WIDGET ("submit_to_box");
	toggle = GET_WIDGET ("cc_toggle");
	entry  = GET_WIDGET ("file_entry2");

	gtk_widget_hide (box);
	gtk_widget_hide (toggle);
	gtk_widget_hide (entry);

	switch (new_type) {
	case SUBMIT_REPORT:
		gtk_widget_show (box);
		gtk_widget_show (toggle);
		break;
	case SUBMIT_FILE:
		gtk_widget_show (entry);
		break;
	default:
		break;
	}
	return FALSE;
}

gboolean
on_front_page_next (GtkWidget *page, GnomeDruid *druid)
{
	if (druid_data.default_email)
		return FALSE;

	gnome_druid_set_page (druid, GNOME_DRUID_PAGE (DEBIAN_PAGE));
	return TRUE;
}

gboolean
on_nature_page_next (GtkWidget *page, GnomeDruid *druid)
{
	if (druid_data.crash_type != CRASH_NONE)
		return FALSE;

	gnome_druid_set_page (druid, GNOME_DRUID_PAGE (VERSION_PAGE));
	return TRUE;
}

gboolean
on_debian_page_next (GtkWidget *page, GnomeDruid *druid)
{
	GtkWidget *w;
	char *s, *email;
	int bugnum;

	s = gtk_entry_get_text (GTK_ENTRY (CTREE_COMBO (GET_WIDGET ("miggie_combo"))->entry));
	if (!strcmp ("general", s)) {
		w = gnome_message_box_new (_("It is much more helpful if you specify\n"
					     "a more specific package than 'general'.\n\n"
					     "Do you want to continue anyway?"),
					   GNOME_MESSAGE_BOX_QUESTION,
					   GNOME_STOCK_BUTTON_YES,
					   GNOME_STOCK_BUTTON_NO,
					   NULL);
		if (GNOME_NO == gnome_dialog_run_and_close (w))
			return TRUE;		
	}

	email = druid_data.bts->get_email ();
	if (email) {
		if (druid_data.bug_type == BUG_NEW) {
			s = g_strconcat ("submit", email, NULL);
		} else {	
			w = GET_WIDGET ("bug_number");
			bugnum = gtk_spin_button_get_value_as_int (
				GTK_SPIN_BUTTON (w));
			s = g_strdup_printf ("%d%s", bugnum, email);
		}
	} else {
		s = g_strdup ("");
	}
	
	w = GET_WIDGET ("submit_to_entry");
	gtk_editable_delete_text (GTK_EDITABLE (w), 0, -1);
	gtk_editable_insert_text (GTK_EDITABLE (w), s, strlen (s), &bugnum);
	g_free (s);

	return FALSE;
}

gboolean
on_desc_page_next (GtkWidget *page, GnomeDruid *druid)
{
	char *s = gtk_editable_get_chars (GTK_EDITABLE (GET_WIDGET ("include_entry")),
					  0, -1);
	if (s && strlen (s) > 0) {
		char *mime_type;
		if (!g_file_exists (s)) {
			g_free (s);
			gnome_dialog_run_and_close (
				GNOME_DIALOG (gnome_error_dialog (_("The specified file does not exist."))));
			return TRUE;
		}

		mime_type = gnome_mime_type_of_file (s);
		g_message (_("File is of type: %s"), mime_type);

		if (!mime_type || strncmp ("text/", mime_type, 5)) {
			char *msg = g_strdup_printf (_("'%s' does not look like a text file."), s);
			gnome_dialog_run_and_close (
				GNOME_DIALOG (gnome_error_dialog (msg)));
			g_free (msg);
			g_free (s);
			return TRUE;
		}
	}
	g_free (s);

	if (GTK_ENTRY (GET_WIDGET ("desc_entry"))->text_length &&
	    gtk_text_get_length (GTK_TEXT (GET_WIDGET ("desc_area"))))
		return FALSE;
	gnome_dialog_run_and_close (
		GNOME_DIALOG (gnome_error_dialog (_("You must include a subject and a description\n"
						    "in your bug report."))));
	return TRUE;
}

gboolean
on_more_page_next (GnomeDruidPage *page, GnomeDruid *druid)
{
	if (druid_data.severity != SEVERITY_WISHLIST)
		return FALSE;

	gnome_druid_set_page (druid, GNOME_DRUID_PAGE (ACTION_PAGE));
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

gboolean
on_contact_page_prepare (GnomeDruidPage *page, GnomeDruid *druid)
{
	gnome_druid_set_buttons_sensitive (druid, FALSE, TRUE, TRUE);	
	return FALSE;
}

static void
on_stop_button_clicked (GtkWidget *button, gpointer data)
{
	GtkWidget *w;
	if (!druid_data.fd)
		return;

	w = gnome_question_dialog (_("gdb has not finished getting the "
				     "debugging information.\n"
				     "Kill the gdb process (the stack trace "
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
on_version_page_back (GtkWidget *page, GnomeDruid *druid)
{
	GnomeDruidPage *newpage;
	if (druid_data.crash_type != CRASH_NONE &&
	    druid_data.severity != SEVERITY_WISHLIST)
		return FALSE;

	if (druid_data.crash_type == SEVERITY_WISHLIST)
		newpage = GNOME_DRUID_PAGE (MORE_PAGE);
	else
		newpage = GNOME_DRUID_PAGE (NATURE_PAGE);

	gnome_druid_set_page (druid, newpage);
	return TRUE;
}

static void
on_about_button_clicked (GtkWidget *button, gpointer data)
{
	static GtkWidget *about, *href;
	static const char *authors[] = {
		"Jacob Berkman  <jacob@helixcode.com>",
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
				 BUDDY_ICONDIR"/bug-buddy.png");
	gtk_signal_connect (GTK_OBJECT (about), "destroy",
			    GTK_SIGNAL_FUNC (gtk_widget_destroyed),
			    &about);

	href = gnome_href_new ("http://bug-buddy.org/",
			       _("The lame bug-buddy web page"));
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (about)->vbox),
			    href, FALSE, FALSE, 0);

	gtk_widget_show (href);
	gtk_widget_show (about);
}

static void
on_help_button_clicked (GtkWidget *w, gpointer data)
{
	GnomeHelpMenuEntry help_entry = { "bug-buddy", "index.html"};
	gnome_help_display(NULL, &help_entry);
}

gboolean
on_the_druid_cancel (GtkWidget *w)
{
	save_config ();
	gtk_main_quit ();
	return FALSE;
}

static gboolean
on_timer (GtkProgress *progress)
{
	static int orient = 0;	
	gfloat val = gtk_progress_get_value (progress);
	if (val >= 100.0) {
		gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR (progress),
						  (orient+1)%2);
		val = -5.0;
	}
	gtk_progress_set_value (progress, val+5.0);
	return TRUE;
}

gboolean
on_version_page_prepare (GnomeDruidPage *page, GnomeDruid *druid)
{
	static char *last_file;
	GtkWidget *w;

	gtk_entry_set_text (GTK_ENTRY (VERSION_EDIT), "");

	if (last_file && !strcmp (last_file, druid_data.bts_file))
		return FALSE;

	g_free (last_file);
	last_file = g_strdup (druid_data.bts_file);

	w = GET_WIDGET ("config_progress");
	gtk_widget_show (w);
	gtk_progress_bar_set_activity_step (GTK_PROGRESS_BAR (w), 5.0);
	gtk_progress_set_activity_mode (GTK_PROGRESS (w), TRUE);

	druid_data.progress_timeout = gtk_timeout_add (150, 
						       (GtkFunction) on_timer,
						       (gpointer) w);
	update_das_clist ();
	/* load_bts_xml (); */

	return FALSE;
}

gboolean
on_contact_page_next (GtkWidget *page, GtkWidget *druid)
{
	GtkWidget *w;
	gchar *s;

	w = GET_WIDGET ("name_entry");
	s = gtk_entry_get_text (GTK_ENTRY (w));
	if (!s || strlen(s) == 0)
		goto contact_failed;
       
	w = GET_WIDGET ("email_entry");
	s = gtk_entry_get_text (GTK_ENTRY (w));
	if (!s || strlen(s) == 0)
		goto contact_failed;

	w = GET_WIDGET ("mailer_entry");
	s = gtk_entry_get_text (GTK_ENTRY (w));
	if (!s || strlen (s) == 0 || !g_file_exists (s)) {
		if (druid_data.submit_type == SUBMIT_TO_SELF ||
		    druid_data.submit_type == SUBMIT_REPORT) {
			GtkWidget *d;
			char *m;
			m = g_strdup_printf (_("'%s' doesn't seem to exist.  "
					       "You won't be able to actually\n"
					       "submit a bug report, but you will "
					       "be able to save it to a file.\n\n"
					       "Specify a new location for sendmail?"),
					     s);	
			d = gnome_question_dialog (m, NULL, NULL);
			g_free (m);
			if (GNOME_YES == gnome_dialog_run_and_close (GNOME_DIALOG (d)))
				return TRUE;
		}
#if 0			    
		w = GET_WIDGET ("submit_radio");
		gtk_widget_hide (w);
		w = GET_WIDGET ("email_radio");
		gtk_widget_hide (w);		
		w = GET_WIDGET ("file_radio");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
					      TRUE);
	} else {
		w = GET_WIDGET ("submit_radio");
		gtk_widget_show (w);
		w = GET_WIDGET ("email_radio");
		gtk_widget_show (w);
#endif
	}
#if 0
	if (load_bts_xml ())
		return TRUE;
#endif
	gnome_druid_set_buttons_sensitive (GNOME_DRUID (druid), 
					   TRUE, TRUE, TRUE);

	return FALSE;

 contact_failed:
	gnome_error_dialog (_("Please enter your name and email address"));
	return TRUE;
}

gboolean
on_complete_page_prepare (GtkWidget *page, GtkWidget *druid)
{
	gchar *to, *s, *s2, *file, *command;
	GtkWidget *w;
	FILE *fp;

	w = GET_WIDGET ( (druid_data.submit_type == SUBMIT_TO_SELF)
			 ? "email_entry" : "submit_to_entry");
	to = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);

	if (druid_data.submit_type == SUBMIT_FILE) {
		file = gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("file_entry")));
		fp = fopen (file, "w");
		if (!fp) {
			s = g_strdup_printf (_("Unable to open file: '%s'"), file);
			w = gnome_error_dialog (s);
			g_free (s);
			g_free (to);
			gnome_dialog_run_and_close (GNOME_DIALOG (w));
			gnome_druid_set_page (GNOME_DRUID (druid), ACTION_PAGE);
			return TRUE;
		}
	} else {
		w = GET_WIDGET ("mailer_entry");
		s = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);
		command = g_strdup_printf ("%s -i -t", s);

		g_message (_("about to run '%s'"), command);
		fp =  popen (command, "w");
		g_free (command);
		if (!fp) {
			s2 = g_strdup_printf (_("Unable to start mail program: '%s'"), s);
			w = gnome_error_dialog (s2);
			g_free (s);
			g_free (s2);
			gnome_dialog_run_and_close (GNOME_DIALOG (w));
			gnome_druid_set_page (GNOME_DRUID (druid), ACTION_PAGE);
			return TRUE;
		}
		g_free (s);
	}

	fprintf (fp, "To: %s\n", to);

	if (druid_data.submit_type == SUBMIT_REPORT &&
	    GTK_TOGGLE_BUTTON (GET_WIDGET ("cc_toggle"))->active) {
		w = GET_WIDGET ("email_entry");
		s = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);
		fprintf (fp, "Cc: %s\n", s);
		g_free (s);
	}
	
	w = GET_WIDGET ("content_text");
	s = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);
	fprintf (fp, "%s", s);
	g_free (s);

	if (druid_data.submit_type == SUBMIT_FILE) {
		fclose (fp);
		s = g_strdup_printf (_("Your bug report was saved in '%s'"), file);
	} else {
		pclose (fp);
		s = g_strdup_printf (_("Your bug report has been submitted to:\n\n        <%s>"), to);
	}
	g_free (to);

	gnome_druid_page_finish_set_text (GNOME_DRUID_PAGE_FINISH (page), s);
	g_free (s);

	return FALSE;
}


gboolean
on_complete_page_finish (GtkWidget *page, GtkWidget *druid)
{
#if 0
	if (druid_data.bts->doit ())
		return TRUE;
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
		gtk_entry_set_text (GTK_ENTRY (VERSION_EDIT), s);
	else
		gtk_editable_delete_text (GTK_EDITABLE (VERSION_EDIT),
					  0, -1);
	gtk_clist_get_text (list, row, 0, &s);
	gtk_label_set_text (GTK_LABEL (VERSION_LABEL), _(s));
}

static void
add_to_clist (gpointer data, gpointer udata)
{
	char *row[3] = { NULL };
	Package *package = data;
	GtkWidget *clist = udata;

	row[0] = _(package->name);
	row[1] = package->version;

	gtk_clist_append (GTK_CLIST (clist), row);
}

void
stop_progress ()
{
	if (!druid_data.progress_timeout)
		return;

	gtk_timeout_remove (druid_data.progress_timeout);
	gtk_widget_hide (GET_WIDGET ("config_progress"));
}

void
append_packages ()
{
	GtkWidget *w;
	w = VERSION_LIST;

	gtk_clist_freeze (GTK_CLIST (w));
	g_slist_foreach (druid_data.packages, add_to_clist, w);
	gtk_clist_thaw (GTK_CLIST (w));

	stop_progress ();

	w = GET_WIDGET ("skip_conf");
	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
		return;

	gnome_druid_set_page (GNOME_DRUID (druid_data.the_druid),
			      GNOME_DRUID_PAGE (ACTION_PAGE));
}
void
update_selected_row (GtkWidget *w, gpointer data)
{
	gchar *s;
	gint row;
	if (druid_data.selected_row == -1)
		return;
	row = druid_data.selected_row;
	s = gtk_entry_get_text (GTK_ENTRY (VERSION_EDIT));
	gtk_clist_set_text (GTK_CLIST (VERSION_LIST), row, 1, s);
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
on_action_page_prepare (GtkWidget *page, GnomeDruid *druid)
{
	GtkWidget *w = GET_WIDGET ("content_text");
	gtk_text_freeze (GTK_TEXT (w));
	gtk_editable_delete_text (GTK_EDITABLE (w), 0, -1);
	druid_data.bts->doit (GTK_EDITABLE (w));
	gtk_text_thaw (GTK_TEXT (w));
	return FALSE;
}

gboolean
on_action_page_next (GtkWidget *page, GtkWidget *druid)
{
	GtkWidget *entry, *d, *w;
	char *s, *title, *to;

	if (druid_data.submit_type != SUBMIT_FILE) {
		w = GET_WIDGET ( (druid_data.submit_type == SUBMIT_TO_SELF)
				 ? "email_entry" : "submit_to_entry");
		to = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);

		if (!strlen (to)) {
			g_free (to);
			gnome_dialog_run_and_close (
				GNOME_DIALOG (gnome_error_dialog (
					_("You must specify a recipient email address"))));
			return TRUE;
		}
		g_free (to);

		return GNOME_NO == gnome_dialog_run_and_close (
			gnome_question_dialog (_("Submit this bug report now?"),
					       NULL, NULL));
	}

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

static void
update_bug_type (GtkWidget *w, gpointer data)
{
	BugType type = GPOINTER_TO_INT (data);

	if (type == BUG_NEW) {
		gtk_widget_hide (GET_WIDGET ("existing_box"));
		gtk_widget_show (GET_WIDGET ("newbug_table"));
	} else {
		gtk_widget_hide (GET_WIDGET ("newbug_table"));
		gtk_widget_show (GET_WIDGET ("existing_box"));
	}

	druid_data.bug_type = type;
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

static gboolean
set_project (GtkWidget *w, gpointer data)
{
	druid_data.bts_file = data;
	load_bts_xml ();
	return FALSE;
}

GtkWidget *
make_pixmap_button (gchar *widget_name, gchar *text, 
		    gchar *s2, int type, int i2)
{
	char *this_is_an_ugly_hack_for_kmaraas_and_the_rest_of_the_gnome_i18n_team =
	{ N_("About"), N_("Help"), N_("Stop"), N_("Refresh") };
	GtkWidget *p, *w;

	/* this is slightly bad but should be ok */
	p = gnome_stock_pixmap_widget (NULL, text);
	w = gnome_pixmap_button (p, _(text));
	GTK_WIDGET_SET_FLAGS (w, GTK_CAN_DEFAULT);
	gtk_container_set_border_width (GTK_CONTAINER (w), GNOME_PAD_SMALL);
	return w;
}

GtkWidget *
make_anim (gchar *widget_name, gchar *imgname, 
	   gchar *string2, gint size, gint freq)
{
	GtkWidget *w;
	gchar *pixmap;

	w = gnome_animator_new_with_size (48, 48);

	gnome_animator_set_loop_type (GNOME_ANIMATOR (w),
				      GNOME_ANIMATOR_LOOP_RESTART);
	pixmap = g_strconcat (BUDDY_DATADIR, imgname, NULL);
	gnome_animator_append_frames_from_file (GNOME_ANIMATOR (w),
						pixmap, 0, 0, freq, size);
	g_free (pixmap);
	return w;
}

static void 
init_toggle (const char *name, int test, int data, GtkSignalFunc func)
{
	GtkWidget *w;
	w = GET_WIDGET (name);
	gtk_signal_connect (GTK_OBJECT (w), "toggled", func,
			    GINT_TO_POINTER (data));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), test == data);
}

static void
init_bts_menu (void)
{
	GtkWidget *w, *m;
	DIR *dir;
	gchar *s = NULL, *p, *chr;
	struct dirent *dent;
	int idx=0, def=0;

	dir = opendir (BUDDY_DATADIR "/xml/");
	if (!dir) {
		s = g_strdup_printf (_("Could not open '%s'.\n"
				       "Please make sure bug-buddy was "
				       "installed correctly."),
				     BUDDY_DATADIR "/xml/");
		w = gnome_error_dialog (s);
		g_free (s);
		gnome_dialog_run_and_close (GNOME_DIALOG (w));
		exit (0);
	}

	m = gtk_menu_new ();
	while ((dent = readdir (dir))) {
		if (dent->d_name[0] == '.')
			continue;

		p = strrchr (dent->d_name, '.');
		if (!p || strcmp (p, ".bts")) 
			continue;
		
		/* memleak */
		s = g_strdup (dent->d_name);
		*p = '\0';
		if (!druid_data.bts_file)
			druid_data.bts_file = s;

                /* convert '_' -> ' ' */
                while ( (chr = strchr (dent->d_name, '_')) )
                        *chr = ' ';

		w = gtk_menu_item_new_with_label (dent->d_name);
		gtk_signal_connect (GTK_OBJECT (w), "activate",
				    GTK_SIGNAL_FUNC (set_project), s);
				    
		gtk_widget_show (w);
		gtk_menu_append (GTK_MENU (m), w);
		if (!strcmp (s, DEFAULT_BTS)) {
			druid_data.bts_file = s;
			def = idx;
		}
		idx++;
	}
	closedir (dir);
       
	w = GET_WIDGET ("bts_menu");
	gtk_option_menu_set_menu (GTK_OPTION_MENU (w), m);
	gtk_option_menu_set_history (GTK_OPTION_MENU (w), def);

	set_project (NULL, DEFAULT_BTS);
	/* evil hack */
	/*if (d) GTK_OPTION_MENU (w)->menu_item = d;*/
}

/* there should be no setting of default values here, I think */
static void
init_ui ()
{
	GtkWidget *w, *m;
	gchar *s;
	int i;

	glade_xml_signal_autoconnect (druid_data.xml);

	w = GET_WIDGET ("the_druid");
	druid_data.the_druid = w;

	load_config ();

	gtk_button_set_relief (GET_WIDGET ("already_href"),
			       GTK_RELIEF_NONE);

	m = gtk_menu_new ();
	for (i = 0; severity[i]; i++) {
		w = gtk_menu_item_new_with_label (_(severity[i]));
		gtk_signal_connect (GTK_OBJECT (w), "activate",
				    GTK_SIGNAL_FUNC (set_severity),
				    GINT_TO_POINTER (i));
		gtk_widget_show (w);
		gtk_menu_append (GTK_MENU (m), w);
	}

	w = GET_WIDGET ("severity_option");
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
	set_bug_class (NULL, GINT_TO_POINTER (druid_data.bug_class));

	w = GET_WIDGET ("class_option");
	gtk_option_menu_set_menu (GTK_OPTION_MENU (w), m);

	/* dialog crash page */
	w = APP_FILE;
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

	w = CRASHED_PID;
	s = popt_data.pid;
	if (!s) {
		s = getenv ("GNOME_CRASHED_PID");
		if (s)
			g_warning (_("$GNOME_CRASHED_PID is deprecated.\n"
				     "Please use the --pid command line"
				     "argument instead."));
	}

	if (s) {
		gtk_entry_set_text (GTK_ENTRY (w), s);
		druid_data.crash_type = CRASH_DIALOG;
	}

	/* core crash page */
	w = CORE_FILE;
	if (popt_data.core_file) {
		gtk_entry_set_text (GTK_ENTRY (w), popt_data.core_file);
		druid_data.crash_type = CRASH_CORE;
	}

	/* package version */
	if (popt_data.package_ver) {
		w = GET_WIDGET ("version_entry");
		gtk_entry_set_text (GTK_ENTRY (w), popt_data.package_ver);
	}

        /* init some ex-radio buttons */
	m = gtk_menu_new ();
	for (i = 0; bug_type[i]; i++) {
		w = gtk_menu_item_new_with_label (_(bug_type[i]));
		gtk_signal_connect (GTK_OBJECT (w), "activate",
				    GTK_SIGNAL_FUNC (update_bug_type),
				    GINT_TO_POINTER (i));
		gtk_widget_show (w);
		gtk_menu_append (GTK_MENU (m), w);
	}
	w = GET_WIDGET ("bug_type_option");
	gtk_option_menu_set_menu (GTK_OPTION_MENU (w), m);
	gtk_option_menu_set_history (GTK_OPTION_MENU (w), druid_data.bug_type);
	update_bug_type (NULL, GINT_TO_POINTER (druid_data.bug_type));

        /* init some ex-radio buttons */
	m = gtk_menu_new ();
	for (i = 0; crash_type[i]; i++) {
		w = gtk_menu_item_new_with_label (_(crash_type[i]));
		gtk_signal_connect (GTK_OBJECT (w), "activate",
				    GTK_SIGNAL_FUNC (update_crash_type),
				    GINT_TO_POINTER (i));
		gtk_widget_show (w);
		gtk_menu_append (GTK_MENU (m), w);
	}
	w = GET_WIDGET ("crash_type_option");
	gtk_option_menu_set_menu (GTK_OPTION_MENU (w), m);
	gtk_option_menu_set_history (GTK_OPTION_MENU (w), druid_data.crash_type);
	update_crash_type (NULL, GINT_TO_POINTER (druid_data.crash_type));

	/* init more ex-radio buttons */
	m = gtk_menu_new ();
	for (i = 0; submit_type[i]; i++) {
		w = gtk_menu_item_new_with_label (_(submit_type[i]));
		gtk_signal_connect (GTK_OBJECT (w), "activate",
				    GTK_SIGNAL_FUNC (update_submit_type),
				    GINT_TO_POINTER (i));
		gtk_widget_show (w);
		gtk_menu_append (GTK_MENU (m), w);
	}
	w = GET_WIDGET ("action_option");
	gtk_option_menu_set_menu (GTK_OPTION_MENU (w), m);
	gtk_option_menu_set_history (GTK_OPTION_MENU (w), druid_data.submit_type);
	update_submit_type (NULL, GINT_TO_POINTER (druid_data.submit_type));

	gtk_signal_connect (GTK_OBJECT (GET_WIDGET ("about_button")),
			    "clicked",
			    GTK_SIGNAL_FUNC (on_about_button_clicked),
			    NULL);

	gtk_signal_connect (GTK_OBJECT (GET_WIDGET ("help_button")),
			    "clicked",
			    GTK_SIGNAL_FUNC (on_help_button_clicked),
			    NULL);

	/* less page */
	w = STOP_BUTTON;
	gtk_signal_connect (GTK_OBJECT (w), "clicked",
			    GTK_SIGNAL_FUNC (on_stop_button_clicked),
			    NULL);
	gtk_widget_set_sensitive (GTK_WIDGET (w), FALSE);

	gtk_signal_connect (GTK_OBJECT (REFRESH_BUTTON), "clicked",
			    GTK_SIGNAL_FUNC (on_refresh_button_clicked),
			    NULL);

	gnome_druid_set_page (GNOME_DRUID (druid_data.the_druid), 
			      GNOME_DRUID_PAGE (GET_WIDGET ("contact_page")));

	gnome_window_icon_set_from_default (GTK_WINDOW (GET_WIDGET ("druid_window")));
}

gint
delete_me (GtkWidget *w, GdkEventAny *evt, gpointer data)
{
	gtk_main_quit ();
	return FALSE;
}

int
main (int argc, char *argv[])
{
	GtkWidget *w;
	char *s;

	memset (&druid_data, 0, sizeof (druid_data));
	memset (&popt_data,  0, sizeof (popt_data));

	druid_data.crash_type = CRASH_NONE;

	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	gnome_init_with_popt_table (PACKAGE, VERSION, argc, argv, 
				    options, 0, NULL);
	gnome_window_icon_set_default_from_file (BUDDY_ICONDIR"/bug-buddy.png");

	glade_gnome_init ();

	s = "bug-buddy.glade";
	if (!g_file_exists (s))
		s = BUDDY_DATADIR "/bug-buddy.glade";

	gtk_widget_push_visual (gdk_imlib_get_visual ());
	gtk_widget_push_colormap (gdk_imlib_get_colormap ());

	druid_data.xml = glade_xml_new (s, "druid_window");

	if (!druid_data.xml) {
		s = g_strdup_printf (_("Could not find '%s'.\n"
				       "Please make sure bug-buddy was "
				       "installed correctly."),
				     BUDDY_DATADIR "/bug-buddy.glade");
		w = gnome_error_dialog (s);
		gnome_dialog_run_and_close (GNOME_DIALOG (w));
		return 0;
	}

	init_ui ();

	init_bts_menu ();

	gtk_widget_show (GET_WIDGET ("druid_window"));

	gtk_widget_pop_colormap ();
	gtk_widget_pop_visual ();

	gtk_main ();

	return 0;
}

