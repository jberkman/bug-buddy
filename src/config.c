/* bug-buddy bug submitting program
 *
 * Copyright (C) 1999 - 2000 Jacob Berkman
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

#include "config.h"

#include "bug-buddy.h"

#include <gnome.h>
#include <gconf/gconf-client.h>

#define BUG_BUDDY_ALREADY_RUN_SERIAL 2

#define d(x)

#define INT(s) (strtol ((s), NULL, 0))

typedef enum {
	CONFIG_DONE,
	CONFIG_TOGGLE,
	CONFIG_ENTRY,
	CONFIG_USER,
	CONFIG_MAILER,
	CONFIG_INT_ENTRY,
} ConfigType;

typedef struct {
	ConfigType t;
	const char *w;
	const char *path;
	const char *w2;
} ConfigItem;

static ConfigItem configs[] = {
/*	{ CONFIG_TOGGLE, "intro-skip-toggle",          "/bug-buddy/last/skip-intro=1" }, */
	{ CONFIG_ENTRY,  "gdb-binary-gnome-entry" },
	{ CONFIG_ENTRY,  "gdb-core-gnome-entry" },
/*	{ CONFIG_TOGGLE, "gdb-continue-toggle",        "/bug-buddy/last/gdb-continue=1" },*/
	{ CONFIG_ENTRY,  "desc-file-gnome-entry" },
/*	{ CONFIG_ENTRY,  "bts-package-gnome-entry" },*/
/*	{ CONFIG_ENTRY,  "bts-module-gnome-entry" }, */
/*	{ CONFIG_TOGGLE, "version-toggle",             "/bug-buddy/last/skip_conf=1" }, */
	{ CONFIG_USER,   "email-name-gnome-entry",     "/bug-buddy/last/name",          "email-name-entry" },
	{ CONFIG_ENTRY,  "email-email-gnome-entry",    "/bug-buddy/last/email_address", "email-email-entry" },
	{ CONFIG_ENTRY,  "email-to-gnome-entry" },
	{ CONFIG_ENTRY,  "email-cc-gnome-entry" },
	{ CONFIG_MAILER, "email-sendmail-gnome-entry", "/bug-buddy/last/mailer",        "email-sendmail-entry" },
	{ CONFIG_ENTRY,  "email-file-gnome-entry",     "/bug-buddy/last/bugfile",       "email-file-entry" },
/*	{ CONFIG_TOGGLE, "email-cc-toggle",            "/bug-buddy/last/cc=0" }, */
	{ CONFIG_TOGGLE, "email-sendmail-radio",       "/bug-buddy/last/use_sendmail=false" },
	{ CONFIG_TOGGLE, "email-custom-radio",         "/bug-buddy/last/use_custom_mailer=false" },
	{ CONFIG_ENTRY,  "email-command-gnome-entry",  "/bug-buddy/custom_mailer/command=evolution", "email-command-entry"},
	{ CONFIG_TOGGLE, "email-terminal-toggle",      "/bug-buddy/custom_mailer/start_in_terminal" },
	{ CONFIG_TOGGLE, "email-remote-toggle",        "/bug-buddy/custom_mailer/use_moz_remote" },
	{ CONFIG_DONE }
};

static ConfigItem gconf_configs[] = {
	{ CONFIG_TOGGLE,     "use-http-proxy",                    "/system/gnome-vfs/use-http-proxy" },
	{ CONFIG_ENTRY,      "http-proxy-host",                   "/system/gnome-vfs/http-proxy-host" },
	{ CONFIG_INT_ENTRY,  "http-proxy-port",                   "/system/gnome-vfs/http-proxy-port" },
	{ CONFIG_TOGGLE,     "use-http-proxy-authorization",      "/system/gnome-vfs/use-http-proxy-authorization" },
	{ CONFIG_ENTRY,      "http-proxy-authorization-user",     "/system/gnome-vfs/http-proxy-authorization-user" },
	{ CONFIG_ENTRY,      "http-proxy-authorization-password", "/system/gnome-vfs/http-proxy-authorization-password" },
	{ CONFIG_DONE }
};

