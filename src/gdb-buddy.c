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

#include <config.h>
#include <gnome.h>

#include "bug-buddy.h"

void
start_gdb ()
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
		if (druid_data.explicit_dirty ||
		    (old_type != CRASH_DIALOG) ||
		    (!old_app || strcmp (app, old_app)) ||
		    (!old_extra || strcmp (extra, old_extra))) {
			get_trace_from_pair (app, extra);
		}
		break;
	case CRASH_CORE:
		extra = gtk_entry_get_text (GTK_ENTRY (druid_data.core_file));
		if (druid_data.explicit_dirty ||
		    (old_type != CRASH_CORE) ||
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
}

void
stop_gdb ()
{
	int status;
	if (!druid_data.fp) {
		g_message (_("gdb has exited"));
		return;
	}

	status = pclose (druid_data.fp);
	g_message (_("Subprocess exited with status %d"), status);
	gdk_input_remove (druid_data.input);
	gnome_druid_set_buttons_sensitive (GNOME_DRUID (druid_data.the_druid),
					   TRUE, TRUE, TRUE);
	gnome_animator_stop (GNOME_ANIMATOR (druid_data.gdb_anim));
	druid_data.fp = NULL;
	druid_data.input = 0;
	gtk_widget_set_sensitive (GTK_WIDGET (druid_data.stop_button), FALSE);
	gtk_widget_set_sensitive (GTK_WIDGET (druid_data.refresh_button),
				  TRUE);
	return;
}

void 
get_trace_from_core (const gchar *core_file)
{
	gchar *gdb_cmd;
	gchar buf[1024];
	gchar *binary = NULL;
	int status;
	FILE *f;

	gdb_cmd = g_strdup_printf ("gdb --batch --core=%s", core_file);

	f = popen (gdb_cmd, "r");
	g_free (gdb_cmd);

	if (!f) {
		gchar *s = g_strdup_printf (_("Unable to process core file with gdb:\n"
					      "'%s'"), core_file);
		GtkWidget *d = gnome_error_dialog (s);
		g_free (s);
		gnome_dialog_run_and_close (GNOME_DIALOG (d));
		return;
	}

	while (fgets(buf, 1024, f) != NULL) {
		if (!binary && !strncmp(buf, "Core was generated", 16)) {
			gchar *s;
			gchar *ptr = buf;
			while (*ptr != '`' && *ptr !='\0') ptr++;
			if (*ptr == '`') {
				ptr++;
				s = ptr;
				while (*ptr != '\'' && *ptr !=' ' && *ptr !='\0') ptr++;
				*ptr = '\0';
				binary = g_strdup(s);
			}
		}
	}

	status = pclose(f);
	g_message (_("Child process exited with status %d\n"), status);
	if (!binary) {
		gchar *s = g_strdup_printf (_("Unable to determine which binary created\n"
					      "'%s'"), core_file);
		GtkWidget *d = gnome_error_dialog (s);
		g_free (s);
		gnome_dialog_run_and_close (GNOME_DIALOG (d));
		return;
	}	

	get_trace_from_pair (binary, core_file);
	g_free (binary);
}

static void
handle_gdb_input (gpointer data, int source, GdkInputCondition cond)
{	
	char buf[1024];
	FILE *fp = druid_data.fp;

	if (!fp) {
		g_warning (_("fp is NULL, not reading input"));
		return;
	}

	if (feof (fp) || !fgets (buf, 1024, fp)) {
		stop_gdb ();
		return;
	}

	gtk_text_set_point (GTK_TEXT (druid_data.gdb_text),
			    gtk_text_get_length (GTK_TEXT (druid_data.gdb_text)));
	gtk_text_insert (GTK_TEXT (druid_data.gdb_text), 
			 NULL, NULL, NULL, buf, strlen (buf));

	return;
}

void
get_trace_from_pair (const gchar *app, const gchar *extra)
{
	gchar *cmd_buf;
	gchar *cmd_file;

	if (!app || !extra || !app[0] || !extra[0])
		return;

	cmd_file = BUDDY_DATADIR "/gdb-cmd";
	if (!cmd_file) {
	        GtkWidget *d;
		d = gnome_error_dialog (_("Could not find the gdb-cmd file.\n"
					  "Please try reinstalling bug-buddy."));
		gnome_dialog_run_and_close (GNOME_DIALOG (d));
		return;
	}

	cmd_buf = g_strdup_printf ("gdb --batch --quiet --command=%s %s %s",
				   cmd_file, app, extra);

	g_message ("about to run: %s", cmd_buf);
	druid_data.fp = popen (cmd_buf, "r");
	g_free (cmd_buf);

	if (!druid_data.fp) {
		gchar *s = g_strdup_printf (_("Unable to start '%s'.\n"), cmd_buf);
		GtkWidget *d = gnome_error_dialog (s);
		g_free (s);
		gnome_dialog_run_and_close (GNOME_DIALOG (d));
		return;
	}

	gtk_editable_delete_text (GTK_EDITABLE (druid_data.gdb_text), 0, -1);
	gnome_druid_set_buttons_sensitive (GNOME_DRUID (druid_data.the_druid),
					   FALSE, FALSE, TRUE);
	gnome_animator_start (GNOME_ANIMATOR (druid_data.gdb_anim));
	druid_data.input = gdk_input_add (fileno (druid_data.fp), GDK_INPUT_READ,
					  handle_gdb_input, druid_data.fp);
	gtk_widget_set_sensitive (GTK_WIDGET (druid_data.stop_button), TRUE);
	gtk_widget_set_sensitive (GTK_WIDGET (druid_data.refresh_button),
				  FALSE);

	druid_data.explicit_dirty = FALSE;
}






