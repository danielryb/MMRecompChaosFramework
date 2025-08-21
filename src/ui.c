#include "chaos.h"
#include "recompui.h"
#include "string.h"

#ifdef DEBUG

RecompuiContext ui_context;
typedef struct {
    RecompuiResource root;
    RecompuiResource container;
} UiFrame;
UiFrame chaos_frame = {0, 0};
static bool initialized_ui = false;
static bool ui_open = false;
PlayState* cur_play_state = NULL;

const float modal_border_width = 1.1f;
const float modal_border_radius = 16.0f;

void create_container(RecompuiContext context, UiFrame* frame);
void createUiFrame(RecompuiContext context, UiFrame* frame) {
    RecompuiColor bg_color;
    bg_color.r = 255;
    bg_color.g = 255;
    bg_color.b = 255;
    bg_color.a = 0.1f * 255;


    frame->root = recompui_context_root(context);
    // Set up the root element so it takes up the full screen.
    recompui_set_position(frame->root, POSITION_ABSOLUTE);
    recompui_set_top(frame->root, 0, UNIT_DP);
    recompui_set_right(frame->root, 0, UNIT_DP);
    recompui_set_bottom(frame->root, 0, UNIT_DP);
    recompui_set_left(frame->root, 0, UNIT_DP);
    recompui_set_width_auto(frame->root);
    recompui_set_height_auto(frame->root);

    // Set up the root element's background color so the modal contents don't touch the screen edges.
    recompui_set_background_color(frame->root, &bg_color);

    // Set up the flexbox properties of the root element.
    recompui_set_flex_direction(frame->root, FLEX_DIRECTION_COLUMN);
    recompui_set_justify_content(frame->root, JUSTIFY_CONTENT_CENTER);
    recompui_set_align_items(frame->root, ALIGN_ITEMS_CENTER);
}

void create_container(RecompuiContext context, UiFrame* frame) {
    RecompuiColor border_color;
    border_color.r = 255;
    border_color.g = 255;
    border_color.b = 255;
    border_color.a = 0.7f * 255;

    RecompuiColor modal_color;
    modal_color.r = 8;
    modal_color.g = 7;
    modal_color.b = 13;
    modal_color.a = 0.5f * 255;

    // Create a container to act as the modal background and hold the elements in the modal.
    frame->container = recompui_create_element(context, frame->root);

    // Set the container's size to grow based on the child elements.
    recompui_set_flex_grow(frame->container, 0.0f);
    recompui_set_flex_shrink(frame->container, 0.0f);
    recompui_set_width_auto(frame->container);
    recompui_set_height_auto(frame->container);

    // Set up the properties of the container.
    recompui_set_display(frame->container, DISPLAY_BLOCK);
    recompui_set_position(frame->container, POSITION_ABSOLUTE);
    recompui_set_padding(frame->container, 16.0f, UNIT_DP);
    recompui_set_align_items(frame->container, ALIGN_ITEMS_STRETCH);
    
    // Set up the container to be the modal's background.
    recompui_set_border_width(frame->container, modal_border_width, UNIT_DP);
    recompui_set_border_radius(frame->container, modal_border_radius, UNIT_DP);
    recompui_set_border_color(frame->container, &border_color);
    recompui_set_background_color(frame->container, &modal_color);

    recompui_set_top(frame->container, (RECOMPUI_TOTAL_HEIGHT - 800.0f) / 2.0f, UNIT_DP);
    recompui_set_left(frame->container, 16, UNIT_DP);

    // Adjust the container's properties.
    recompui_set_width(frame->container, 560.0f, UNIT_DP);
    recompui_set_height(frame->container, 800.0f, UNIT_DP);
    recompui_set_max_height(frame->container, 800.0f, UNIT_DP);
    recompui_set_display(frame->container, DISPLAY_FLEX);
    recompui_set_flex_direction(frame->container, FLEX_DIRECTION_COLUMN);
    recompui_set_align_items(frame->container, ALIGN_ITEMS_BASELINE);
    recompui_set_justify_content(frame->container, JUSTIFY_CONTENT_FLEX_START);

    // Remove the padding on the frame's container so that the divider line has the full width of the container.
    recompui_set_padding(frame->container, 0.0f, UNIT_DP);

    recompui_set_overflow_y(frame->container, OVERFLOW_AUTO);
}

