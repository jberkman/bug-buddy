#ifndef __CORE_BUDDY_H__
#define __CORE_BUDDY_H__

void get_trace_from_core (GnomeDruid *druid, GtkText *text, gchar *core_file);
void get_trace_from_pair (GnomeDruid *druid, GtkText *text, const gchar *app, 
			  const gchar *extra);

#endif /*__CORE_BUDDY_H__*/
