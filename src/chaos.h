#ifndef __CHAOS_H__
#define __CHAOS_H__

#include "util/debug.h"

#include "modding.h"
#include "recomputils.h"
#include "global.h"

void chaos_init(void);
void chaos_update(GraphicsContext* gfxCtx, GameState* gameState);

extern bool chaos_is_player_active;

#endif /* __CHAOS_H__ */