static MailerItem default_mailers[] = {
	{ N_("Evolution"), "evolution", FALSE, FALSE },
	{ NULL }
};

static void
update_string (GtkEntry *entry, const char *key)
{
	GConfClient *client;

	client = gconf_client_get_default ();
	gconf_client_set_string (client, key,
				 gtk_entry_get_text (entry),
				 NULL);
}

void
gconf_buddy_connect_string (const char *widget_name, const char *key)
{
	GtkWidget *widget;
	GConfClient *client;
	char *s;

	g_return_if_fail (key != NULL);
	widget = GET_WIDGET (widget_name);
	g_return_if_fail (GTK_IS_ENTRY (widget));

	client = gconf_client_get_default ();
	s = gconf_client_get_string (client, key, NULL);

	gtk_entry_set_text (GTK_ENTRY (widget), s?s:"");
	g_free (s);

	/* FIXME: fix memleak */
	g_signal_connect (widget, "changed",
			  G_CALLBACK (update_string),
			  g_strdup (key));
}

static void
update_bool (GtkToggleButton *button, const char *key)
{
	GConfClient *client;
	
	client = gconf_client_get_default ();
	gconf_client_set_bool (client, key, button->active, NULL);
}

void
gconf_buddy_connect_bool (const char *widget_name, const char *key)
{
	GtkWidget *widget;
	GConfClient *client;
	gboolean active;

	g_return_if_fail (key != NULL);
	widget = GET_WIDGET (widget_name);
	g_return_if_fail (GTK_IS_TOGGLE_BUTTON (widget));

	client = gconf_client_get_default ();
	active = gconf_client_get_bool (client, key, NULL);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				      active);

	/* FIXME: fix leak */
	g_signal_connect (widget, "toggled",
			  G_CALLBACK (update_bool),
			  g_strdup (key));
}

static void
update_int (GtkEntry *entry, const char *key)
{
	GConfClient *client;

	client = gconf_client_get_default ();
	gconf_client_set_int (client, key,
			      INT (gtk_entry_get_text (entry)),
			      NULL);
}

void
gconf_buddy_connect_int (const char *widget_name, const char *key)
{
	GtkWidget *widget;
	GConfClient *client;
	char *s;

	g_return_if_fail (key != NULL);
	widget = GET_WIDGET (widget_name);
	g_return_if_fail (GTK_IS_ENTRY (widget));

	client = gconf_client_get_default ();
	s = g_strdup_printf ("%d", gconf_client_get_int (client, key, NULL));

	gtk_entry_set_text (GTK_ENTRY (widget), s);
	g_free (s);

	/* FIXME: fix memleak */
	g_signal_connect (widget, "changed",
			  G_CALLBACK (update_int),
			  g_strdup (key));
}

void
save_config (void)
{
	ConfigItem *item;
	GtkWidget *w;
	gboolean b;
	char *s;

	d(g_print ("saving config...\n"));

	for (item = configs; item->t; item++) {
		if (item->t == CONFIG_TOGGLE) {
			b = gtk_toggle_button_get_active (
				GTK_TOGGLE_BUTTON (GET_WIDGET (item->w)));
			gnome_config_set_bool (item->path, b);
			continue;
		}

		if (item->path) {
			w = GET_WIDGET (item->w2);;
			s = buddy_get_text (item->w2);
			gnome_config_set_string (item->path, s);
		} else {
			s = NULL;
		}
		
		w = GET_WIDGET (item->w);
		if (GNOME_IS_FILE_ENTRY (w))
			w = gnome_file_entry_gnome_entry (GNOME_FILE_ENTRY (w));

		if (s && *s)
			gnome_entry_prepend_history (GNOME_ENTRY (w), TRUE, s);

		g_free (s);
#ifdef FIXME
		gnome_entry_save_history (GNOME_ENTRY (w));
#endif
	}

	gnome_config_set_int ("/bug-buddy/last/submittype", 
			      druid_data.submit_type);

	gnome_config_set_int ("/bug-buddy/last/already_run", BUG_BUDDY_ALREADY_RUN_SERIAL);

	gnome_config_set_string ("/bug-buddy/last/gnome_mailer",
				 _(druid_data.mailer->name));

	gnome_config_set_bool ("/bug-buddy/last/show_debugging", 
			       gtk_notebook_get_current_page (GTK_NOTEBOOK (GET_WIDGET ("gdb-notebook"))));
			       
	gnome_config_sync ();
}

