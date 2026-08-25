// SPDX-License-Identifier: GPL-3.0-only

#include <furi.h>
#include <furi_hal_bt.h>

#include <bt/bt_service/bt.h>
#include <extra_profiles/hid_profile.h>
#include <gui/elements.h>
#include <gui/gui.h>
#include <gui/view.h>
#include <gui/view_dispatcher.h>
#include <furi_hal_usb_hid.h>
#include <storage/storage.h>

#include <mp_240_remote_icons.h>

#include "remote_input_model.h"

#define TAG "240MPRemote"

#define REMOTE_VIEW_ID           0U
#define REMOTE_STATUS_EVENT_BASE 0x100U
#define REMOTE_KEYS_STORAGE_NAME ".bt_240mp.keys"
#define REMOTE_MAC_XOR           0x2400U

typedef struct {
    BtStatus bluetooth_status;
    bool profile_start_failed;
    RemoteInputModel input;
} RemoteModel;

typedef struct {
    Gui* gui;
    Bt* bt;
    ViewDispatcher* view_dispatcher;
    View* view;
    FuriHalBleProfileBase* hid_profile;
} RemoteApp;

static const BleProfileHidParams remote_hid_params = {
    .device_name_prefix = "240MP",
    .mac_xor = REMOTE_MAC_XOR,
};

static void remote_draw_arrow(Canvas* canvas, uint8_t x, uint8_t y, CanvasDirection direction) {
    canvas_draw_triangle(canvas, x, y, 5, 3, direction);

    if(direction == CanvasDirectionBottomToTop) {
        canvas_draw_line(canvas, x, y + 6, x, y - 1);
    } else if(direction == CanvasDirectionTopToBottom) {
        canvas_draw_line(canvas, x, y - 6, x, y + 1);
    } else if(direction == CanvasDirectionRightToLeft) {
        canvas_draw_line(canvas, x + 6, y, x - 1, y);
    } else {
        canvas_draw_line(canvas, x - 6, y, x + 1, y);
    }
}

static void remote_draw_direction_button(
    Canvas* canvas,
    uint8_t button_x,
    uint8_t button_y,
    uint8_t arrow_x,
    uint8_t arrow_y,
    CanvasDirection direction,
    bool pressed) {
    canvas_draw_icon(canvas, button_x, button_y, &I_Button_18x18);
    if(pressed) {
        elements_slightly_rounded_box(canvas, button_x + 3, button_y + 2, 13, 13);
        canvas_set_color(canvas, ColorWhite);
    }

    remote_draw_arrow(canvas, arrow_x, arrow_y, direction);
    canvas_set_color(canvas, ColorBlack);
}

static void remote_draw_action_button(
    Canvas* canvas,
    uint8_t y,
    const Icon* key_icon,
    const char* label,
    bool pressed) {
    canvas_draw_icon(canvas, 2, y, &I_Space_60x18);
    if(pressed) {
        elements_slightly_rounded_box(canvas, 5, y + 2, 55, 13);
        canvas_set_color(canvas, ColorWhite);
    }

    canvas_draw_icon(canvas, 11, y + 4, key_icon);
    elements_multiline_text_aligned(canvas, 26, y + 12, AlignLeft, AlignBottom, label);
    canvas_set_color(canvas, ColorBlack);
}

static void remote_draw_exit_hint(Canvas* canvas) {
    canvas_draw_icon(canvas, 2, 18, &I_Pin_back_arrow_10x8);
    canvas_set_font(canvas, FontSecondary);
    elements_multiline_text_aligned(canvas, 15, 19, AlignLeft, AlignTop, "Hold to exit");
}

static void remote_draw_bt_message(Canvas* canvas, const char* title, const char* instruction) {
    canvas_set_font(canvas, FontPrimary);
    elements_multiline_text_aligned(canvas, 32, 44, AlignCenter, AlignTop, title);
    canvas_set_font(canvas, FontSecondary);
    elements_multiline_text_aligned(canvas, 32, 72, AlignCenter, AlignTop, instruction);
    remote_draw_exit_hint(canvas);
}

