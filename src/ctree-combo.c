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

const gchar *ctree_combo_string_key = "ctree-combo-string-value";

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
set_scroll_size (CTreeCombo *w, GtkAllocation *alloc, gpointer data)
{
	gtk_widget_set_usize (w->scroller->parent,
			      GTK_WIDGET (w)->allocation.width,
			      150);
	return FALSE;
}

#if 0
static gchar *
gtk_combo_func (GtkListItem * li)
{
	GtkWidget *label;
	gchar *ltext = NULL;
	
	ltext = (gchar *) gtk_object_get_data (GTK_OBJECT (li),
					       ctree_combo_string_key);
	if (!ltext)
	{
		label = GTK_BIN (li)->child;
		if (!label || !GTK_IS_LABEL (label))
			return NULL;
		gtk_label_get (GTK_LABEL (label), &ltext);
	}
	return ltext;
}

static int
gtk_combo_entry_key_press (GtkEntry *entry, GdkEventKey *event, 
			   CTreeCombo *combo)
{
	GList *li;
	if (!combo->use_arrows || !GTK_LIST (combo->list)->children)
		return FALSE;
	
	li = g_list_find (GTK_LIST (combo->list)->children, gtk_combo_find (combo));
	
	if ((event->keyval == GDK_Up)
	    || (event->keyval == GDK_KP_Up)
	    || ((event->state & GDK_MOD1_MASK) && ((event->keyval == 'p') || (event->keyval == 'P'))))
	{
		if (li)
			li = li->prev;
		if (!li && combo->use_arrows_always)
		{
			li = g_list_last (GTK_LIST (combo->list)->children);
		}
		if (li)
		{
			gtk_list_select_child (GTK_LIST (combo->list), GTK_WIDGET (li->data));
			gtk_signal_emit_stop_by_name (GTK_OBJECT (entry), "key_press_event");
			return TRUE;
		}
	}
	else if ((event->keyval == GDK_Down)
		 || (event->keyval == GDK_KP_Down)
		 || ((event->state & GDK_MOD1_MASK) && ((event->keyval == 'n') || (event->keyval == 'N'))))
	{
		if (li)
			li = li->next;
		if (!li && combo->use_arrows_always)
		{
			li = GTK_LIST (combo->list)->children;
		}
		if (li)
		{
			gtk_list_select_child (GTK_LIST (combo->list), GTK_WIDGET (li->data));
			gtk_signal_emit_stop_by_name (GTK_OBJECT (entry), "key_press_event");
			return TRUE;
		}
	}
	return FALSE;
}
#endif

static int
on_ctree_select_row (GtkCTree *ctree, GList *node, int col, gpointer data)
{
	CTreeCombo *combo = data;
	char *text = NULL;
	if (GTK_CTREE_ROW (node)->children)
		return FALSE;

	gtk_ctree_get_node_info (GTK_CTREE (ctree), GTK_CTREE_NODE (node), 
				 &text,
				 NULL, NULL, NULL, NULL, NULL, NULL, NULL);
				 
	if (text) gtk_entry_set_text (GTK_ENTRY (combo->entry), text);

	gtk_combo_box_popup_hide (GTK_COMBO_BOX (combo));
	return FALSE;
}

void
ctree_combo_construct (CTreeCombo *cc, gint columns,
		       gint tree_column, gchar *titles[])		       
{
	g_return_if_fail (cc != NULL);
	g_return_if_fail (IS_CTREE_COMBO (cc));

	cc->entry = gtk_entry_new ();
	gtk_widget_show (cc->entry);

	cc->scroller = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (cc->scroller),
					GTK_POLICY_NEVER,
					GTK_POLICY_ALWAYS);
	gtk_widget_show (cc->scroller);

	gtk_signal_connect_after (GTK_OBJECT (cc), "size-allocate",
				  GTK_SIGNAL_FUNC (set_scroll_size),
				  NULL);

	cc->ctree = gtk_ctree_new_with_titles (columns, tree_column, titles);
	gtk_clist_set_selection_mode (GTK_CLIST (cc->ctree),
				      GTK_SELECTION_SINGLE);
	gtk_signal_connect_after (GTK_OBJECT (cc->ctree), "tree-select-row",
				  GTK_SIGNAL_FUNC (on_ctree_select_row), cc);
	gtk_widget_show (cc->ctree);

	gtk_scrolled_window_add_with_viewport (
		GTK_SCROLLED_WINDOW (cc->scroller), cc->ctree);

	gtk_combo_box_construct (GTK_COMBO_BOX (cc), cc->entry,
				 cc->scroller);
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
