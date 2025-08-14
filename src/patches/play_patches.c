#include "chaos.h"

RECOMP_HOOK("Player_Update") void on_Player_Update(Actor* thisx, PlayState* play) {
    chaos_is_player_active = true;
}

RECOMP_HOOK("Player_Destroy") void on_Player_Destroy(Actor* thisx, PlayState* play) {
    chaos_is_player_active = false;
}