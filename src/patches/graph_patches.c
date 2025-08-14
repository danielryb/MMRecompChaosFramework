#include "chaos.h"

bool chaos_is_player_active = false;

RECOMP_HOOK("Graph_Init") void on_Graph_Init(GraphicsContext* gfxCtx) {
    chaos_init();
}

RECOMP_HOOK("Graph_ExecuteAndDraw") void on_Graph_ExecuteAndDraw(GraphicsContext* gfxCtx, GameState* gameState) {
    if (chaos_is_player_active) {
        PlayState* GAME_CTX = (PlayState*)gameState;
        chaos_update(GAME_CTX);
    }

    chaos_is_player_active = false;
}