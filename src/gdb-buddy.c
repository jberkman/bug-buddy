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

#include <sys/wait.h>
#include <unistd.h>

#include "bug-buddy.h"
#include "util.h"
#include "glade-druid.h"

void
start_gdb ()
{
	static gchar *old_app = NULL;
	static gchar *old_extra = NULL;
	static CrashType old_type = -1;
	
	gchar *app = NULL, *extra = NULL;

	g_message (_("obtaining stack trace... (%d)"), druid_data.crash_type);
	
	switch (druid_data.crash_type) {
	case CRASH_DIALOG:
		app = gtk_entry_get_text (GTK_ENTRY (APP_FILE));
		extra = gtk_entry_get_text (GTK_ENTRY (CRASHED_PID));
		druid_data.app_pid = atoi (extra);
		if (druid_data.explicit_dirty ||
		    (old_type != CRASH_DIALOG) ||
		    (!old_app || strcmp (app, old_app)) ||
		    (!old_extra || strcmp (extra, old_extra))) {
			get_trace_from_pair (app, extra);
		}
		break;
	case CRASH_CORE:
		extra = gtk_entry_get_text (GTK_ENTRY (CORE_FILE));
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
	if (!druid_data.ioc) {
		g_message (_("gdb has already exited"));
		return;
	}
	
	g_io_channel_close (druid_data.ioc);
	waitpid (druid_data.gdb_pid, NULL, 0);
	
	druid_data.gdb_pid = 0;
	gnome_druid_set_buttons_sensitive (GNOME_DRUID (druid_data.the_druid),
					   TRUE, TRUE, TRUE);
	gnome_animator_stop (GNOME_ANIMATOR (GDB_ANIM));
	druid_data.fd = 0;
	druid_data.ioc = NULL;
	gtk_widget_set_sensitive (GTK_WIDGET (STOP_BUTTON), FALSE);
	gtk_widget_set_sensitive (GTK_WIDGET (REFRESH_BUTTON),
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
#if 0
	g_message (_("Child process exited with status %d"), status);
#endif
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

static gboolean
handle_gdb_input (GIOChannel *ioc, GIOCondition condition, gpointer data)
{	
	GtkWidget *w;
	gchar buf[1024];
	guint len;
	
	if (condition == G_IO_HUP) {
		stop_gdb ();
		return FALSE;
	}

 gdb_try_read:
	switch (g_io_channel_read (ioc, buf, 1024, &len)) {
	case G_IO_ERROR_NONE:
		break;
	case G_IO_ERROR_AGAIN:
		goto gdb_try_read;
	default:
		g_warning (_("Error on read... aborting"));
		stop_gdb ();
		return FALSE;
	}

	w = GDB_TEXT;
	gtk_text_set_point (GTK_TEXT (w),
			    gtk_text_get_length (GTK_TEXT (w)));
	gtk_text_insert (GTK_TEXT (w), 
			 NULL, NULL, NULL, buf, len);

	return TRUE;
}

void
get_trace_from_pair (const gchar *app, const gchar *extra)
{
	GtkWidget *d;
	int fd;
	const char *args[] = { "gdb", 
			       "--batch", 
			       "--quiet",
			       "--command=" BUDDY_DATADIR "/gdb-cmd",
			       NULL, NULL, NULL };

	args[4] = app;
	args[5] = extra;

	if (!app || !extra || !app[0] || !extra[0])
		return;
	
	if (!g_file_exists (BUDDY_DATADIR "/gdb-cmd")) {
		d = gnome_error_dialog (_("Could not find the gdb-cmd file.\n"
					  "Please try reinstalling bug-buddy."));
		gnome_dialog_run_and_close (GNOME_DIALOG (d));
		return;
	}

	druid_data.gdb_pid = start_commandv (args, &fd);
	if (druid_data.gdb_pid == -1) {
		d = gnome_error_dialog (_("Error on fork()."));
		gnome_dialog_run_and_close (GNOME_DIALOG (d));
		return;
	}
	
	druid_data.fd = fd;
	druid_data.ioc = g_io_channel_unix_new (fd);
	g_io_add_watch (druid_data.ioc, G_IO_IN | G_IO_HUP, 
			handle_gdb_input, NULL);
	g_io_channel_unref (druid_data.ioc);
	gtk_editable_delete_text (GTK_EDITABLE (GDB_TEXT), 0, -1);
	gnome_druid_set_buttons_sensitive (GNOME_DRUID (THE_DRUID),
					   FALSE, FALSE, TRUE);
	gnome_animator_start (GNOME_ANIMATOR (GDB_ANIM));
	gtk_widget_set_sensitive (GTK_WIDGET (STOP_BUTTON), TRUE);
	gtk_widget_set_sensitive (GTK_WIDGET (REFRESH_BUTTON),
				  FALSE);

	druid_data.explicit_dirty = FALSE;
}






