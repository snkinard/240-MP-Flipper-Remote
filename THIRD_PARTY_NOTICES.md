# Third-party notices

This project contains assets copied from and code adapted from the official [Flipper Zero firmware](https://github.com/flipperdevices/flipperzero-firmware) tag [`1.4.3`](https://github.com/flipperdevices/flipperzero-firmware/tree/1.4.3).

## Keynote drawing and layout

The vertical controller drawing and layout in `mp_240_remote.c` adapts the Keynote HID view from:

- [`applications/system/hid_app/views/hid_keynote.c`](https://github.com/flipperdevices/flipperzero-firmware/blob/1.4.3/applications/system/hid_app/views/hid_keynote.c)

The adaptation retains the vertical directional-button and action-button layout, arrow drawing, and pressed-state presentation. Changes include the `240-MP` title, `Select` label, Return/Escape HID mappings, app-specific model and callback integration, connection and Bluetooth failure states, private-bond reset confirmation, and helper refactoring.

## Bluetooth HID lifecycle and pairing reset

The Bluetooth HID profile lifecycle and pairing-reset flow in `mp_240_remote.c` adapt the official HID app implementation from:

- [`applications/system/hid_app/hid.c`](https://github.com/flipperdevices/flipperzero-firmware/blob/1.4.3/applications/system/hid_app/hid.c)

The adaptation retains the disconnect settling delay, private bond-storage path, HID profile startup and advertising, pairing reset through bonded-device removal, callback cleanup, and restoration of the default bond-storage path and Bluetooth profile. Changes include app-specific profile parameters, failure handling, view-dispatcher event delivery, and the single-view application lifecycle.

## Image assets

The following files are byte-for-byte copies from tag `1.4.3`:

| Local file | Official firmware 1.4.3 source path |
| --- | --- |
| `mp_240_remote_10px.png` | `applications/system/hid_app/hid_ble_10px.png` |
| `images/Button_18x18.png` | `applications/system/hid_app/assets/Button_18x18.png` |
| `images/Space_60x18.png` | `applications/system/hid_app/assets/Space_60x18.png` |
| `images/Ble_connected_15x15.png` | `applications/system/hid_app/assets/Ble_connected_15x15.png` |
| `images/Ble_disconnected_15x15.png` | `applications/system/hid_app/assets/Ble_disconnected_15x15.png` |
| `images/Pin_back_arrow_10x8.png` | `applications/system/hid_app/assets/Pin_back_arrow_10x8.png` |
| `images/Ok_btn_9x9.png` | `applications/system/hid_app/assets/Ok_btn_9x9.png` |

The image contents are unchanged. The six interface assets retain their official filenames; the launcher icon is stored under the project-specific filename `mp_240_remote_10px.png`. Their use and placement in 240-MP Remote are modifications made by this project.

## License

The official firmware tag identifies its license as the [GNU General Public License version 3](https://github.com/flipperdevices/flipperzero-firmware/blob/1.4.3/LICENSE). This repository distributes its source and the adapted/copy material under the [GNU GPL version 3](LICENSE); the local C source files identify `GPL-3.0-only` with SPDX headers.
