/* bug-buddy bug submitting program
 *
 * Copyright (C) 1999, 2000 Jacob Berkman
 * Copyright 2000 Helix Code, Inc.
 *
 * Author:  Jacob Berkman  <jacob@helixcode.com>
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

typedef enum {
	CRASH_DIALOG,
	CRASH_CORE,
	CRASH_NONE
} CrashType;

typedef enum {
	SUBMIT_REPORT,
	SUBMIT_TO_SELF,
	SUBMIT_FILE
} SubmitType;

typedef enum {
	BUG_NEW,
	BUG_EXISTING
} BugType;

typedef enum {
	SEVERITY_CRITICAL,
	SEVERITY_GRAVE,
	SEVERITY_NORMAL,
	SEVERITY_WISHLIST,
} SeverityType;

typedef enum {
	BUG_CLASS_SW,
	BUG_CLASS_DOC,
	BUG_CLASS_CHANGE,
	BUG_CLASS_SUPPORT
} BugClassType;

typedef struct {
	/* contact page */
	gchar *name;
	gchar *email;
	
	/* package page */
	gchar *package;
	gchar *package_ver;
	
	/* dialog page */
	gchar *app_file;
	gchar *pid;
	
	/* core page */
	gchar *core_file;
} PoptData;
extern PoptData popt_data;

typedef struct {
	GtkWidget *the_druid;
	GladeXML  *xml;

	gboolean default_email;

	Distribution      *distro;
	BugTrackingSystem *bts;
	char              *bts_file;

	CrashType  crash_type;

	int selected_row;
	int progress_timeout;

	/* gdb stuff */
	pid_t       app_pid;
	pid_t       gdb_pid;
	GIOChannel *ioc;
	int         fd;
	gboolean    explicit_dirty;

	/* Debian BTS stuff */
	SubmitType    submit_type;
	BugType       bug_type; 
	SeverityType  severity;
	BugClassType  bug_class;
	GSList       *packages;
} DruidData;
extern DruidData druid_data;

extern const gchar *severity[];
extern const gchar *bug_class[][2];

void get_trace_from_core (const gchar *core_file);
void get_trace_from_pair (const gchar *app, const gchar *extra);
void stop_gdb (void);
void start_gdb (void);

void stop_progress (void);

void append_packages (void);
#endif /* __bug_buddy_h__ */
