/*
 * Save a file.
 *
 * Taken from gedit:
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 *
 * (although i think federico wrote this part, taking it from emacs)
 *
 * Hacked up by jacob berkman  <jacob@bug-buddy.org>
 *
 * Copyright 2002 Ximian, Inc.
 *
 */

#include <config.h>

#include "save-buddy.h"

#include <libgnome/gnome-i18n.h>
#include <gtk/gtk.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_LINK_LEVEL 256

GQuark
bb_save_error_quark (void)
{
	static GQuark quark = 0;

	if (!quark)
		quark = g_quark_from_static_string ("bb_save_error");

	return quark;
}

/* Does readlink() recursively until we find a real filename. */
static char *
follow_symlinks (const char *filename, GError **error)
{
	char *followed_filename;
	int link_count;

	g_return_val_if_fail (filename != NULL, NULL);
	
	g_return_val_if_fail (strlen (filename) + 1 <= MAXPATHLEN, NULL);

	followed_filename = g_strdup (filename);
	link_count = 0;

	while (link_count < MAX_LINK_LEVEL)
	{
		struct stat st;

		if (lstat (followed_filename, &st) != 0)
			/* We could not access the file, so perhaps it does not
			 * exist.  Return this as a real name so that we can
			 * attempt to create the file.
			 */
			return followed_filename;

		if (S_ISLNK (st.st_mode))
		{
			int len;
			char linkname[MAXPATHLEN];

			link_count++;

			len = readlink (followed_filename, linkname, MAXPATHLEN - 1);

			if (len == -1)
			{
				g_set_error (error, BB_SAVE_ERROR, errno,
					     _("Could not read symbolic link information "
					       "for %s"), followed_filename);
				g_free (followed_filename);
				return NULL;
			}

			linkname[len] = '\0';

			/* If the linkname is not an absolute path name, append
			 * it to the directory name of the followed filename.  E.g.
			 * we may have /foo/bar/baz.lnk -> eek.txt, which really
			 * is /foo/bar/eek.txt.
			 */

			if (linkname[0] != G_DIR_SEPARATOR)
			{
				char *slashpos;
				char *tmp;

				slashpos = strrchr (followed_filename, G_DIR_SEPARATOR);

				if (slashpos)
					*slashpos = '\0';
				else
				{
					tmp = g_strconcat ("./", followed_filename, NULL);
					g_free (followed_filename);
					followed_filename = tmp;
				}

				tmp = g_build_filename (followed_filename, linkname, NULL);
				g_free (followed_filename);
				followed_filename = tmp;
			}
			else
			{
				g_free (followed_filename);
				followed_filename = g_strdup (linkname);
			}
		} else
			return followed_filename;
	}

	/* Too many symlinks */

	g_set_error (error, BB_SAVE_ERROR, ELOOP,
		     _("The file has too many symbolic links."));

	return NULL;
}

/* FIXME: define new ERROR_CODE and remove the error 
 * strings from here -- Paolo
 */

