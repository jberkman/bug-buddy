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

#include <gnome-xml/tree.h>
#include <gnome-xml/parser.h>
#include <gnome-xml/xmlmemory.h>

#include <gnome.h>

#include "bug-buddy.h"
#include "util.h"
#include "distro.h"
#include "bts.h"
#include "glade-druid.h"

static Distribution distros[] = {
	{ "Slackware", "/etc/slackware-version", &debian_phy },
	{ "Debian",    "/etc/debian_version",    &debian_phy },
	{ "Red Hat",   "/etc/redhat-release",    &redhat_phy },
	{ "SuSE",      "/etc/SuSE-release",      &redhat_phy },
	{ "Mandrake",  "/etc/mandrake-release",  &redhat_phy },
	{ NULL }
};


static void
free_package_from_node (gpointer data, gpointer udata)
{
	Package *package = data;

	xmlFree (package->name);
	xmlFree (package->pre_command);
	xmlFree (package->rpm);
	xmlFree (package->deb);

	g_free (package->version);
	g_free (package);
}

static Package *
make_package_from_node (xmlNodePtr node)
{
	Package *package = g_new0 (Package, 1);

	package->name = xmlGetProp (node, "name");
	package->pre_command = xmlGetProp (node, "pre");
	package->rpm = xmlGetProp  (node, "rpm");
	package->deb = xmlGetProp (node, "deb");

	return package;
}

static void
get_version_from_pre (gpointer data, gpointer udata)
{
	Package *package = data;
	if (package->version || !package->pre_command)
		return;
	package->version = get_line_from_command (package->pre_command);
}

static void
update_das_clist ()
{
	GtkWidget *w;
	int i;
	char *row[3] = { NULL };

        /* system config page */

	w = VERSION_LIST;
	gtk_clist_clear (GTK_CLIST (w));

	for (i = 0; distros[i].name; i++) {
		if (!g_file_exists (distros[i].version_file))
			continue;
		row[0] = _("Distribution");
		row[1] = distros[i].phylum->version (&distros[i]);
		gtk_clist_append (GTK_CLIST (w), row);
		g_free (row[1]);
		druid_data.distro = &distros[i];
		break;
	}

	row[0] = N_("System");
	row[1] = get_line_from_command ("uname -rpms");
	gtk_clist_append (GTK_CLIST (w), row);
	g_free (row[1]);

	if (!druid_data.packages) {
		stop_progress ();
		return;
	}

	g_slist_foreach (druid_data.packages, get_version_from_pre, NULL);

	if (druid_data.distro)
		druid_data.distro->phylum->packager (druid_data.packages);
	else
		append_packages ();
#if 0
	g_slist_foreach (druid_data.packages, get_version_from_post, NULL);
	g_slist_foreach (druid_data.packages, add_to_clist, w);
#endif
}

gboolean
load_bts_xml ()
{
	static char *last_file;
	xmlDocPtr doc;
	xmlNodePtr cur, cur2;
	char *s, *file;

	g_return_val_if_fail (druid_data.bts_file, TRUE);
	if (last_file && !strcmp (last_file, druid_data.bts_file)) {
		stop_progress ();
		return FALSE;
	}
	
	if (druid_data.bts) {
		druid_data.bts->denit ();
		druid_data.bts = NULL;
	}
	
	if (druid_data.packages) {
		g_slist_foreach (druid_data.packages,
				 free_package_from_node,
				 NULL);
		g_slist_free (druid_data.packages);
		druid_data.packages = NULL;
	}

	g_free (last_file);
	last_file = g_strdup (druid_data.bts_file);

	file = g_strconcat (BUDDY_DATADIR "/xml/", druid_data.bts_file, NULL);
	doc = xmlParseFile (file);
	if (!doc || !doc->root || !doc->root->childs) {
		g_warning ("'%s' not found", file);
		g_free (file);
		stop_progress ();
		return TRUE;
	}
	g_free (file);
	cur = doc->root->childs;
	while (cur) {
		switch (cur->name[0]) {
		case 'b':
			if (strcmp (cur->name, "bts"))
				break;
			s = xmlGetProp (cur, "name");
			switch (s[0]) {
			case 'd':
				if (strcmp (s, "debian"))
					break;
				druid_data.bts = &debian_bts;
				break;
			}
			xmlFree (s);
			if (druid_data.bts)
				druid_data.bts->init (cur);
			break;
		case 'v':
			if (strcmp (cur->name, "version-stuff"))
				break;
			cur2 = cur->childs;
			while (cur2) {
				if (strcmp (cur2->name, "package"))
					continue;
				druid_data.packages = g_slist_append (
					druid_data.packages, 
					make_package_from_node (cur2));
				cur2 = cur2->next;
			}
			break;
		}
		cur = cur->next;
	}
	xmlFreeDoc (doc);

	update_das_clist ();

	return FALSE;
}
