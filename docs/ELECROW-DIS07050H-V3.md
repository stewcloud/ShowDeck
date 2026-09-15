# Elecrow DIS07050H V3.0 support

This target supports the Elecrow 5-inch HMI Basic board identified as
`DIS07050H`, hardware `V3.0`, with an ESP32-S3-WROOM-1-N4R8 module.

## Hardware implemented

- 800x480 16-bit parallel RGB display
- GT911 capacitive touch on I2C pins 19 and 20
- PCA9557 touch reset/interrupt expander at address `0x18`
- Backlight PWM on GPIO 2
- Status LED on GPIO 38, off by default
- 4 MB flash and 8 MB octal PSRAM

The RGB pins and panel timing are based on Elecrow's V3.0 example. The original
CYD remains available as the separate `cyd` PlatformIO environment.

## First-device validation

This release has been compile-tested for both targets. Before permanently
mounting the first Elecrow unit, verify:

1. The rear PCB says `DIS07050H` and `V3.0`.
2. A factory flash boots and creates the `ShowDeck-Setup` access point.
3. The complete image is visible with correct colors and orientation.
4. All four corners and all nine button regions respond to touch.
5. Horizontal swipes move one page in the expected direction.
6. Brightness changes work and the GPIO 38 status LED remains off.

If the picture works but touch is mirrored or rotated, record the raw corner
behavior before changing the coordinate mapping.