gboolean	
bb_save_file (GtkWindow *parent, const char *filename, const char *utf8data, GError **error)
{
	char *real_filename; /* Final filename with no symlinks */
	char *backup_filename; /* Backup filename, like real_filename.bak */
	char *temp_filename; /* Filename for saving */
	char *slashpos;
	char *dirname;
	mode_t saved_umask;
	struct stat st;
	const char *chars;
	int chars_len;
	int fd;
	int retval;
	gboolean create_backup_copy = TRUE;
	
	g_return_val_if_fail (utf8data != NULL, FALSE);
	g_return_val_if_fail (g_utf8_validate (utf8data, -1, NULL), FALSE);
	if (parent)
		g_return_val_if_fail (GTK_IS_WINDOW (parent), FALSE);

	retval = FALSE;

	real_filename = NULL;
	backup_filename = NULL;
	temp_filename = NULL;

	/* We don't support non-file:/// stuff */

	if (!filename || !*filename) {
		g_set_error (error, BB_SAVE_ERROR, 0,
			     _("Invalid filename."));
		goto out;
	}

	/* Get the real filename and file permissions */

	real_filename = follow_symlinks (filename, error);

	if (!real_filename)
		goto out;

	if (stat (real_filename, &st) != 0) {
		/* File does not exist? */
		create_backup_copy = FALSE;

		/* Use default permissions */
		st.st_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
		st.st_uid = getuid ();
		st.st_gid = getgid ();
	} else {
		GtkWidget *d;

		d = gtk_message_dialog_new (parent, 0,
					    GTK_MESSAGE_QUESTION,
					    GTK_BUTTONS_NONE,
					    _("There already exists a file name '%s'.\n\n"
					      "Do you wish to overwrite this file?"),
					    filename);
		gtk_dialog_add_buttons (GTK_DIALOG (d),
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					_("_Overwrite"), GTK_RESPONSE_OK,
					NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (d), GTK_RESPONSE_OK);

		if (GTK_RESPONSE_OK != gtk_dialog_run (GTK_DIALOG (d))) {
			/* set g_error */
			gtk_widget_destroy (d);
			g_set_error (error, BB_SAVE_ERROR, 0,
				     _("The file already exists."));
			goto out;
		}
		gtk_widget_destroy (d);
	}

	/* Save to a temporary file.  We set the umask because some (buggy)
	 * implementations of mkstemp() use permissions 0666 and we want 0600.
	 */

	slashpos = strrchr (real_filename, G_DIR_SEPARATOR);

	if (slashpos)
	{
		dirname = g_strdup (real_filename);
		dirname[slashpos - real_filename + 1] = '\0';
	}
	else
		dirname = g_strdup (".");

	temp_filename = g_build_filename (dirname, ".bug-buddy-save-XXXXXX", NULL);
	g_free (dirname);

	saved_umask = umask (0077);
	fd = g_mkstemp (temp_filename); /* this modifies temp_filename to the used name */
	umask (saved_umask);

	if (fd == -1)
	{
		g_set_error (error, BB_SAVE_ERROR, errno, " ");
		goto out;
	}

#if 0
	encoding_setting = gedit_prefs_manager_get_save_encoding ();

	if (encoding_setting == GEDIT_SAVE_CURRENT_LOCALE_IF_POSSIBLE)
	{
		GError *conv_error = NULL;
		char* converted_file_contents = NULL;

		gedit_debug (DEBUG_DOCUMENT, "Using current locale's encoding");

		converted_file_contents = g_locale_from_utf8 (chars, -1, NULL, NULL, &conv_error);

		if (conv_error != NULL)
		{
			/* Conversion error */
			g_error_free (conv_error);
		}
		else
		{
			g_free (chars);
			chars = converted_file_contents;
		}
	}
	else
	{
		if ((doc->priv->encoding != NULL) &&
		    ((encoding_setting == GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE) ||
		     (encoding_setting == GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE_NCL)))
		{
			GError *conv_error = NULL;
			char* converted_file_contents = NULL;

			gedit_debug (DEBUG_DOCUMENT, "Using encoding %s", doc->priv->encoding);

			/* Try to convert it from UTF-8 to original encoding */
			converted_file_contents = g_convert (chars, -1, 
							     doc->priv->encoding, "UTF-8", 
							     NULL, NULL, &conv_error); 

			if (conv_error != NULL)
			{
				/* Conversion error */
				g_error_free (conv_error);
			}
			else
			{
				g_free (chars);
				chars = converted_file_contents;
			}
		}
		else
			gedit_debug (DEBUG_DOCUMENT, "Using UTF-8 (Null)");

	}	    
#else
	chars = utf8data;
#endif

	chars_len = strlen (chars);
	if (write (fd, chars, chars_len) != chars_len)
	{
		char *msg;

		switch (errno)
		{
			case ENOSPC:
				msg = _("There is not enough disk space to save the file.\n"
					"Please free some disk space and try again.");
				break;

			case EFBIG:
				msg = _("The disk where you are trying to save the file has "
					"a limitation on file sizes.  Please try saving "
					"a smaller file or saving it to a disk that does not "
					"have this limitation.");
				break;

			default:
				msg = " ";
				break;
		}

		g_set_error (error, BB_SAVE_ERROR, errno, msg);
		close (fd);
		unlink (temp_filename);
		goto out;
	}

	if (close (fd) != 0)
	{
		g_set_error (error, BB_SAVE_ERROR, errno, " ");
		unlink (temp_filename);
		goto out;
	}

	/* Move the original file to a backup */

	if (create_backup_copy) {	
		int result;

		backup_filename = g_strconcat (real_filename, 
				               "~",
					       NULL);

		result = rename (real_filename, backup_filename);

		if (result != 0)
		{
			g_set_error (error, BB_SAVE_ERROR, errno,
				     _("Could not create a backup file."));
			unlink (temp_filename);
			goto out;
		}
	}

	/* Move the temp file to the original file */

	if (rename (temp_filename, real_filename) != 0)
	{
		int saved_errno;

		saved_errno = errno;

#if 0
		if (create_backup_copy && rename (backup_filename, real_filename) != 0)
			g_set_error (error, BB_SAVE_ERROR, saved_errno,
				     " ");
		else
#endif
			g_set_error (error, BB_SAVE_ERROR, saved_errno,
				     " ");

		goto out;
	}

	/* Restore permissions.  There is not much error checking we can do
	 * here, I'm afraid.  The final data is saved anyways.
	 */

	chmod (real_filename, st.st_mode);
	chown (real_filename, st.st_uid, st.st_gid);

	retval = TRUE;

	/* Done */

 out:
	g_free (real_filename);
	g_free (backup_filename);
	g_free (temp_filename);

	return retval;
}
