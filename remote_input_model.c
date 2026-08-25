// SPDX-License-Identifier: GPL-3.0-only

#include "remote_input_model.h"

static RemoteButton remote_input_direction_button(RemoteInputKey key) {
    switch(key) {
    case RemoteInputKeyUp:
        return RemoteButtonUp;
    case RemoteInputKeyDown:
        return RemoteButtonDown;
    case RemoteInputKeyLeft:
        return RemoteButtonLeft;
    case RemoteInputKeyRight:
        return RemoteButtonRight;
    default:
        return 0U;
    }
}

void remote_input_model_clear_buttons(RemoteInputModel* model) {
    model->pressed_buttons = 0U;
    model->held_direction_buttons = 0U;
}

void remote_input_model_reset(RemoteInputModel* model) {
    remote_input_model_clear_buttons(model);
    model->pairing_reset_confirmation = false;
}

RemoteInputResult remote_input_model_update(
    RemoteInputModel* model,
    RemoteInputKey key,
    RemoteInputEvent event,
    bool connected,
    bool pairing_reset_available) {
    RemoteInputResult result = {.action = RemoteActionNone, .key = RemoteInputKeyNone};

    if((key == RemoteInputKeyBack) && (event == RemoteInputEventLong)) {
        remote_input_model_reset(model);
        result.action = RemoteActionExit;
        return result;
    }

    if(model->pairing_reset_confirmation) {
        if((key == RemoteInputKeyOk) && (event == RemoteInputEventPress)) {
            remote_input_model_reset(model);
            result.action = RemoteActionResetPairing;
        } else if((key == RemoteInputKeyBack) && (event == RemoteInputEventShort)) {
            remote_input_model_reset(model);
        } else if((key == RemoteInputKeyOk) && (event == RemoteInputEventRelease)) {
            model->pressed_buttons &= (uint8_t)~RemoteButtonOk;
        } else if((key == RemoteInputKeyBack) && (event == RemoteInputEventRelease)) {
            model->pressed_buttons &= (uint8_t)~RemoteButtonBack;
        }
        return result;
    }

    const RemoteButton direction_button = remote_input_direction_button(key);
    if(direction_button != 0U) {
        if(event == RemoteInputEventPress) {
            model->pressed_buttons |= direction_button;
            if(connected) {
                model->held_direction_buttons |= direction_button;
                result.action = RemoteActionPress;
                result.key = key;
            }
        } else if(event == RemoteInputEventRelease) {
            model->pressed_buttons &= (uint8_t)~direction_button;
            if((model->held_direction_buttons & direction_button) != 0U) {
                model->held_direction_buttons &= (uint8_t)~direction_button;
                result.action = RemoteActionRelease;
                result.key = key;
            }
        }
        return result;
    }

    if(key == RemoteInputKeyOk) {
        if(event == RemoteInputEventPress) {
            model->pressed_buttons |= RemoteButtonOk;
        } else if(event == RemoteInputEventRelease) {
            model->pressed_buttons &= (uint8_t)~RemoteButtonOk;
        } else if((event == RemoteInputEventShort) && connected) {
            result.action = RemoteActionPulse;
            result.key = key;
        } else if((event == RemoteInputEventLong) && pairing_reset_available) {
            remote_input_model_clear_buttons(model);
            model->pairing_reset_confirmation = true;
            result.action = RemoteActionReleaseAll;
        }
        return result;
    }

    if(key == RemoteInputKeyBack) {
        if(event == RemoteInputEventPress) {
            model->pressed_buttons |= RemoteButtonBack;
        } else if(event == RemoteInputEventRelease) {
            model->pressed_buttons &= (uint8_t)~RemoteButtonBack;
        } else if((event == RemoteInputEventShort) && connected) {
            result.action = RemoteActionPulse;
            result.key = key;
        }
    }

    return result;
}
