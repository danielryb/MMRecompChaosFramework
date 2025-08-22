#ifndef __UI_H__
#define __UI_H__

#ifdef DEBUG

void debug_ui_init(void);
void debug_ui_update(void);

#else

#define debug_ui_init() /* NULL */
#define debug_ui_update() /* NULL */

#endif

#endif /* __UI_H__ */