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

#ifndef __BUG_BUDDY_H__
#define __BUG_BUDDY_H__

#include <glade/glade.h>

#define SUBMIT_ADDRESS "submit@bugs.gnome.org";
#define COMMAND_SIZE 5

typedef struct {
	const gchar *label;
	const gchar *cmds[COMMAND_SIZE];	
	gint row;
} ListData;

typedef enum {
	CRASH_NONE,
	CRASH_DIALOG,
	CRASH_CORE
} CrashType;

typedef enum {
	SUBMIT_REPORT,
	SUBMIT_TO_SELF,
	SUBMIT_NONE,
	SUBMIT_FILE
} SubmitType;

typedef struct {
	GtkWidget *the_druid;

	GtkWidget *nature;
	GtkWidget *attach;
	GtkWidget *core;
	GtkWidget *less;
	GtkWidget *misc;

	GtkWidget *gdb_text;
	GtkWidget *app_file;
	GtkWidget *pid;
	GtkWidget *core_file;

	GtkWidget *version_edit;
	GtkWidget *version_label;
	GtkWidget *version_list;
	ListData *selected_data;

	gchar *mail_cmd;
	CrashType crash_type;
	SubmitType submit_type;
	int severity;

	GladeXML *xml;
} DruidData;

extern DruidData druid_data;

void get_trace_from_core (const gchar *core_file);
void get_trace_from_pair (const gchar *app, const gchar *extra);

#endif /* __bug_buddy_h__ */




