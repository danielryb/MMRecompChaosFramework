#ifndef __CHAOS_H__
#define __CHAOS_H__

#include "util/debug.h"

#include "modding.h"
#include "recomputils.h"
#include "global.h"

#define GAME_CTX_ARG PlayState* play
#define GAME_CTX play

void chaos_init(void);
void chaos_update(GAME_CTX_ARG);

extern bool chaos_is_player_active;

#endif /* __CHAOS_H__ */