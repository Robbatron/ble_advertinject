#include "ble_advertinject.h"
#include <gui/gui.h>
#include <furi_hal_bt.h>
#include <extra_beacon.h>
#include <gui/elements.h>
#include <gui/view_dispatcher.h>
#include "protocols/_protocols.h"

static Attack attacks[] = {
    {
        .title = "BLE AdvertInject",
        .text = "Broadcast a secret",
        .protocol = &protocol_nameflood,
        .payload =
            {
                .random_mac = true,
                .cfg.nameflood = {},
            },
    },
};

typedef struct {
    Ctx ctx;
    View* main_view;
    bool advertising;
    GapExtraBeaconConfig config;
    FuriThread* thread;
    int8_t index;
} State;

static void randomize_mac(State* state) {
    furi_hal_random_fill_buf(state->config.address, sizeof(state->config.address));
}

static void start_extra_beacon(State* state) {
    uint8_t size;
    uint8_t* packet;
    GapExtraBeaconConfig* config = &state->config;
    Payload* payload = &attacks[state->index].payload;
    const Protocol* protocol = attacks[state->index].protocol;

    config->min_adv_interval_ms = 100;
    config->max_adv_interval_ms = 150;
    if(payload->random_mac) randomize_mac(state);
    furi_check(furi_hal_bt_extra_beacon_set_config(config));

    protocol->make_packet(&size, &packet, payload);
    furi_check(furi_hal_bt_extra_beacon_set_data(packet, size));
    free(packet);

    furi_check(furi_hal_bt_extra_beacon_start());
}

static int32_t adv_thread(void* _ctx) {
    State* state = _ctx;
    Payload* payload = &attacks[state->index].payload;
    if(!payload->random_mac) randomize_mac(state);
    
    if(furi_hal_bt_extra_beacon_is_active()) {
        furi_check(furi_hal_bt_extra_beacon_stop());
    }

    start_extra_beacon(state);
    
    while(state->advertising) {
        furi_delay_ms(100);
    }
    
    furi_check(furi_hal_bt_extra_beacon_stop());
    return 0;
}

static void toggle_adv(State* state) {
    if(state->advertising) {
        state->advertising = false;
        furi_thread_flags_set(furi_thread_get_id(state->thread), true);
        furi_thread_join(state->thread);
    } else {
        state->advertising = true;
        furi_thread_start(state->thread);
    }
}

static void text_input_callback(void* context) {
    State* state = context;
    Payload* payload = &attacks[state->index].payload;
    payload->mode = PayloadModeValue;
    view_dispatcher_switch_to_view(state->ctx.view_dispatcher, ViewMain);
}

static void draw_callback(Canvas* canvas, void* _ctx) {
    State* state = *(State**)_ctx;
    const Attack* attack = &attacks[state->index];

    canvas_set_font(canvas, FontSecondary);
    const Icon* icon = attack->protocol ? attack->protocol->icon : &I_ble_spam;
    canvas_draw_icon(canvas, 4 - (icon == &I_ble_spam), 3, icon);
    canvas_draw_str(canvas, 14, 12, "BLE AdvertInject");

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 4, 33, attack->title);

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 4, 46, attack->text);

    elements_button_center(canvas, state->advertising ? "Stop" : "Start");
    elements_button_right(canvas, "Msg");
}

static bool input_callback(InputEvent* input, void* _ctx) {
    View* view = _ctx;
    State* state = *(State**)view_get_model(view);
    bool consumed = false;

    if(input->type == InputTypeShort || input->type == InputTypeLong) {
        consumed = true;
        switch(input->key) {
        case InputKeyOk:
            toggle_adv(state);
            break;
        case InputKeyRight:
            text_input_set_header_text(state->ctx.text_input, "Enter custom message");
            text_input_set_result_callback(
                state->ctx.text_input,
                text_input_callback,
                state,
                attacks[state->index].payload.cfg.nameflood.name,
                20,
                true);
            view_dispatcher_switch_to_view(state->ctx.view_dispatcher, ViewTextInput);
            break;
        case InputKeyBack:
            if(state->advertising) toggle_adv(state);
            consumed = false;
            break;
        default:
            break;
        }
    }

    view_commit_model(view, consumed);
    return consumed;
}

static bool back_event_callback(void* _ctx) {
    State* state = _ctx;
    if(state->advertising) {
        toggle_adv(state);
    }
    return false;  // This will allow the app to exit
}

int32_t ble_advertinject(void* p) {
    UNUSED(p);
    State* state = malloc(sizeof(State));
    state->config.adv_channel_map = GapAdvChannelMapAll;
    state->config.adv_power_level = GapAdvPowerLevel_6dBm;
    state->config.address_type = GapAddressTypePublic;
    state->thread = furi_thread_alloc();
    furi_thread_set_callback(state->thread, adv_thread);
    furi_thread_set_context(state->thread, state);
    furi_thread_set_stack_size(state->thread, 2048);

    state->ctx.notification = furi_record_open(RECORD_NOTIFICATION);
    Gui* gui = furi_record_open(RECORD_GUI);
    state->ctx.view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(state->ctx.view_dispatcher);
    view_dispatcher_set_event_callback_context(state->ctx.view_dispatcher, state);
    view_dispatcher_set_navigation_event_callback(state->ctx.view_dispatcher, back_event_callback);

    state->main_view = view_alloc();
    view_allocate_model(state->main_view, ViewModelTypeLocking, sizeof(State*));
    with_view_model(state->main_view, State * *model, { *model = state; }, false);
    view_set_context(state->main_view, state->main_view);
    view_set_draw_callback(state->main_view, draw_callback);
    view_set_input_callback(state->main_view, input_callback);
    view_dispatcher_add_view(state->ctx.view_dispatcher, ViewMain, state->main_view);

    state->ctx.text_input = text_input_alloc();
    view_dispatcher_add_view(
        state->ctx.view_dispatcher, ViewTextInput, text_input_get_view(state->ctx.text_input));

    view_dispatcher_attach_to_gui(state->ctx.view_dispatcher, gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_switch_to_view(state->ctx.view_dispatcher, ViewMain);

    view_dispatcher_run(state->ctx.view_dispatcher);

    view_dispatcher_remove_view(state->ctx.view_dispatcher, ViewTextInput);
    text_input_free(state->ctx.text_input);

    view_dispatcher_remove_view(state->ctx.view_dispatcher, ViewMain);
    view_free(state->main_view);

    view_dispatcher_free(state->ctx.view_dispatcher);
    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_NOTIFICATION);

    furi_thread_free(state->thread);
    free(state);

    return 0;
}
