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

#ifndef __GTK_CTREE_COMBO_H__
#define __GTK_CTREE_COMBO_H__


#include <gtk/gtkctree.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkscrolledwindow.h>
#include "gtk-combo-box.h"

BEGIN_GNOME_DECLS

#define CTREE_COMBO_TYPE     (ctree_combo_get_type ())
#define CTREE_COMBO(o)       (GTK_CHECK_CAST((o), CTREE_COMBO_TYPE, CTreeCombo))
#define CTREE_COMBO_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), CTREE_COMBO_TYPE, CTreeComboClass))
#define IS_CTREE_COMBO(o)    (GTK_CHECK_TYPE ((o), CTREE_COMBO_TYPE))

typedef struct {
	GtkComboBox combo_box;

	GtkWidget *entry;
	GtkWidget *ctree;
} CTreeCombo;

typedef struct {
	GtkComboBoxClass parent_class;
} CTreeComboClass;

GtkType ctree_combo_get_type (void);

GtkWidget *ctree_combo_new             (gint columns, 
					gint tree_column);
GtkWidget *ctree_combo_new_with_titles (gint columns,
					gint tree_column,
					gchar *titles[]);

void ctree_combo_construct (CTreeCombo *cc, 
			    gint columns,
			    gint tree_column,
			    gchar *titles[]);

END_GNOME_DECLS

#endif /* __GTK_CTREE_COMBO_H__ */