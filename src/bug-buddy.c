/* Bug-buddy bug submitting program
 *
 * Copyright (C) 1999 - 2001 Jacob Berkman
 * Copyright 2000, 2001 Ximian, Inc.
 *
 * Author:  jacob berkman  <jacob@bug-buddy.org>
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

#include "config.h"

#include "bug-buddy.h"

#include "libglade-buddy.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <errno.h>

#include <gnome.h>
#include <libgnomeui/gnome-window-icon.h>
#include <glade/glade.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libgnomecanvas/gnome-canvas-pixbuf.h>
#include <libgnomevfs/gnome-vfs.h>

#include <libxml/tree.h>
#include <libxml/parser.h>

#include <sys/types.h>
#include <signal.h>

#define d(x)

#define DRUID_PAGE_HEIGHT 440
#define DRUID_PAGE_WIDTH  600

const gchar *submit_type[] = {
	N_("Submit bug report"),
	N_("Only send report to yourself"),
	N_("Save report to file"),
	NULL
};

const gchar *crash_type[] = {
	N_("crashed application"),
	N_("core file"),
	N_("nothing"),
	NULL
};

const gchar *bug_type[] = {
	N_("Create a new bug report"),
	N_("Add more information to existing report"),
	NULL
};

PoptData popt_data;

DruidData druid_data;

static const struct poptOption options[] = {
	{ "name",        0, POPT_ARG_STRING, &popt_data.name,         0, N_("Name of contact"),                    N_("NAME") },
	{ "email",       0, POPT_ARG_STRING, &popt_data.email,        0, N_("Email address of contact"),           N_("EMAIL") },
	{ "package",     0, POPT_ARG_STRING, &popt_data.package,      0, N_("Package containing the program"),     N_("PACKAGE") },
	{ "package-ver", 0, POPT_ARG_STRING, &popt_data.package_ver,  0, N_("Version of the package"),             N_("VERSION") },
	{ "appname",     0, POPT_ARG_STRING, &popt_data.app_file,     0, N_("File name of crashed program"),       N_("FILE") },
	{ "pid",         0, POPT_ARG_STRING, &popt_data.pid,          0, N_("PID of crashed program"),             N_("PID") },
	{ "core",        0, POPT_ARG_STRING, &popt_data.core_file,    0, N_("Core file from program"),             N_("FILE") },
	{ "include",     0, POPT_ARG_STRING, &popt_data.include_file, 0, N_("Text file to include in the report"), N_("FILE") },
	{ NULL } 
};

void
buddy_set_text_widget (GtkWidget *w, const char *s)
{
	g_return_if_fail (GTK_IS_ENTRY (w) || 
			  GTK_IS_TEXT_VIEW (w) ||
			  GTK_IS_LABEL (w));

	if (!s) s = "";

#if 0
	g_object_set (G_OBJECT (w), "text", s, NULL);
#else
	if (GTK_IS_ENTRY (w)) {
		gtk_entry_set_text (GTK_ENTRY (w), s);
	} else if (GTK_IS_TEXT_VIEW (w)) {
		gtk_text_buffer_set_text (
			gtk_text_view_get_buffer (GTK_TEXT_VIEW (w)),
			s, strlen (s));
	} else if (GTK_IS_LABEL (w)) {
		gtk_label_set_text (GTK_LABEL (w), s);
	}
#endif
}

char *
buddy_get_text_widget (GtkWidget *w)
{
	char *s = NULL;
	g_return_val_if_fail (GTK_IS_ENTRY (w) || 
			      GTK_IS_TEXT_VIEW (w) ||
			      GTK_IS_LABEL (w), NULL);
#if 0
	g_object_get (G_OBJECT (w), "text", &s, NULL);
#else
	if (GTK_IS_ENTRY (w)) {
		s = g_strdup (gtk_entry_get_text (GTK_ENTRY (w)));
	} else if (GTK_IS_TEXT_VIEW (w)) {
		GtkTextIter start, end;

		gtk_text_buffer_get_bounds (
			gtk_text_view_get_buffer (GTK_TEXT_VIEW (w)),
			&start, &end);
		s = gtk_text_iter_get_text (&start, &end);
	} else if (GTK_IS_LABEL (w)) {
		s = g_strdup (gtk_label_get_text (GTK_LABEL (w)));
	}
#endif
	return s;
}

static gboolean
update_crash_type (GtkWidget *w, gpointer data)
{
	CrashType new_type = GPOINTER_TO_INT (data);
	GtkWidget *table, *box, *scrolled, *go;

	stop_gdb ();

	druid_data.crash_type = new_type;
	
	table = GET_WIDGET ("gdb-binary-table");
	box = GET_WIDGET ("gdb-core-box");
	scrolled = GET_WIDGET ("gdb-text-scrolled");
	go = GET_WIDGET ("gdb-go");

	switch (new_type) {
	case CRASH_DIALOG:
		gtk_widget_hide (box);
		gtk_widget_show (table);
		gtk_widget_set_sensitive (scrolled, TRUE);
		gtk_widget_set_sensitive (go, TRUE);
		break;
	case CRASH_CORE:
		gtk_widget_hide (table);
		gtk_widget_show (box);
		gtk_widget_set_sensitive (scrolled, TRUE);
		gtk_widget_set_sensitive (go, TRUE);
		break;
	case CRASH_NONE:
		gtk_widget_hide (table);
		gtk_widget_hide (box);
		gtk_widget_set_sensitive (scrolled, FALSE);
		gtk_widget_set_sensitive (go, FALSE);
		break;
	}

	return FALSE;
}

static gboolean
update_submit_type (GtkWidget *w, gpointer data)
{
	SubmitType new_type = GPOINTER_TO_INT (data);
	GtkWidget *table, *box;
	druid_data.submit_type = new_type;

	table = GET_WIDGET ("email-to-table");
	box = GET_WIDGET ("email-file-gnome-entry");
	
	switch (new_type) {
	case SUBMIT_REPORT:
		gtk_widget_hide (box);
		gtk_widget_show (table);
		break;
	case SUBMIT_FILE:
		gtk_widget_hide (table);
		gtk_widget_show (box);
		break;
	case SUBMIT_TO_SELF:
		gtk_widget_hide (table);
		gtk_widget_hide (box);
		break;
	default:
		break;
	}

	gtk_widget_queue_resize (GET_WIDGET ("email-vbox"));

	return FALSE;
}

void
on_gdb_go_clicked (GtkWidget *w, gpointer data)
{
	druid_data.explicit_dirty = TRUE;
	start_gdb ();
}

void
on_gdb_stop_clicked (GtkWidget *button, gpointer data)
{
	GtkWidget *w;
	if (!druid_data.fd)
		return;

	w = gtk_message_dialog_new (GTK_WINDOW (GET_WIDGET ("druid-window")),
				    0,
				    GTK_MESSAGE_QUESTION,
				    GTK_BUTTONS_YES_NO,
				    _("gdb has not finished getting the "
				      "debugging information.\n"
				      "Kill the gdb process (the stack trace "
				      "will be incomplete)?"));

	gtk_dialog_set_default_response (GTK_DIALOG (w),
					 GTK_RESPONSE_YES);

	if (GTK_RESPONSE_YES == gtk_dialog_run (GTK_DIALOG (w))) {
		gtk_widget_destroy (w);
		if (druid_data.gdb_pid == 0) {
			d(g_message (_("gdb has already exited")));
			return;
		}
		kill (druid_data.gdb_pid, SIGTERM);
		stop_gdb ();		
		kill (druid_data.app_pid, SIGCONT);
		druid_data.explicit_dirty = TRUE;
	}
	gtk_widget_destroy (w);
}

void
stop_progress ()
{
	if (!druid_data.progress_timeout)
		return;

	gtk_timeout_remove (druid_data.progress_timeout);
	gtk_widget_hide (GET_WIDGET ("config_progress"));
}

void
on_file_radio_toggled (GtkWidget *radio, gpointer data)
{
	static GtkWidget *entry2 = NULL;
	int on = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radio));
	if (!entry2)
		entry2 = glade_xml_get_widget (druid_data.xml,
					       "file_entry2");
	
	gtk_widget_set_sensitive (GTK_WIDGET (entry2), on);
}

GtkWidget *
stock_pixmap_buddy (gchar *w, char *n, char *a, int b, int c)
{
	return gtk_image_new_from_stock (n, GTK_ICON_SIZE_SMALL_TOOLBAR);
}

void
title_configure_size (GtkWidget *w, GtkAllocation *alloc, gpointer data)
{
	gnome_canvas_set_scroll_region (GNOME_CANVAS (w), 0.0, 0.0,
					alloc->width, alloc->height);
	gnome_canvas_item_set (druid_data.title_box,
			       "x1", 0.0,
			       "y1", 0.0,
			       "x2", (gfloat)alloc->width,
			       "y2", (gfloat)alloc->height,
			       "width_units", 1.0, NULL);

	gnome_canvas_item_set (druid_data.banner,
			       "x", 15.0,
			       "y", 24.0,
			       "anchor", GTK_ANCHOR_WEST, NULL);

	gnome_canvas_item_set (druid_data.logo,
			       "x", (gfloat)(alloc->width - 48),
			       "y", -2.0,
			       NULL);
}

void
side_configure_size (GtkWidget *w, GtkAllocation *alloc, gpointer data)
{
	gnome_canvas_set_scroll_region (GNOME_CANVAS (w), 0.0, 0.0,
					alloc->width, alloc->height);
	gnome_canvas_item_set (druid_data.side_box,
			       "x1", 0.0,
			       "y1", 0.0,
			       "x2", (gfloat)alloc->width,
			       "y2", (gfloat)alloc->height,
			       "width_units", 1.0, NULL);

	gnome_canvas_item_set (druid_data.side_image,
			       "x", 0.0,
			       "y", (gfloat)(alloc->height - 179),
			       "width", 68.0,
			       "height", 179.0, NULL);
}

static void
init_canvi (void)
{
	GdkPixbuf *pb;
	GnomeCanvasItem *root;
	GnomeCanvas *canvas;
	PangoFontDescription *pfd;

	canvas = GNOME_CANVAS (GET_WIDGET ("title-canvas"));
	root = GNOME_CANVAS_ITEM (gnome_canvas_root (GNOME_CANVAS (canvas)));

	druid_data.title_box = 
		gnome_canvas_item_new (GNOME_CANVAS_GROUP (root),
				       gnome_canvas_rect_get_type (),
				       "fill_color", "black",
				       "outline_color", "black", NULL);

	pfd = pango_font_description_from_string ("Helvetica Bold 18");
	druid_data.banner =
		gnome_canvas_item_new (GNOME_CANVAS_GROUP (root),
				       gnome_canvas_text_get_type (),
				       "fill_color", "white",
				       "font_desc", pfd,
				       "anchor", GTK_ANCHOR_WEST,
				       NULL);
	pango_font_description_free (pfd);

	pb = gdk_pixbuf_new_from_file (BUDDY_DATADIR "/bug-core.png", NULL);
	druid_data.logo =
		gnome_canvas_item_new (GNOME_CANVAS_GROUP (root),
				       GNOME_TYPE_CANVAS_PIXBUF,
				       "pixbuf", pb, NULL);


	/*******************************************************/
	canvas = GNOME_CANVAS (GET_WIDGET ("side-canvas"));
	root = GNOME_CANVAS_ITEM (gnome_canvas_root (GNOME_CANVAS (canvas)));
	druid_data.side_box =
		gnome_canvas_item_new (GNOME_CANVAS_GROUP (root),
				       gnome_canvas_rect_get_type (),
				       "fill_color", "black",
				       "outline_color", "black", NULL);

	pb = gdk_pixbuf_new_from_file (BUDDY_DATADIR "/bug-flower.png", NULL);

	druid_data.side_image =
		gnome_canvas_item_new (GNOME_CANVAS_GROUP (root),
				       GNOME_TYPE_CANVAS_PIXBUF,
				       "pixbuf", pb, NULL);


	/*******************************************************/
	canvas = GNOME_CANVAS (GET_WIDGET ("gdb-canvas"));
	gnome_canvas_set_scroll_region (GNOME_CANVAS (canvas), -12.0, -12.0, 12.0, 12.0);
	root = GNOME_CANVAS_ITEM (gnome_canvas_root (GNOME_CANVAS (canvas)));
	
	pb = gdk_pixbuf_new_from_file (BUDDY_DATADIR "/bug-buddy.png", NULL);
	druid_data.throbber_pb = gdk_pixbuf_scale_simple (pb, 24, 24, GDK_INTERP_BILINEAR);
	gdk_pixbuf_unref (pb);

	druid_data.throbber =
		gnome_canvas_item_new (GNOME_CANVAS_GROUP (root),
				       GNOME_TYPE_CANVAS_PIXBUF,
				       "pixbuf", druid_data.throbber_pb,
				       "x", -6.0,
				       "y", -6.0,
				       "height", 24.0,
				       "width", 24.0,
				       NULL);
}

