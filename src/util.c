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
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>


#include "util.h"

static gboolean
clean_hash (gpointer key, gpointer value, gpointer data)
{
	g_free (key);
	if (GPOINTER_TO_INT (data))
		g_free (value);
	return TRUE;
}

void
destroy_hash_table (GHashTable *table, gboolean free_data)
{
	g_hash_table_foreach_remove (table, clean_hash, 
				     GINT_TO_POINTER (free_data));
	g_hash_table_destroy (table);
}

pid_t
start_commandv (const char *args[], int *rfd)
{
	int fd[2];
	pid_t pid;
	GtkWidget *d;

	if (pipe (fd) == -1) {
		perror ("can't open pipe");
		d = gnome_error_dialog (_("Unable to open pipe"));
		gnome_dialog_run_and_close (GNOME_DIALOG (d));
		return -1;
	}

	pid = fork ();
	if (pid == 0) {
		close (1);
		close (fd[0]);
		dup (fd[1]);

		execvp (args[0], args);
		g_warning (_("Could not run '%s'."), args[0]);
		_exit (1);
	} else if (pid == -1) {
		d = gnome_error_dialog (_("Error on fork()."));
		gnome_dialog_run_and_close (GNOME_DIALOG (d));
		close (fd[0]);
		close (fd[1]);
		return -1;
	}

	close (fd[1]);
	*rfd = fd[0];

	return pid;
}

pid_t
start_command (const char *command, int *fd)
{
	const char *args[] = { "sh", "-c", NULL, NULL };
	args[2] = command;
	return start_commandv (args, fd);
}

char *
get_line_from_fd (int fd)
{
	char buf[1024];
	int pos;

	buf[0] = '\0';
	for (pos = 0; pos < 1023; pos++)
		if (read (fd, buf+pos, 1) < 1 ||
		    buf[pos] == '\n')
			break;
	
	if (pos == 0 && buf[0] == '\0')
		return NULL;

	buf[pos] = '\0';
	return g_strdup (buf);
}

char *
get_line_from_commandv (const char *argv[])
{
	char *retval;
	int fd, status;
	pid_t pid;

	pid = start_commandv (argv, &fd);
	retval = get_line_from_fd (fd);
	
	close (fd);
	kill (pid, SIGTERM);
	waitpid (pid, &status, 0);

	return retval;
}

char *
get_line_from_command (const char *command)
{
	const char *args[] = { "sh", "-c", NULL, NULL };
	args[2] = command;
	return get_line_from_commandv (args);
}

char *
get_line_from_file (const char *filename)
{
	char *retval;
	int fd;

	g_return_val_if_fail (filename, NULL);

	if (!g_file_exists (filename))
		return NULL;

	fd = open (filename, O_RDONLY);
	if (fd == -1) {
		g_warning ("Could not open file '%s' for reading", filename);
		return NULL;
	}
	
	retval = get_line_from_fd (fd);
	close (fd);
	return retval;
}


