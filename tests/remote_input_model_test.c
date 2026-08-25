// SPDX-License-Identifier: GPL-3.0-only

#include "remote_input_model.h"

#include <assert.h>
#include <stdio.h>

static void expect_result(
    RemoteInputResult result,
    RemoteAction expected_action,
    RemoteInputKey expected_key) {
    assert(result.action == expected_action);
    assert(result.key == expected_key);
}

static void test_disconnected_direction_is_visual_only(void) {
    RemoteInputModel model;
    remote_input_model_reset(&model);

    expect_result(
        remote_input_model_update(&model, RemoteInputKeyUp, RemoteInputEventPress, false, true),
        RemoteActionNone,
        RemoteInputKeyNone);
    assert(model.pressed_buttons == RemoteButtonUp);
    assert(model.held_direction_buttons == 0U);

    expect_result(
        remote_input_model_update(&model, RemoteInputKeyUp, RemoteInputEventRelease, false, true),
        RemoteActionNone,
        RemoteInputKeyNone);
    assert(model.pressed_buttons == 0U);
    assert(model.held_direction_buttons == 0U);
}

static void test_connected_direction_tracks_hid_hold(void) {
    RemoteInputModel model;
    remote_input_model_reset(&model);

    expect_result(
        remote_input_model_update(&model, RemoteInputKeyLeft, RemoteInputEventPress, true, false),
        RemoteActionPress,
        RemoteInputKeyLeft);
    assert(model.pressed_buttons == RemoteButtonLeft);
    assert(model.held_direction_buttons == RemoteButtonLeft);

    expect_result(
        remote_input_model_update(&model, RemoteInputKeyLeft, RemoteInputEventRepeat, true, false),
        RemoteActionNone,
        RemoteInputKeyNone);

    expect_result(
        remote_input_model_update(&model, RemoteInputKeyLeft, RemoteInputEventRelease, true, false),
        RemoteActionRelease,
        RemoteInputKeyLeft);
    assert(model.pressed_buttons == 0U);
    assert(model.held_direction_buttons == 0U);
}

static void test_reconnect_does_not_create_unmatched_release(void) {
    RemoteInputModel model;
    remote_input_model_reset(&model);

    remote_input_model_update(&model, RemoteInputKeyRight, RemoteInputEventPress, false, true);
    expect_result(
        remote_input_model_update(
            &model, RemoteInputKeyRight, RemoteInputEventRelease, true, false),
        RemoteActionNone,
        RemoteInputKeyNone);
}

static void test_disconnect_clears_direction_hold(void) {
    RemoteInputModel model;
    remote_input_model_reset(&model);

    remote_input_model_update(&model, RemoteInputKeyDown, RemoteInputEventPress, true, false);
    remote_input_model_clear_buttons(&model);
    expect_result(
        remote_input_model_update(&model, RemoteInputKeyDown, RemoteInputEventRelease, false, true),
        RemoteActionNone,
        RemoteInputKeyNone);
}

static void test_ok_pulses_once_per_short_press(void) {
    RemoteInputModel model;
    remote_input_model_reset(&model);

    expect_result(
        remote_input_model_update(&model, RemoteInputKeyOk, RemoteInputEventPress, true, false),
        RemoteActionNone,
        RemoteInputKeyNone);
    assert((model.pressed_buttons & RemoteButtonOk) != 0U);
    expect_result(
        remote_input_model_update(&model, RemoteInputKeyOk, RemoteInputEventShort, true, false),
        RemoteActionPulse,
        RemoteInputKeyOk);
    expect_result(
        remote_input_model_update(&model, RemoteInputKeyOk, RemoteInputEventRepeat, true, false),
        RemoteActionNone,
        RemoteInputKeyNone);
    expect_result(
        remote_input_model_update(&model, RemoteInputKeyOk, RemoteInputEventRelease, true, false),
        RemoteActionNone,
        RemoteInputKeyNone);
    assert((model.pressed_buttons & RemoteButtonOk) == 0U);

    remote_input_model_update(&model, RemoteInputKeyOk, RemoteInputEventPress, true, true);
    expect_result(
        remote_input_model_update(&model, RemoteInputKeyOk, RemoteInputEventLong, true, true),
        RemoteActionReleaseAll,
        RemoteInputKeyNone);
    assert(model.pairing_reset_confirmation);
    assert(model.pressed_buttons == 0U);
    remote_input_model_update(&model, RemoteInputKeyOk, RemoteInputEventRelease, true, true);
    assert((model.pressed_buttons & RemoteButtonOk) == 0U);
    expect_result(
        remote_input_model_update(&model, RemoteInputKeyOk, RemoteInputEventPress, true, true),
        RemoteActionResetPairing,
        RemoteInputKeyNone);

    remote_input_model_update(&model, RemoteInputKeyOk, RemoteInputEventPress, false, true);
    expect_result(
        remote_input_model_update(&model, RemoteInputKeyOk, RemoteInputEventShort, false, true),
        RemoteActionNone,
        RemoteInputKeyNone);
    remote_input_model_update(&model, RemoteInputKeyOk, RemoteInputEventRelease, false, true);
}

