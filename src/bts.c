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
#include "bts.h"

static void
free_package_from_node (gpointer data, gpointer udata)
{
	Package *package = data;

	g_message ("Deleting package %s...", package->name);
	xmlFree (package->name);
	xmlFree (package->pre_command);
	xmlFree (package->rpm);
	xmlFree (package->deb);
	xmlFree (package->post_command);

	g_free (package->version);
	g_free (package);
}

static Package *
make_package_from_node (xmlNodePtr node)
{
	xmlNodePtr chile;
	Package *package = g_new0 (Package, 1);

	package->name = xmlGetProp (node, "name");
	g_message ("Loading package %s...", package->name);
	chile = node->childs;
	while (chile) {
		switch (chile->name[0]) {
		case 't':
			if (strcmp ("try", chile->name)) break;
			package->pre_command = xmlNodeGetContent (chile);
			break;
		case 'l':
			if (strcmp ("last", chile->name)) break;
			package->post_command = xmlNodeGetContent (chile);
			break;
		case 'r':
			if (strcmp ("rpm", chile->name)) break;
			package->rpm = xmlNodeGetContent (chile);
			break;
		case 'd':
			if (strcmp ("deb", chile->name)) break;
			package->deb = xmlNodeGetContent (chile);
			break;
		}
		chile = chile->next;
	}
	return package;
}

gboolean
load_bts_xml ()
{
	xmlDocPtr doc;
	xmlNodePtr cur;
	char *s, *file;

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

	file = g_strconcat (BUDDY_DATADIR "/xml/", druid_data.project, NULL);
	doc = xmlParseFile (file);
	if (!doc || !doc->root || !doc->root->childs) {
		g_warning ("'%s' not found", file);
		g_free (file);
		return TRUE;
	}
	g_free (file);
	cur = doc->root->childs;
	while (cur) {
		g_message ("Node: %s", cur->name);
		if (!strcmp (cur->name, "bts")) {
			s = xmlGetProp (cur, "name");
			if (!strcmp (s, "debian"))
				druid_data.bts = &debian_bts;
			xmlFree (s);
			if (druid_data.bts)
				druid_data.bts->init (cur);
		} else if (!strcmp (cur->name, "version-stuff")) {
			xmlNodePtr cur2 = cur->childs;
			while (cur2) {
				g_message ("Subnode: %s", cur2->name);
				if (strcmp (cur2->name, "package"))
					continue;
				druid_data.packages = g_slist_append (
					druid_data.packages, 
					make_package_from_node (cur2));
				cur2 = cur2->next;
			}
		}
		cur = cur->next;
	}
	xmlFreeDoc (doc);
	return 0;
}
