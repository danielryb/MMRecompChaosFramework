#include "chaos.h"
#include "tag_names.h"
// #include "ui.h"

RECOMP_HOOK("Graph_Init") void on_Graph_Init(GraphicsContext* gfxCtx) {
    chaos_init();
    // debug_ui_init();
}

void update_cutscene_tag(PlayState* play) {
    static bool prev_in_cutscene = false;

    MessageContext* msgCtx = &play->msgCtx;

    bool in_cutscene = ((play->csCtx.state != CS_STATE_IDLE)
        || (msgCtx->msgMode == MSGMODE_NEW_CYCLE_0)     // song of time
        || (msgCtx->ocarinaMode == OCARINA_MODE_APPLY_DOUBLE_SOT)
        || (msgCtx->ocarinaMode == OCARINA_MODE_WARP)   // song of soaring
        || play->soaringCsOrSoTCsPlaying                //
    );

    if (in_cutscene != prev_in_cutscene) {
        if (in_cutscene) {
            chaos_forbid_tag(CHAOS_TAG_CUTSCENE);
        } else {
            chaos_allow_tag(CHAOS_TAG_CUTSCENE);
        }
        prev_in_cutscene = in_cutscene;
    }
}

void update_player_inactive_tag() {
    static bool prev_is_paused = false;

    bool is_paused = !chaos_is_player_active;

    if (is_paused != prev_is_paused) {
        if (is_paused) {
            chaos_forbid_tag(CHAOS_TAG_PLAYER_INACTIVE);
        } else {
            chaos_allow_tag(CHAOS_TAG_PLAYER_INACTIVE);
        }
        prev_is_paused = is_paused;
    }

    chaos_is_player_active = false;
}

RECOMP_HOOK("Graph_ExecuteAndDraw") void on_Graph_ExecuteAndDraw(GraphicsContext* gfxCtx, GameState* gameState) {
    PlayState* play = (PlayState*)gameState;

    update_cutscene_tag(play);
    update_player_inactive_tag();

    chaos_execute_fun_queues();
    chaos_update(play);

    // debug_ui_update();

}