#ifndef __UI_H__
#define __UI_H__

#ifdef __cplusplus
namespace Chaos {
extern "C" {
#endif

#ifdef DEBUG

void debug_ui_init(void);
void debug_ui_update(void);

#else

#define debug_ui_init() /* NULL */
#define debug_ui_update() /* NULL */

#endif

#ifdef __cplusplus
} /* extern "C" */
} /* namespace Chaos */
#endif

#endif /* __UI_H__ */