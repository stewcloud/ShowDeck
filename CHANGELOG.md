# Changelog

## 0.4.1 — On-device settings and Elecrow save fix

- Corrected the Elecrow filesystem partition label so `LittleFS.begin()` can
  mount it and persist web-editor configuration and artwork.
- Added a mount fallback for the original v0.4.0 Elecrow partition label so
  the configuration-preserving update image fixes existing installations.
- Replaced direct configuration writes with a temporary write, JSON
  verification, backup and rollback sequence.
- Added clear filesystem diagnostics and more useful browser save errors.
- Added an on-device Settings screen, opened with the header gear, with
  immediately saved brightness controls, setup-hotspot access and restart.
- Added primitive-drawn Wi-Fi, status and settings iconography that does not
  depend on Unicode font support.

## 0.4.0 — Elecrow 5-inch V3 support

- Added a second PlatformIO target for the Elecrow DIS07050H 5-inch HMI,
  hardware V3.0, while retaining the original CYD target.
- Added the Elecrow 800x480 RGB display timings and pin map.
- Added GT911 capacitive-touch support through the board's PCA9557 reset
  circuit.
- Adapted the physical 3x3 deck layout, hit areas, labels, header and swipe
  threshold to the larger screen.
- Scale the existing full-bleed artwork format to the larger physical buttons,
  keeping one configuration format portable between both boards.
- Added independently named factory and update images for each board.

## 0.3.2 — Detailed help and button rearranging

- Added `?` controls with expanded guidance for pages, artwork, button fields,
  action types, networking, MQTT, and ESP-NOW.
- Made Target and Payload help adapt to the selected action type.
- Added drag-and-drop button rearranging for desktop browsers.
- Added long-press-and-drag rearranging for phones and tablets.
- Moves the complete button configuration, including artwork and actions.

## 0.3.1 — Artwork colors and editor guidance

- Corrected RGB565 byte order when drawing uploaded images and emoji artwork.
- Applied the correction when reading existing artwork, so images do not need
  to be uploaded again.
- Added a Reset button command that returns a configured button to blank.
- Added action-specific Target and Payload guidance with practical examples.
- Added concise help for labels, colors, Wi-Fi, MQTT, and ESP-NOW settings.

## 0.3.0 — Full-bleed artwork and dynamic pages

- Expanded uploaded artwork to fill the complete visible button face.
- Changed image conversion to crop-to-fill, matching Stream Deck-style tiles.
- Render emoji artwork automatically when the button editor is closed with
  Done; a separate render step is no longer required.
- Added Add page and Delete page controls to the browser editor.
- Added support for one to twelve pages while preserving existing pages and
  artwork during the configuration migration.
- Kept compatibility with the smaller artwork files created by version 0.2.0.

## 0.2.0 — Show Deck editor and artwork

- Renamed the project and default device to Show Deck.
- Added an explicit UTF-8 response charset to eliminate setup-page mojibake.
- Replaced the expanded button forms with a 3x3 live deck preview.
- Added a three-dot per-button settings dialog.
- Added browser-side image resizing and RGB565 artwork uploads.
- Added emoji rendering to bitmap artwork for display compatibility.
- GIF uploads use the first frame; animated playback remains planned.
- Migrates the original default `TikiDeck` device name to `Show Deck` without
  replacing a user-customized name.

## 0.1.1 — Rear LED default

- Drive all three active-low channels of the onboard rear RGB LED high during
  early startup so the LED defaults to off.

## 0.1.0 — MVP

- Added four configurable 3×3 control pages.
- Added embedded phone/desktop configuration editor.
- Added LittleFS configuration persistence and JSON import/export.
- Added ESP-NOW broadcast, MQTT, HTTP and page actions.
- Added fallback setup hotspot and Wi-Fi OTA support.
- Added example ESP-NOW receiver and protocol documentation.
