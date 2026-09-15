# Show Deck

Show Deck is configurable handheld show-control firmware for supported ESP32
touch displays. Flash it once, then edit its pages and touchscreen buttons from
a phone or computer.

## Current MVP features

- One to twelve user-managed pages with nine touchscreen buttons each
- Browser editor for labels, colors and actions
- Configuration stored in LittleFS
- ESP-NOW broadcast actions for router-free props and candles
- MQTT publish actions for Home Assistant and Node-RED
- HTTP GET/JSON POST actions for webhooks and WLED
- Page-navigation actions and swipe navigation
- Setup access point when Wi-Fi is missing
- Wi-Fi OTA firmware support
- JSON configuration import/export
- Rear RGB status LED defaults to off
- Setup interface mirrors the physical 3x3 button layout
- Per-button configuration opens from a three-dot control
- Full-bleed button images and browser-rendered emoji artwork
- Contextual quick tips and detailed `?` help for important settings
- Mouse drag-and-drop and touch long-press button rearranging
- On-device Settings screen with brightness, setup-hotspot and restart controls

## Supported boards

| PlatformIO environment | Exact target | Display and touch |
| --- | --- | --- |
| `cyd` | Original ESP32-2432S028R "Cheap Yellow Display" | 320x240 ILI9341, XPT2046 resistive touch |
| `elecrow_5` | Elecrow DIS07050H 5-inch HMI, hardware V3.0 | 800x480 RGB panel, GT911 capacitive touch |

The Elecrow target follows the manufacturer's V3.0 pinout, including its
PCA9557 touch-reset circuit. Do not flash either image onto a similar-looking
board, another Elecrow revision, or the Advanced/P4 model until its hardware
has been confirmed.

## First flash

1. Install Visual Studio Code and the PlatformIO extension.
2. Open this folder as a PlatformIO project.
3. Select the environment for the connected board, then connect it over USB.
4. If uploading does not begin, hold `BOOT`, start Upload, then release `BOOT`
   when PlatformIO begins connecting.
5. Run `Upload Filesystem Image` only if PlatformIO requests it. The firmware
   creates its default configuration automatically.
6. Restart the board.

If no Wi-Fi has been configured, the board creates:

- SSID: `ShowDeck-Setup`
- Password: `tikitime`
- Editor: `http://192.168.4.1/`

Change the setup password in `src/main.cpp` before deploying the controller in
a public venue.

Build or upload one target explicitly with:

```text
pio run -e cyd -t upload
pio run -e elecrow_5 -t upload
```

## Release binaries

The `dist` folder contains separate binaries for each board:

- `ShowDeck-v0.4.1-CYD-update.bin` and `ShowDeck-v0.4.1-CYD-factory.bin`
- `ShowDeck-v0.4.1-Elecrow5-V3-update.bin` and
  `ShowDeck-v0.4.1-Elecrow5-V3-factory.bin`

An update image is written at `0x10000` and preserves the saved LittleFS button
configuration. A factory image is written at `0x0` for a first installation or
complete reset and should be treated as destructive to the saved setup.

Tap the gear in the physical display header to open on-device settings. The
brightness `-` and `+` controls save immediately; Setup AP starts the editor
hotspot and Restart reboots the deck.

## Button actions

| Type | Target | Payload |
| --- | --- | --- |
| `setup` | Ignored | Starts the temporary setup hotspot |
| `espnow` | Receiver group, e.g. `all_candles` | Command, e.g. `preset:3` |
| `mqtt` | MQTT topic | MQTT payload |
| `http` | Full `http://` URL | Blank for GET; JSON body for POST |
| `page` | Page number `1` through `12` | Ignored |
| `none` | Ignored | Ignored |

For a WLED HTTP button, use a target such as
`http://192.168.10.50/json/state` and a payload such as `{"ps":3}`.

## Important first-hardware checks

Before connecting a battery or power controller:

1. Photograph the rear of the board and confirm its printed model number.
2. Verify whether it has one USB connector or both USB-C and micro-USB.
3. Test screen and touch orientation before mounting it in an enclosure.
4. Identify unused GPIOs before wiring battery sensing or soft shutdown.

## Planned next milestone

- Guided touchscreen calibration
- Button press/hold/double-tap actions
- Macros with delays and parallel steps
- ESP-NOW receiver pairing, acknowledgement and retries
- WLED-native receiver/gateway support
- Battery percentage and low-voltage shutdown
- Graceful soft-power controller output
- Lock screen and protected blackout button
- Multiple named profile files on microSD

## Safety

Do not connect a bare Li-ion/LiPo cell to the board's 5 V input. Use a protected
charger/power-path circuit and the correct regulated output. Confirm polarity
and board revision before applying power. Use a stable 5 V supply sized for the
display; the 5-inch panel draws substantially more current than the CYD.