static void remote_draw_callback(Canvas* canvas, void* context) {
    furi_assert(context);
    RemoteModel* model = context;

    canvas_clear(canvas);

    if(model->bluetooth_status == BtStatusConnected) {
        canvas_draw_icon(canvas, 0, 0, &I_Ble_connected_15x15);
    } else {
        canvas_draw_icon(canvas, 0, 0, &I_Ble_disconnected_15x15);
    }

    canvas_set_font(canvas, FontPrimary);
    elements_multiline_text_aligned(canvas, 20, 3, AlignLeft, AlignTop, "240-MP");

    if(model->profile_start_failed) {
        remote_draw_bt_message(canvas, "HID start\nfailed", "Check BT,\nthen reopen");
        return;
    } else if(model->bluetooth_status == BtStatusUnavailable) {
        remote_draw_bt_message(canvas, "BT not\navailable", "Restart app");
        return;
    } else if(model->bluetooth_status == BtStatusOff) {
        remote_draw_bt_message(canvas, "Bluetooth\noff", "Enable BT in\nSettings");
        return;
    }

    if(model->input.pairing_reset_confirmation) {
        canvas_set_font(canvas, FontPrimary);
        elements_multiline_text_aligned(canvas, 32, 31, AlignCenter, AlignTop, "Reset?");
        canvas_set_font(canvas, FontSecondary);
        elements_multiline_text_aligned(
            canvas, 32, 51, AlignCenter, AlignTop, "Forget all\npaired hosts?");
        remote_draw_action_button(canvas, 86, &I_Ok_btn_9x9, "Confirm", false);
        remote_draw_action_button(canvas, 107, &I_Pin_back_arrow_10x8, "Cancel", false);
        return;
    }

    remote_draw_exit_hint(canvas);

    const uint8_t buttons = model->input.pressed_buttons;
    const uint8_t x_1 = 2;
    const uint8_t x_2 = 23;
    const uint8_t x_3 = 44;
    const uint8_t y_1 = 44;
    const uint8_t y_2 = 65;

    remote_draw_direction_button(
        canvas,
        x_2,
        y_1,
        x_2 + 9,
        y_1 + 6,
        CanvasDirectionBottomToTop,
        (buttons & RemoteButtonUp) != 0U);
    remote_draw_direction_button(
        canvas,
        x_2,
        y_2,
        x_2 + 9,
        y_2 + 10,
        CanvasDirectionTopToBottom,
        (buttons & RemoteButtonDown) != 0U);
    remote_draw_direction_button(
        canvas,
        x_1,
        y_2,
        x_1 + 7,
        y_2 + 8,
        CanvasDirectionRightToLeft,
        (buttons & RemoteButtonLeft) != 0U);
    remote_draw_direction_button(
        canvas,
        x_3,
        y_2,
        x_3 + 11,
        y_2 + 8,
        CanvasDirectionLeftToRight,
        (buttons & RemoteButtonRight) != 0U);

    remote_draw_action_button(
        canvas, 86, &I_Ok_btn_9x9, "Select", (buttons & RemoteButtonOk) != 0U);
    remote_draw_action_button(
        canvas, 107, &I_Pin_back_arrow_10x8, "Back", (buttons & RemoteButtonBack) != 0U);
}

static RemoteInputKey remote_input_key_from_flipper(InputKey key) {
    switch(key) {
    case InputKeyUp:
        return RemoteInputKeyUp;
    case InputKeyDown:
        return RemoteInputKeyDown;
    case InputKeyLeft:
        return RemoteInputKeyLeft;
    case InputKeyRight:
        return RemoteInputKeyRight;
    case InputKeyOk:
        return RemoteInputKeyOk;
    case InputKeyBack:
        return RemoteInputKeyBack;
    default:
        return RemoteInputKeyNone;
    }
}