/* there should be no setting of default values here, I think */
static void
init_ui (void)
{
	GtkWidget *w, *m;
	gchar *s;
	int i;

	glade_xml_signal_autoconnect (druid_data.xml);

	load_config ();

	init_canvi ();

	w = GET_WIDGET ("druid-notebook");
	gtk_notebook_set_show_border (GTK_NOTEBOOK (w), FALSE);
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (w), FALSE);

	/* dialog crash page */
	s = popt_data.app_file;
	if (!s) {
		s = getenv ("GNOME_CRASHED_APPNAME");
		if (s)
			g_warning (_("$GNOME_CRASHED_APPNAME is deprecated.\n"
				     "Please use the --appname command line"
				     "argument instead."));
	}
	buddy_set_text ("gdb-binary-entry", s);

	s = popt_data.pid;
	if (!s) {
		s = getenv ("GNOME_CRASHED_PID");
		if (s)
			g_warning (_("$GNOME_CRASHED_PID is deprecated.\n"
				     "Please use the --pid command line"
				     "argument instead."));
	}
	if (s) {
		buddy_set_text ("gdb-pid-entry", s);
		druid_data.crash_type = CRASH_DIALOG;
	}

	/* core crash page */
	if (popt_data.core_file) {
		buddy_set_text ("gdb-core-entry", popt_data.core_file);
		druid_data.crash_type = CRASH_CORE;
	}

	/* package version */
	buddy_set_text ("the-version-entry", popt_data.package_ver);

        /* init some ex-radio buttons */
	m = gtk_menu_new ();
	for (i = 0; crash_type[i]; i++) {
		w = gtk_menu_item_new_with_label (_(crash_type[i]));
		g_signal_connect (G_OBJECT (w), "activate",
				  G_CALLBACK (update_crash_type),
				  GINT_TO_POINTER (i));
		gtk_widget_show (w);
		gtk_menu_shell_append (GTK_MENU_SHELL (m), w);
	}
	w = GET_WIDGET ("gdb-option");
	gtk_option_menu_set_menu (GTK_OPTION_MENU (w), m);
	gtk_option_menu_set_history (GTK_OPTION_MENU (w), druid_data.crash_type);
	update_crash_type (NULL, GINT_TO_POINTER (druid_data.crash_type));

	/* init more ex-radio buttons */
	m = gtk_menu_new ();
	for (i = 0; submit_type[i]; i++) {
		w = gtk_menu_item_new_with_label (_(submit_type[i]));
		g_signal_connect (G_OBJECT (w), "activate",
				  G_CALLBACK (update_submit_type),
				  GINT_TO_POINTER (i));
		gtk_widget_show (w);
		gtk_menu_shell_append (GTK_MENU_SHELL (m), w);
	}
	w = GET_WIDGET ("email-option");
	gtk_option_menu_set_menu (GTK_OPTION_MENU (w), m);
	gtk_option_menu_set_history (GTK_OPTION_MENU (w), druid_data.submit_type);
	update_submit_type (NULL, GINT_TO_POINTER (druid_data.submit_type));

	gnome_window_icon_set_from_default (GTK_WINDOW (GET_WIDGET ("druid-window")));

	g_object_set (G_OBJECT (GET_WIDGET ("druid-about")),
		      "label", GNOME_STOCK_ABOUT,
		      "use_stock", TRUE,
		      "use_underline", TRUE,
		      NULL);

	g_object_set (G_OBJECT (GET_WIDGET ("gdb-stop")),
		      "label", GTK_STOCK_STOP,
		      "use_stock", TRUE,
		      "use_underline", TRUE,
		      NULL);

