#include "chaos.h"

bool chaos_is_player_active = false;

RECOMP_HOOK("Graph_Init") void on_Graph_Init(GraphicsContext* gfxCtx) {
    chaos_init();
}

RECOMP_HOOK("Graph_ExecuteAndDraw") void on_Graph_ExecuteAndDraw(GraphicsContext* gfxCtx, GameState* gameState) {
    if (chaos_is_player_active) {
        PlayState* play = (PlayState*)gameState;
        chaos_update(play);
    }

    chaos_is_player_active = false;
}