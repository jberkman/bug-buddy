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

#include "distro-redhat.h"
#include "util.h"

static char *get_redhat_version (Distribution *distro);
static void get_package_versions (Package packages[]);

Phylum redhat_phy = { 
	get_redhat_version,
	get_package_versions 
};

static char *
get_redhat_version (Distribution *distro)
{
	g_return_val_if_fail (distro, NULL);
	g_return_val_if_fail (distro->version_file, NULL);

	return get_line_from_file (distro->version_file);
}

static void
get_package_versions (Package packages[])
{
	int i;
	const char *args[] = { "rpm", "-q", NULL, NULL };

	g_return_if_fail (packages);

	for (i = 0; packages[i].name; i++) {
		if (packages[i].version ||
		    !packages[i].rpm)
			continue;
		args[2] = packages[i].rpm;
		packages[i].version = get_line_from_commandv (args);
	}
}





