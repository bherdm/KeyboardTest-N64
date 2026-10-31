# N64 Keyboard Test ROM

This is a simple test ROM for Nintendo 64 that detects full-size keyboards on any controller port and shows which keys are currently pressed. Keyboards connect through [BlueRetroForever](https://github.com/bherdm/BlueRetroForever), a Bluetooth adapter, in its full-layout mode. A real RandNET Keyboard, or the adapter in its classic mode, isn't read: libdragon's Keyboard subsystem reads full-size keyboards only.

![Screenshot of detected keyboard with keys pressed](./screenshot.png?raw=true)

## Run the test ROM

[Download](./keyboardtest.z64?raw=true) or [compile](#build-the-rom) the ROM file and load it as you would any other N64 ROM.

This ROM file has been tested to work on real Nintendo 64 hardware using the [EverDrive-64 by krikzz](http://krikzz.com/) and [64drive by retroactive](http://64drive.retroactive.be/).

## Build the ROM

1. [Install LibDragon](https://github.com/DragonMinded/libdragon) and make sure you export `N64_INST` as the path to your N64 compiler toolchain. The ROM reads keyboards through libdragon's Keyboard subsystem (`keyboard.h`), from the `blueretroforever-keyboard` branch of [bherdm/libdragon](https://github.com/bherdm/libdragon).
2. Run `make` to produce `keyboardtest.z64`

The ROM logs to the SummerCart64's USB debug lane: `[id]` when a port's device changes, `[hb]` once a second, `[ev]` for each key going down or up (its HID usage as libdragon codes it, its name, and the milliseconds since boot), and `[kb]` when a keyboard's held keys, LEDs, layout or arrangement change: up to ten keys, in the adapter's order, and whether more are held. Caps Lock, Num Lock and Scroll Lock toggle their LEDs. `[br]` lines show BlueRetroForever's state, read once a second through its command 0x7B when it changes: pairing, the LEDs' level, each port's device and keyboard settings. The ROM changes none of them.

## License

This project is [Unlicensed public domain software](./LICENSE.md?raw=true) written by [Christopher Bonhage](https://github.com/meeq).

LibDragon is [Unlicensed public domain software](https://github.com/DragonMinded/libdragon/blob/trunk/LICENSE.md?raw=true).

"Nintendo 64" is a registered trademark of Nintendo used for informational purposes without permission.
