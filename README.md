# N64 Keyboard Test ROM

A test ROM for the Nintendo 64 that shows every key of a full-size keyboard as it's pressed, on any controller port. Keyboards connect through [BlueRetroForever](https://github.com/bherdm/BlueRetroForever), a Bluetooth controller adapter, and are read by the Keyboard subsystem in [bherdm/libdragon](https://github.com/bherdm/libdragon).

It started as Christopher Bonhage's RandNET Keyboard test ROM. It now reads full-size keyboards instead: a real RandNET Keyboard, or BlueRetroForever in its classic mode, isn't read.

## What it shows

- Each controller port's device.
- BlueRetroForever's state: pairing, its LED level, and each port's keyboard settings.
- Each keyboard's lit LEDs, the layout and Mac or PC arrangement the adapter is set to, and up to ten held keys by name, modifiers among them.
- The latest eight keys to go down or up.

Caps Lock, Num Lock and Scroll Lock toggle their LEDs. The ROM never changes BlueRetroForever's settings.

## Tested

On a real Nintendo 64 with a SummerCart64 and BlueRetroForever: every key of US, UK, German, French and Japanese (JIS) keyboards, media and system keys, chords of ten keys, and the lock LEDs.

## Build the ROM

1. Install the libdragon toolchain from the `blueretroforever-keyboard` branch of [bherdm/libdragon](https://github.com/bherdm/libdragon), and export `N64_INST` as its path.
2. Run `make` to produce `keyboardtest.z64`, then load it as you would any other N64 ROM.

## Debug output

On a SummerCart64, the ROM logs over the cart's USB debug channel (`sc64deployer debug`):

| Line | When | What |
| --- | --- | --- |
| `[id]` | a port's device changes | the device and its Joybus identifier |
| `[hb]` | once a second | a heartbeat with each port's identifier |
| `[ev]` | a key goes down or up | its code (the key's HID usage), name, and milliseconds since boot |
| `[kb]` | a keyboard's state changes | LEDs, layout, arrangement, held keys in the adapter's order, and whether more are held |
| `[br]` | BlueRetroForever's state changes | pairing, LED level, each port's device and keyboard settings |

## License

This project is [Unlicensed public domain software](./LICENSE.md?raw=true), originally written by [Christopher Bonhage](https://github.com/meeq), with changes by [bherdm](https://github.com/bherdm).

libdragon is [Unlicensed public domain software](https://github.com/bherdm/libdragon/blob/blueretroforever-keyboard/LICENSE.md).

"Nintendo 64" is a registered trademark of Nintendo used for informational purposes without permission.

## AI Disclaimer

LLM coding and testing make this project possible.
