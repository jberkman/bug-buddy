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
#include <gnome.h>
#include "ctree-combo.h"

static GtkObjectClass *ctree_combo_parent_class;

static void
ctree_combo_class_init (GtkObjectClass *object_class)
{
	ctree_combo_parent_class = gtk_type_class (gtk_combo_box_get_type ());
}

GtkType
ctree_combo_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"CTreeCombo",
			sizeof (CTreeCombo),
			sizeof (CTreeComboClass),
			(GtkClassInitFunc) ctree_combo_class_init,
			(GtkObjectInitFunc) NULL,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gtk_combo_box_get_type (), &info);
	}

	return type;
}

static gint
set_scroll_size (GtkWidget *w, GtkAllocation *alloc, gpointer data)
{
	GtkWidget *scroller = data;
	g_message ("setting size to %d", w->allocation.width);
	gtk_widget_set_usize (scroller, 
			      w->allocation.width,
			      175);
	return FALSE;
}

static int
on_clist_select_row (GtkCList *clist, gint row, gint col,
		     GdkEventButton *event, gpointer data)
{
	CTreeCombo *combo = data;
	char *text = NULL;
	g_message ("col: %d", col);
	gtk_clist_get_text (clist, row, 0, &text);
	g_message ("text: %s", text);
	gtk_clist_get_text (clist, row, 1, &text);
	g_message ("text: %s", text);
	gtk_clist_get_text (clist, row, 2, &text);
	g_message ("text: %s", text);
	gtk_entry_set_text (GTK_ENTRY (combo->entry), text);
	gtk_combo_box_popup_hide (GTK_COMBO_BOX (combo));
	return FALSE;
}

void
ctree_combo_construct (CTreeCombo *cc, gint columns,
		       gint tree_column, gchar *titles[])		       
{
	GtkWidget *scroller;

	g_return_if_fail (cc != NULL);
	g_return_if_fail (IS_CTREE_COMBO (cc));

	cc->entry = gtk_entry_new ();
	gtk_widget_show (cc->entry);

	scroller = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
					GTK_POLICY_ALWAYS,
					GTK_POLICY_ALWAYS);
	gtk_widget_show (scroller);

	gtk_signal_connect_after (GTK_OBJECT (cc), "size-allocate",
				  GTK_SIGNAL_FUNC (set_scroll_size),
				  scroller);

	cc->ctree = gtk_ctree_new_with_titles (columns, tree_column, titles);
	gtk_clist_set_selection_mode (GTK_CLIST (cc->ctree),
				      GTK_SELECTION_SINGLE);
	gtk_signal_connect_after (GTK_OBJECT (cc->ctree), "select-row",
				  GTK_SIGNAL_FUNC (on_clist_select_row),
				  cc);
	gtk_widget_show (cc->ctree);

	gtk_scrolled_window_add_with_viewport (
		GTK_SCROLLED_WINDOW (scroller), cc->ctree);

	gtk_combo_box_construct (GTK_COMBO_BOX (cc),
				 cc->entry, scroller);
}

GtkWidget *
ctree_combo_new_with_titles (gint columns, gint tree_column, gchar *titles[])
{
	CTreeCombo *cc;

	cc = gtk_type_new (CTREE_COMBO_TYPE);
	ctree_combo_construct (cc, columns, tree_column, titles);
	return GTK_WIDGET (cc);
}

GtkWidget *
ctree_combo_new (gint columns, gint tree_column)
{
	return ctree_combo_new_with_titles (columns, tree_column, NULL);
}
