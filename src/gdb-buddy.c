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

#include <config.h>
#include <gnome.h>

#include "gdb-buddy.h"

void 
get_trace_from_core (GnomeDruid *druid, GtkText *text, gchar *core_file)
{
	gchar *gdb_cmd;
	gchar buf[1024];
	gchar *binary = NULL;
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

	fclose(f);
	if (!binary) {
		gchar *s = g_strdup_printf (_("Unable to determine which binary created\n"
					      "'%s'"), core_file);
		GtkWidget *d = gnome_error_dialog (s);
		g_free (s);
		gnome_dialog_run_and_close (GNOME_DIALOG (d));
		return;
	}	

	get_trace_from_pair (druid, text, binary, core_file);
	g_free (binary);
}

typedef struct {
	FILE *fp;
	GtkText *text;
	GnomeDruid *druid;
	int input;
} GdbData;
GdbData gdb_data;

static void
handle_gdb_input (gpointer data, int source, GdkInputCondition cond)
{	
	char buf[1024];
	GdbData *gdb_data = data;

	if (feof (gdb_data->fp)) {
		gdk_input_remove (gdb_data->input);
		gnome_druid_set_buttons_sensitive (gdb_data->druid, 
						   TRUE, TRUE, TRUE);
		return;
	}

	fgets (buf, 1024, gdb_data->fp);

	gtk_text_set_point (gdb_data->text, gtk_text_get_length(gdb_data->text));
	gtk_text_insert (gdb_data->text, NULL, NULL, NULL, buf, strlen(buf)); 

	return;
}

void
get_trace_from_pair (GnomeDruid *druid, GtkText *text,
		     const gchar *app, const gchar *extra)
{
	gchar *cmd_buf;
	gchar *cmd_file;

	if (!app || !extra || !app[0] || !extra[0])
		return;

	cmd_file = gnome_datadir_file ("bug-buddy/gdb-cmd");
	if (!cmd_file) {
	        GtkWidget *d = gnome_error_dialog (_("Could not find the gdb-cmd file.\n"
						     "Please try reinstalling bug-buddy."));
		gnome_dialog_run_and_close (GNOME_DIALOG (d));
		return;
	}

	cmd_buf = g_strdup_printf ("gdb --batch --quiet --command=%s %s %s",
				   cmd_file, app, extra);
	g_free (cmd_file);
	
	g_message ("about to run: %s", cmd_buf);
	gdb_data.fp = popen (cmd_buf, "r");
	g_free (cmd_buf);

	if (!gdb_data.fp) {
		gchar *s = g_strdup_printf (_("Unable to start '%s'.\n"), cmd_buf);
		GtkWidget *d = gnome_error_dialog (s);
		g_free (s);
		gnome_dialog_run_and_close (GNOME_DIALOG (d));
		return;
	}

	gdb_data.druid = druid;
	gdb_data.text = text;
	gdb_data.input = gdk_input_add (fileno (gdb_data.fp),
					GDK_INPUT_READ,
					handle_gdb_input,
					&gdb_data);
}
