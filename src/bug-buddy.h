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

#ifndef __BUG_BUDDY_H__
#define __BUG_BUDDY_H__

#include <glade/glade.h>
#include <sys/types.h>
#include "distro.h"
#include "bts.h"

#define SUBMIT_ADDRESS "@bugs.gnome.org"

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

typedef enum {
	BUG_NEW,
	BUG_EXISTING
} BugType;

typedef struct {
	GtkWidget *the_druid;

	int selected_row;
	Distribution *distro;

	CrashType crash_type;
	SubmitType submit_type;
	BugType bug_type;

	int severity;
	int bug_class;

	GladeXML *xml;

	pid_t app_pid;

	pid_t gdb_pid;
	GIOChannel *ioc;
	int fd;

	char *project;
	BugTrackingSystem *bts;

	GSList *projects;
	GSList *packages;
	gboolean explicit_dirty;
} DruidData;

extern DruidData druid_data;
extern const gchar *severity[];
extern const gchar *bug_class[][2];

void get_trace_from_core (const gchar *core_file);
void get_trace_from_pair (const gchar *app, const gchar *extra);
void stop_gdb (void);
void start_gdb (void);

#endif /* __bug_buddy_h__ */