static void test_pairing_reset_releases_connected_direction_before_cancel(void) {
    RemoteInputModel model;
    remote_input_model_reset(&model);

    expect_result(
        remote_input_model_update(&model, RemoteInputKeyUp, RemoteInputEventPress, true, true),
        RemoteActionPress,
        RemoteInputKeyUp);
    remote_input_model_update(&model, RemoteInputKeyOk, RemoteInputEventPress, true, true);
    expect_result(
        remote_input_model_update(&model, RemoteInputKeyOk, RemoteInputEventLong, true, true),
        RemoteActionReleaseAll,
        RemoteInputKeyNone);
    assert(model.pairing_reset_confirmation);
    assert(model.pressed_buttons == 0U);
    assert(model.held_direction_buttons == 0U);

    expect_result(
        remote_input_model_update(&model, RemoteInputKeyUp, RemoteInputEventRelease, true, true),
        RemoteActionNone,
        RemoteInputKeyNone);
    expect_result(
        remote_input_model_update(&model, RemoteInputKeyBack, RemoteInputEventShort, true, true),
        RemoteActionNone,
        RemoteInputKeyNone);
    assert(!model.pairing_reset_confirmation);
    assert(model.held_direction_buttons == 0U);
}

static void test_back_short_and_long_are_distinct(void) {
    RemoteInputModel model;
    remote_input_model_reset(&model);

    remote_input_model_update(&model, RemoteInputKeyBack, RemoteInputEventPress, true, false);
    expect_result(
        remote_input_model_update(&model, RemoteInputKeyBack, RemoteInputEventShort, true, false),
        RemoteActionPulse,
        RemoteInputKeyBack);
    expect_result(
        remote_input_model_update(&model, RemoteInputKeyBack, RemoteInputEventRelease, true, false),
        RemoteActionNone,
        RemoteInputKeyNone);

    remote_input_model_update(&model, RemoteInputKeyBack, RemoteInputEventPress, true, false);
    expect_result(
        remote_input_model_update(&model, RemoteInputKeyBack, RemoteInputEventLong, true, false),
        RemoteActionExit,
        RemoteInputKeyNone);
    assert(model.pressed_buttons == 0U);
}

static void test_pairing_reset_confirm_cancel_and_exit(void) {
    RemoteInputModel model;
    remote_input_model_reset(&model);

    remote_input_model_update(&model, RemoteInputKeyOk, RemoteInputEventPress, false, true);
    remote_input_model_update(&model, RemoteInputKeyOk, RemoteInputEventLong, false, true);
    assert(model.pairing_reset_confirmation);
    expect_result(
        remote_input_model_update(&model, RemoteInputKeyOk, RemoteInputEventRelease, false, true),
        RemoteActionNone,
        RemoteInputKeyNone);
    assert((model.pressed_buttons & RemoteButtonOk) == 0U);
    expect_result(
        remote_input_model_update(&model, RemoteInputKeyBack, RemoteInputEventShort, false, true),
        RemoteActionNone,
        RemoteInputKeyNone);
    assert(!model.pairing_reset_confirmation);

    remote_input_model_update(&model, RemoteInputKeyBack, RemoteInputEventPress, false, true);
    remote_input_model_update(&model, RemoteInputKeyOk, RemoteInputEventLong, false, true);
    remote_input_model_update(&model, RemoteInputKeyBack, RemoteInputEventRelease, false, true);
    assert(model.pairing_reset_confirmation);
    assert((model.pressed_buttons & RemoteButtonBack) == 0U);
    expect_result(
        remote_input_model_update(&model, RemoteInputKeyOk, RemoteInputEventPress, false, true),
        RemoteActionResetPairing,
        RemoteInputKeyNone);
    assert(!model.pairing_reset_confirmation);

    remote_input_model_update(&model, RemoteInputKeyOk, RemoteInputEventLong, false, true);
    expect_result(
        remote_input_model_update(&model, RemoteInputKeyBack, RemoteInputEventLong, false, true),
        RemoteActionExit,
        RemoteInputKeyNone);
}

static void test_irrelevant_input_is_ignored(void) {
    RemoteInputModel model;
    remote_input_model_reset(&model);

    expect_result(
        remote_input_model_update(&model, RemoteInputKeyNone, RemoteInputEventOther, true, false),
        RemoteActionNone,
        RemoteInputKeyNone);
    expect_result(
        remote_input_model_update(&model, RemoteInputKeyOk, RemoteInputEventLong, false, false),
        RemoteActionNone,
        RemoteInputKeyNone);
}

int main(void) {
    test_disconnected_direction_is_visual_only();
    test_connected_direction_tracks_hid_hold();
    test_reconnect_does_not_create_unmatched_release();
    test_disconnect_clears_direction_hold();
    test_ok_pulses_once_per_short_press();
    test_pairing_reset_releases_connected_direction_before_cancel();
    test_back_short_and_long_are_distinct();
    test_pairing_reset_confirm_cancel_and_exit();
    test_irrelevant_input_is_ignored();
    puts("remote_input_model tests passed");
    return 0;
}