static RemoteInputEvent remote_input_event_from_flipper(InputType type) {
    switch(type) {
    case InputTypePress:
        return RemoteInputEventPress;
    case InputTypeRelease:
        return RemoteInputEventRelease;
    case InputTypeShort:
        return RemoteInputEventShort;
    case InputTypeLong:
        return RemoteInputEventLong;
    case InputTypeRepeat:
        return RemoteInputEventRepeat;
    default:
        return RemoteInputEventOther;
    }
}

static uint16_t remote_input_key_to_hid_usage(RemoteInputKey key) {
    switch(key) {
    case RemoteInputKeyUp:
        return HID_KEYBOARD_UP_ARROW;
    case RemoteInputKeyDown:
        return HID_KEYBOARD_DOWN_ARROW;
    case RemoteInputKeyLeft:
        return HID_KEYBOARD_LEFT_ARROW;
    case RemoteInputKeyRight:
        return HID_KEYBOARD_RIGHT_ARROW;
    case RemoteInputKeyOk:
        return HID_KEYBOARD_RETURN;
    case RemoteInputKeyBack:
        return HID_KEYBOARD_ESCAPE;
    case RemoteInputKeyNone:
    default:
        return 0U;
    }
}

static void remote_release_all(RemoteApp* app) {
    if(app->hid_profile) {
        ble_profile_hid_kb_release_all(app->hid_profile);
    }
}

static void remote_disconnect_and_settle(RemoteApp* app) {
    bt_disconnect(app->bt);
    furi_delay_ms(200);
}

static void remote_reset_pairing(RemoteApp* app) {
    furi_assert(app->hid_profile);

    remote_disconnect_and_settle(app);
    furi_hal_bt_stop_advertising();
    bt_forget_bonded_devices(app->bt);
    furi_hal_bt_start_advertising();
}

static bool remote_input_callback(InputEvent* event, void* context) {
    furi_assert(context);
    RemoteApp* app = context;
    bool reset_pairing = false;
    bool exit_app = false;

    with_view_model(
        app->view,
        RemoteModel * model,
        {
            const bool hid_connected = (app->hid_profile != NULL) &&
                                       (model->bluetooth_status == BtStatusConnected);
            const bool pairing_reset_available =
                (app->hid_profile != NULL) && (model->bluetooth_status != BtStatusUnavailable) &&
                (model->bluetooth_status != BtStatusOff);
            const RemoteInputResult result = remote_input_model_update(
                &model->input,
                remote_input_key_from_flipper(event->key),
                remote_input_event_from_flipper(event->type),
                hid_connected,
                pairing_reset_available);
            const uint16_t usage = remote_input_key_to_hid_usage(result.key);
            switch(result.action) {
            case RemoteActionPress:
                furi_check(usage != 0U);
                if(app->hid_profile) {
                    ble_profile_hid_kb_press(app->hid_profile, usage);
                }
                break;
            case RemoteActionRelease:
                furi_check(usage != 0U);
                if(app->hid_profile) {
                    ble_profile_hid_kb_release(app->hid_profile, usage);
                }
                break;
            case RemoteActionReleaseAll:
                remote_release_all(app);
                break;
            case RemoteActionPulse:
                furi_check(usage != 0U);
                if(app->hid_profile) {
                    ble_profile_hid_kb_press(app->hid_profile, usage);
                    ble_profile_hid_kb_release(app->hid_profile, usage);
                }
                break;
            case RemoteActionResetPairing:
                remote_release_all(app);
                reset_pairing = true;
                break;
            case RemoteActionExit:
                remote_release_all(app);
                exit_app = true;
                break;
            case RemoteActionNone:
            default:
                break;
            }
        },
        true);

    if(reset_pairing) {
        remote_reset_pairing(app);
    } else if(exit_app) {
        view_dispatcher_stop(app->view_dispatcher);
    }

    return true;
}