typedef struct EffectRow {
    RecompuiResource container;
    RecompuiResource label;
    RecompuiResource button;
    RecompuiResource time_label;
} EffectRow;

typedef struct ChaosClickContext {
    ChaosEffectEntity* effect;
    ChaosGroup* group;
    ChaosMachine* machine;
    EffectRow row;
} ChaosClickContext;

ChaosClickContext *all_contexts = NULL;

u32 get_num_effects(void) {
    u32 num_effects = 0;

    for (u32 i = 0; i < machine_count; i++) {
        ChaosMachine* machine = &machines[i];
        for (int j = 0; j < CHAOS_DISTURBANCE_MAX; j++) {
            ChaosGroup* group = &machine->groups[j];
            num_effects += group->effect_count;
        }
    }

    return num_effects;
}

void alloc_all_machines(void) {
    u32 num_effects = get_num_effects();
    all_contexts = (ChaosClickContext *)recomp_alloc(sizeof(ChaosClickContext) * num_effects);
}
void free_all_machines(void) {
    if (all_contexts != NULL) {
        recomp_free(all_contexts);
        all_contexts = NULL;
    }
}

void handle_invoke_effect(RecompuiResource resource, const RecompuiEventData* event, void* userdata) {
    if (event->type == UI_EVENT_CLICK) {
        ChaosClickContext* context = (ChaosClickContext*)userdata;
        ChaosEffectEntity* effect = context->effect;
        if (effect->status == CHAOS_EFFECT_STATUS_AVAILABLE) {
            active_list_add(context->machine, context->group, context->effect, cur_play_state);
        }
    }
}

void render_chaos_effect(ChaosEffectEntity* effect, ChaosGroup* group, ChaosMachine *machine, ChaosClickContext *click_context) {
    static const f32 button_size = 40.0f;
    static const f32 button_text_size = 18.0f;


    click_context->effect = effect;
    click_context->group = group;
    click_context->machine = machine;

    click_context->row.container = recompui_create_element(ui_context, chaos_frame.container);
    recompui_set_width(click_context->row.container, 100, UNIT_PERCENT);
    recompui_set_display(click_context->row.container, DISPLAY_FLEX);
    recompui_set_flex_direction(click_context->row.container, FLEX_DIRECTION_ROW);
    recompui_set_align_items(click_context->row.container, ALIGN_ITEMS_CENTER);
    recompui_set_justify_content(click_context->row.container, JUSTIFY_CONTENT_FLEX_START);
    recompui_set_height(click_context->row.container, 56, UNIT_DP);
    recompui_set_min_height(click_context->row.container, 56, UNIT_DP);
    recompui_set_margin(click_context->row.container, 4.0f, UNIT_DP);
    recompui_set_margin_left(click_context->row.container, 0.0f, UNIT_DP);
    recompui_set_margin_right(click_context->row.container, 0.0f, UNIT_DP);
    recompui_set_padding(click_context->row.container, 4.0f, UNIT_DP);
    recompui_set_padding_left(click_context->row.container, 20.0f, UNIT_DP);
    recompui_set_padding_right(click_context->row.container, 20.0f, UNIT_DP);
    recompui_set_border_bottom_width(click_context->row.container, 1.1f, UNIT_DP);
    recompui_set_border_bottom_color(click_context->row.container, &(RecompuiColor){ 255, 255, 255, 255/10 });

    {
        click_context->row.label = recompui_create_label(ui_context, click_context->row.container, effect->effect.name, LABELSTYLE_SMALL);
        {
            recompui_set_width(click_context->row.label, 280, UNIT_DP);
            recompui_set_max_width(click_context->row.label, 280, UNIT_DP);
            recompui_set_overflow_x(click_context->row.label, OVERFLOW_HIDDEN);
        }
    
        click_context->row.button = recompui_create_button(
            ui_context,
            click_context->row.container,
            "",
            effect->status == CHAOS_EFFECT_STATUS_ACTIVE ? BUTTONSTYLE_PRIMARY : BUTTONSTYLE_SECONDARY
        );
        {
            recompui_set_width(click_context->row.button, button_size, UNIT_DP);
            recompui_set_height(click_context->row.button, button_size, UNIT_DP);
            recompui_set_border_radius(click_context->row.button, button_size / 2.0f, UNIT_DP);
            recompui_set_font_size(click_context->row.button, button_text_size, UNIT_DP);
            recompui_set_line_height(click_context->row.button, button_text_size, UNIT_DP);
            recompui_set_text_align(click_context->row.button, TEXT_ALIGN_CENTER);
            recompui_set_padding(click_context->row.button, (button_size - button_text_size) / 2.0f, UNIT_DP);
    
            recompui_register_callback(click_context->row.button, handle_invoke_effect, click_context);
        }

        click_context->row.time_label = recompui_create_label(ui_context, click_context->row.container, "(N/A)", LABELSTYLE_ANNOTATION);
        {
            recompui_set_margin_left(click_context->row.time_label, 16.0f, UNIT_DP);
            recompui_set_text_align(click_context->row.time_label, TEXT_ALIGN_RIGHT);
        }
    }

}

