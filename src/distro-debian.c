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

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include <gnome.h>
#include "distro-debian.h"
#include "util.h"

static char *get_debian_version (Distribution *distro);
static void get_package_versions (Package packages[]);

Phylum debian_phy = { 
	get_debian_version,
	get_package_versions 
};

static char *
get_debian_version (Distribution *distro)
{
	char *retval, *version;

	g_return_val_if_fail (distro, NULL);
	g_return_val_if_fail (distro->version_file, NULL);
	g_return_val_if_fail (distro->name, NULL);

	version = get_line_from_file (distro->version_file);
	if (!version) {
		g_warning ("Could not get distro version");
		return NULL;
	}
	
	retval = g_strdup_printf ("%s %s", distro->name, version);
	g_free (version);

	return retval;
}

static void
get_package_versions (Package packages[])
{
	pid_t pid;
	int argc, fd, status, cur;
	char **argv, *command, *line;
	Package *package;
	GHashTable *table;
	GtkWidget *d;

	g_return_if_fail (packages);
	
	for (argc = cur = 0; packages[cur].name; cur++) {
		if (!packages[cur].version &&
		    packages[cur].deb)
			argc++;
	}

	if (argc == 0)
		return;
	
	argv = g_new (char *, argc+1);

	table = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_freeze (table);
		
	for (argc = cur = 0; packages[cur].name; cur++) {
		if (packages[cur].version ||
		    !packages[cur].deb)
			continue;
		argv[argc] = packages[cur].deb;
		g_hash_table_insert (table, 
				     g_strdup (packages[cur].deb),
				     &packages[cur]);
		argc++;
	}
	
	g_hash_table_thaw (table);
	argv[argc] = NULL;
	line = g_strjoinv (" ", argv);
	g_free (argv);
	command = g_strdup_printf ("dpkg -l %s | tail +6 | "
				   "awk '{ print $2\" \"$3 }'",
				   line);
	g_free (line);
	pid = start_command (command, &fd);
	g_free (command);
/* what we are looking at:

Desired=Unknown/Install/Remove/Purge
| Status=Not/Installed/Config-files/Unpacked/Failed-config/Half-installed
||/ Name            Version        Description
+++-===============-==============-============================================
ii  gnome-core      1.0.54-1.99.sl Common files for Gnome core apps

*/
	while ( (line = get_line_from_fd (fd)) ) {
		argv = g_strsplit (line, " ", 2);
		if (!argv[0] || !argv[1])
			goto end_while;
		package = g_hash_table_lookup (table, argv[0]);
		if (!package)
			goto end_while;
		package->version = g_strdup_printf ("%s %s", 
						    package->name, argv[1]);
	end_while:
		g_strfreev (argv);
	}

	g_hash_table_destroy (table);
	close (fd);
	kill (pid, SIGTERM);
	waitpid (pid, &status, 0);
}
