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

#ifndef __GLADE_DRUID_H__
#define __GLADE_DRUID_H__

#define GET_WIDGET(name) (glade_xml_get_widget (druid_data.xml, (name)))

#define THE_DRUID      GET_WIDGET ("the_druid")

#define ACTION_PAGE    GET_WIDGET ("action_page")
#define COMPLETE_PAGE  GET_WIDGET ("complete_page")
#define NATURE_PAGE    GET_WIDGET ("nature_page")
#define LESS_PAGE      GET_WIDGET ("less_page")

#define VERSION_LIST   GET_WIDGET ("version_list")
#define VERSION_EDIT   GET_WIDGET ("version_edit")
#define VERSION_LABEL  GET_WIDGET ("version_label")
#define GDB_ANIM       GET_WIDGET ("gdb_anim")
#define APP_FILE       GET_WIDGET ("app_file")
#define CRASHED_PID    GET_WIDGET ("crashed_pid")
#define CORE_FILE      GET_WIDGET ("core_file")
#define STOP_BUTTON    GET_WIDGET ("stop_button")
#define REFRESH_BUTTON GET_WIDGET ("refresh_button")
#define GDB_TEXT       GET_WIDGET ("gdb_text")

#endif /* __GLADE_DRUID_H__ */