void render_chaos_group(ChaosGroup* group, ChaosMachine *machine, ChaosClickContext **click_context) {
    for (u32 i = 0; i < group->effect_count; i++) {
        ChaosEffectEntity* effect = &group->effects[i];
        render_chaos_effect(effect, group, machine, *click_context);
        (*click_context)++;
    }
}

const RecompuiColor label_color = { 185, 125, 242, 255 };
void render_chaos_machine(ChaosMachine *machine, ChaosClickContext **click_context) {
    for (u32 i = 0; i < CHAOS_DISTURBANCE_MAX; i++) {
        ChaosGroup* group = &machine->groups[i];
        if (group->effect_count > 0) {
            RecompuiResource label = recompui_create_label(ui_context, chaos_frame.container, DISTURBANCE_NAME[i], LABELSTYLE_SMALL);
            recompui_set_color(label, &label_color);
            recompui_set_margin_top(label, 24.0f, UNIT_DP);
            recompui_set_margin_left(label, 0, UNIT_DP);
            recompui_set_margin_bottom(label, 12.0f, UNIT_DP);
            render_chaos_group(group, machine, click_context);
        }
    }
}

void render_chaos_machines(void) {
    free_all_machines();
    alloc_all_machines();
    create_container(ui_context, &chaos_frame);
    ChaosClickContext *original_context = all_contexts;
    ChaosClickContext **click_context = &all_contexts;
    for (u32 i = 0; i < machine_count; i++) {
        ChaosMachine* machine = &machines[i];
        render_chaos_machine(machine, click_context);
    }
    all_contexts = original_context;
}

RecompuiResource fab = 0;

static const RecompuiColor fab_text_color = { 255, 255, 255, 255 };
static const RecompuiColor fab_color = { 255, 255, 255, 255/4 };
static const RecompuiColor fab_color__hover = { 255, 255, 255, 255/3 };
bool queue_toggle_ui = false;

void handle_fab_events(RecompuiResource resource, const RecompuiEventData* event, void* userdata) {
    switch (event->type) {
        case UI_EVENT_CLICK:
            queue_toggle_ui = true;
            break;
        case UI_EVENT_HOVER:
            recompui_set_background_color(fab, event->data.hover.active ? &fab_color__hover : &fab_color);
            recompui_set_opacity(fab, event->data.hover.active ? 1.0f : 0.5f);
            break;
        default:
            break;
    }
}