#if 0
	/* set the cursor at the beginning of the second line */
	{
		GtkTextBuffer *buffy;
		GtkTextIter iter;

		buffy = gtk_text_view_get_buffer (GTK_TEXT_VIEW (GET_WIDGET ("desc-text")));
		gtk_text_buffer_get_iter_at_line (buffer, &iter, 2);
	}
#endif
}

gint
delete_me (GtkWidget *w, GdkEventAny *evt, gpointer data)
{
	save_config ();
	gtk_main_quit ();
	return FALSE;
}

int
main (int argc, char *argv[])
{
	GtkWidget *w;
	char *s;

	memset (&druid_data, 0, sizeof (druid_data));
	memset (&popt_data,  0, sizeof (popt_data));

	druid_data.crash_type = CRASH_NONE;
	druid_data.state = -1;

	srand (time (NULL));

	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	gnome_program_init (PACKAGE, VERSION,
			    LIBGNOMEUI_MODULE,
			    argc, argv,
			    GNOME_PARAM_POPT_TABLE, options,
			    GNOME_PARAM_POPT_FLAGS, 0,
			    NULL);

	gnome_window_icon_set_default_from_file (BUDDY_ICONDIR"/bug-buddy.png");

#if 0
	s = "bug-buddy.glade2";
	if (!g_file_tests (s))
#endif
		s = BUDDY_DATADIR "/bug-buddy.glade2";

	druid_data.xml = glade_xml_new (s, NULL, GETTEXT_PACKAGE);

	if (!druid_data.xml) {
		w = gtk_message_dialog_new (NULL,
					    0,
					    GTK_MESSAGE_ERROR,
					    GTK_BUTTONS_OK,
					    _("Bug Buddy could not load its user interface file (%s).\n"
					      "Please make sure Bug Buddy was installed correctly."), 
					    s);
					      
		gtk_dialog_set_default_response (GTK_DIALOG (w),
						 GTK_RESPONSE_OK);
		gtk_dialog_run (GTK_DIALOG (w));
		gtk_widget_destroy (w);
		return 0;
	}

	init_ui ();

	gtk_widget_show (GET_WIDGET ("druid-window"));

	if (getenv ("BUG_ME_HARDER"))
		gtk_widget_show (GET_WIDGET ("progress-window"));
	
	druid_set_state (STATE_INTRO);	

	if (druid_data.already_run && 
	    gtk_toggle_button_get_active (
		    GTK_TOGGLE_BUTTON (GET_WIDGET ("intro-skip-toggle"))))
		on_druid_next_clicked (NULL, NULL);

	load_bugzillas ();


	gtk_main ();

	return 0;
}
