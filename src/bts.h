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

#ifndef __BTS_H__
#define __BTS_H__

#include <gnome-xml/parser.h>
#include <gnome-xml/tree.h>

typedef struct _BugTrackingSystem BugTrackingSystem;
struct _BugTrackingSystem {
	gint (*init) (xmlNodePtr node);
	void (*denit) (void);

	gint (*doit) (void);
	const char *(*get_email) (void);
};

extern BugTrackingSystem debian_bts;

gboolean load_bts_xml (void);

#endif /* __BTS_H__ */
