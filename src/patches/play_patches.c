#include "chaos.h"

RECOMP_HOOK("Player_Update") void on_Player_Update(Actor* thisx, PlayState* play) {
    chaos_is_player_active = true;
}