static bool remote_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    RemoteApp* app = context;
    if((event < REMOTE_STATUS_EVENT_BASE) ||
       (event > (REMOTE_STATUS_EVENT_BASE + BtStatusConnected))) {
        return false;
    }

    const BtStatus status = (BtStatus)(event - REMOTE_STATUS_EVENT_BASE);

    if(status != BtStatusConnected) {
        remote_release_all(app);
    }

    with_view_model(
        app->view,
        RemoteModel * model,
        {
            model->bluetooth_status = status;
            if((status == BtStatusUnavailable) || (status == BtStatusOff)) {
                model->input.pairing_reset_confirmation = false;
            }
            if(status != BtStatusConnected) {
                remote_input_model_clear_buttons(&model->input);
            }
        },
        true);

    return true;
}

static void remote_status_changed_callback(BtStatus status, void* context) {
    furi_assert(context);
    RemoteApp* app = context;

    // The Bluetooth service callback may run outside the view-dispatcher context.
    // Defer model and HID state updates to remote_custom_event_callback().
    view_dispatcher_send_custom_event(
        app->view_dispatcher, REMOTE_STATUS_EVENT_BASE + (uint32_t)status);
}

static RemoteApp* remote_app_alloc(void) {
    RemoteApp* app = malloc(sizeof(RemoteApp));
    memset(app, 0, sizeof(RemoteApp));

    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, remote_custom_event_callback);
    app->view = view_alloc();
    view_set_context(app->view, app);
    view_allocate_model(app->view, ViewModelTypeLocking, sizeof(RemoteModel));
    view_set_draw_callback(app->view, remote_draw_callback);
    view_set_input_callback(app->view, remote_input_callback);
    view_set_orientation(app->view, ViewOrientationVerticalFlip);
    view_dispatcher_add_view(app->view_dispatcher, REMOTE_VIEW_ID, app->view);

    with_view_model(
        app->view,
        RemoteModel * model,
        {
            model->bluetooth_status = BtStatusUnavailable;
            model->profile_start_failed = false;
            remote_input_model_reset(&model->input);
        },
        false);

    app->gui = furi_record_open(RECORD_GUI);
    app->bt = furi_record_open(RECORD_BT);
    bt_set_status_changed_callback(app->bt, remote_status_changed_callback, app);

    return app;
}

static void remote_app_free(RemoteApp* app) {
    furi_assert(app);

    // Stop status delivery before tearing down the dispatcher and Bluetooth session.
    bt_set_status_changed_callback(app->bt, NULL, NULL);
    remote_release_all(app);
    remote_disconnect_and_settle(app);

    // End the private HID session before restoring the default bond store and profile.
    bt_keys_storage_set_default_path(app->bt);
    if(!bt_profile_restore_default(app->bt)) {
        FURI_LOG_E(TAG, "Failed to restore default BLE profile");
    }

    view_dispatcher_remove_view(app->view_dispatcher, REMOTE_VIEW_ID);
    view_free(app->view);
    view_dispatcher_free(app->view_dispatcher);

    furi_record_close(RECORD_BT);
    furi_record_close(RECORD_GUI);
    free(app);
}

int32_t mp_240_remote_app(void* context) {
    UNUSED(context);
    RemoteApp* app = remote_app_alloc();

    remote_disconnect_and_settle(app);
    bt_keys_storage_set_storage_path(app->bt, APP_DATA_PATH(REMOTE_KEYS_STORAGE_NAME));

    app->hid_profile =
        bt_profile_start(app->bt, ble_profile_hid, (FuriHalBleProfileParams)&remote_hid_params);

    if(app->hid_profile) {
        furi_hal_bt_start_advertising();
    } else {
        FURI_LOG_E(TAG, "Failed to start BLE HID profile");
        with_view_model(
            app->view, RemoteModel * model, { model->profile_start_failed = true; }, false);
    }

    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_switch_to_view(app->view_dispatcher, REMOTE_VIEW_ID);
    view_dispatcher_run(app->view_dispatcher);

    remote_app_free(app);
    return 0;
}
