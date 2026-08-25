// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    RemoteButtonUp = (1U << 0),
    RemoteButtonDown = (1U << 1),
    RemoteButtonLeft = (1U << 2),
    RemoteButtonRight = (1U << 3),
    RemoteButtonOk = (1U << 4),
    RemoteButtonBack = (1U << 5),
} RemoteButton;

typedef enum {
    RemoteInputKeyNone,
    RemoteInputKeyUp,
    RemoteInputKeyDown,
    RemoteInputKeyLeft,
    RemoteInputKeyRight,
    RemoteInputKeyOk,
    RemoteInputKeyBack,
} RemoteInputKey;

typedef enum {
    RemoteInputEventOther,
    RemoteInputEventPress,
    RemoteInputEventRelease,
    RemoteInputEventShort,
    RemoteInputEventLong,
    RemoteInputEventRepeat,
} RemoteInputEvent;

typedef enum {
    RemoteActionNone,
    RemoteActionPress,
    RemoteActionRelease,
    RemoteActionReleaseAll,
    RemoteActionPulse,
    RemoteActionResetPairing,
    RemoteActionExit,
} RemoteAction;

typedef struct {
    uint8_t pressed_buttons;
    uint8_t held_direction_buttons;
    bool pairing_reset_confirmation;
} RemoteInputModel;

typedef struct {
    RemoteAction action;
    RemoteInputKey key;
} RemoteInputResult;

void remote_input_model_reset(RemoteInputModel* model);
void remote_input_model_clear_buttons(RemoteInputModel* model);

RemoteInputResult remote_input_model_update(
    RemoteInputModel* model,
    RemoteInputKey key,
    RemoteInputEvent event,
    bool connected,
    bool pairing_reset_available);