void render_fab(void) {
    static const f32 size = 56.0f;
    static const f32 text_size = 24.0f;
    fab = recompui_create_button(ui_context, chaos_frame.root, "•••", BUTTONSTYLE_SECONDARY);
    recompui_set_position(fab, POSITION_ABSOLUTE);
    recompui_set_border_width(fab, 0, UNIT_DP);
    recompui_set_color(fab, &fab_text_color);
    recompui_set_padding_top(fab, (size - text_size) / 2.0f, UNIT_DP);
    recompui_set_padding_bottom(fab, (size - text_size) / 2.0f, UNIT_DP);
    recompui_set_padding_left(fab, 0.0f, UNIT_DP);
    recompui_set_padding_right(fab, 0.0f, UNIT_DP);
    recompui_set_font_size(fab, text_size, UNIT_DP);
    recompui_set_line_height(fab, text_size, UNIT_DP);
    recompui_set_text_align(fab, TEXT_ALIGN_CENTER);
    recompui_set_background_color(fab, &fab_color);
    recompui_set_border_radius(fab, size / 2.0f, UNIT_DP);
    recompui_set_opacity(fab, 0.5f);
    recompui_set_right(fab, 16.0f, UNIT_DP);
    recompui_set_bottom(fab, 16.0f, UNIT_DP);
    recompui_set_width(fab, size, UNIT_DP);
    recompui_set_height(fab, size, UNIT_DP);
    recompui_register_callback(fab, handle_fab_events, NULL);
}

void toggle_ui(void);


void init_ui() {
    if (!initialized_ui) {
        ui_context = recompui_create_context();
        recompui_open_context(ui_context);
        recompui_set_context_captures_input(ui_context, false);
        createUiFrame(ui_context, &chaos_frame);
        render_fab();
        recompui_close_context(ui_context);
        recompui_show_context(ui_context);
        initialized_ui = true;
    }
}

void toggle_ui(void) {
    ui_open = !ui_open;

    if (ui_open) {
        recompui_open_context(ui_context);
        render_chaos_machines();
        recompui_close_context(ui_context);
    } else {
        recompui_open_context(ui_context);
        if (chaos_frame.container != 0) {
            recompui_destroy_element(chaos_frame.root, chaos_frame.container);
            chaos_frame.container = 0;
        }
        recompui_close_context(ui_context);
    }
}


u32 get_time_left_in_effect(ChaosEffectEntity* effect, ChaosMachine* machine) {
    if (effect->status != CHAOS_EFFECT_STATUS_ACTIVE) {
        return 0;
    }
    for (ActiveChaosEffectList* cur = machine->active_effects; cur != NULL; cur = cur->next) {
        if (cur->effect == effect) {
            return cur->timer;
        }
    }
    return 0;
}

int sprintf(char* dst, const char* fmt, ...);
void update_effect_buttons(void) {
    static const RecompuiColor active_color = { 0, 255, 0, 255 };
    static const RecompuiColor inactive_color = { 0, 255, 255, 255 };

    if (!ui_open || all_contexts == NULL) {
        return;
    }
    u32 num_effects = get_num_effects();
    recompui_open_context(ui_context);

    char buf[0x100];
    for (u32 i = 0; i < num_effects; i++) {
        ChaosClickContext* context = &all_contexts[i];
        ChaosEffectEntity* effect = context->effect;
        if (effect->status == CHAOS_EFFECT_STATUS_ACTIVE) {
            recompui_set_border_color(context->row.button, &active_color);
            recompui_set_color(context->row.time_label, &active_color);
        } else {
            recompui_set_border_color(context->row.button, &inactive_color);
            recompui_set_color(context->row.time_label, &inactive_color);
        }
        sprintf(buf, "(%04lu / %04lu)", get_time_left_in_effect(effect, context->machine), effect->effect.duration);
        recompui_set_text(context->row.time_label, buf);
    }

    recompui_close_context(ui_context);
}

RECOMP_HOOK("Graph_ExecuteAndDraw") void on_check_toggle_ui(GraphicsContext* gfxCtx, GameState* gameState) {
    if (chaos_is_player_active) {
        PlayState* play = (PlayState*)gameState;
        cur_play_state = play;
        Input *input = CONTROLLER1(&play->state);
        init_ui();
        if (queue_toggle_ui) {
            toggle_ui();
            queue_toggle_ui = false;
        } else {
            update_effect_buttons();
        }
    }
}


#endif