void
load_config (void)
{
	ConfigItem *item;
	GtkWidget *w;
	MailerItem *mailer;
	char *def = NULL, *d2;
	
	d(g_print ("loading config...\n"));

	for (item = configs; item->t; item++) {
		switch (item->t) {
		case CONFIG_TOGGLE:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (GET_WIDGET (item->w)),
				gnome_config_get_bool (item->path));
			continue;
		case CONFIG_USER:
			def = g_strdup (g_get_real_name ());
			break;
		case CONFIG_MAILER:
			def = g_find_program_in_path ("sendmail");
			if (!def) {
				if (g_file_test ("/usr/sbin/sendmail", G_FILE_TEST_EXISTS))
					def = g_strdup ("/usr/sbin/sendmail");
				else if (g_file_test ("/usr/lib/sendmail", G_FILE_TEST_EXISTS))
					def = g_strdup ("/usr/lib/sendmail");
			}
			break;
		default:
			break;
		}

		if (item->w2) {
			if (item->path) {
				d2 = gnome_config_get_string (item->path);
				if (d2) {
					g_free (def);
					def = d2;
				}
			}
			buddy_set_text (item->w2, def);
		}

		g_free (def);
		def = NULL;

		w = GET_WIDGET (item->w);
		if (GNOME_IS_FILE_ENTRY (w))
			w = gnome_file_entry_gnome_entry (GNOME_FILE_ENTRY (w));

#ifdef FIXME
		gnome_entry_load_history (GNOME_ENTRY (w));
#endif
	}

	for (item = gconf_configs; item->t; item++) {
		switch (item->t) {
		case CONFIG_TOGGLE:
			gconf_buddy_connect_bool (item->w, item->path);
			break;
		case CONFIG_ENTRY:
			gconf_buddy_connect_string (item->w, item->path);
			break;
		case CONFIG_INT_ENTRY:
			gconf_buddy_connect_int (item->w, item->path);
			break;
		default:
			g_assert_not_reached ();
		}
	}

	druid_data.mailer_hash = g_hash_table_new (g_str_hash, g_str_equal);
	for (mailer = default_mailers; mailer->name; mailer++)
		g_hash_table_insert (druid_data.mailer_hash, _(mailer->name), mailer);

	{
		char *s = g_strconcat ("/bug-buddy/last/gnome_mailer=", _(default_mailers[0].name), NULL);
		char *mailer = gnome_config_get_string (s);
		g_free (s);
		druid_data.mailer = g_hash_table_lookup (druid_data.mailer_hash, mailer);
		g_free (mailer);
		if (!druid_data.mailer)
			druid_data.mailer = &default_mailers[0];
	}

	druid_data.submit_type =
		gnome_config_get_int ("/bug-buddy/last/submittype");

	druid_data.already_run =
		(gnome_config_get_int ("/bug-buddy/last/already_run=0") >= BUG_BUDDY_ALREADY_RUN_SERIAL);

	druid_data.state = 0;

	if (gnome_config_get_bool ("/bug-buddy/last/show_debugging=0"))
		gtk_button_clicked (GTK_BUTTON (GET_WIDGET ("debugging-options-button")));
}
