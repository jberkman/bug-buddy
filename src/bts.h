/* bug-buddy bug submitting program
 *
 * Copyright (C) 1999, 2000, 2001 Jacob Berkman
 * Copyright 2000, 2001 Ximian, Inc.
 *
 * Author:  Jacob Berkman  <jacob@bug-buddy.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
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

#ifndef __BTS_H__
#define __BTS_H__

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <gtk/gtkeditable.h>

#define DEFAULT_BTS "GNOME.bts"

typedef struct _BugTrackingSystem BugTrackingSystem;
struct _BugTrackingSystem {
	gint (*init) (xmlNodePtr node);
	void (*denit) (void);

	void (*doit) (GtkEditable *);
	const char *(*get_email) (void);
};

extern BugTrackingSystem debian_bts;

gboolean load_bts_xml (void);
void update_das_clist (void);

#endif /* __BTS_H__ */
