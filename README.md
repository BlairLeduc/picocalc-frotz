# picocalc-frotz

This is an unofficial port of Frotz to the PicoCalc. Frotz has been actively developed since 1995 and is a popular interpreter for Z-Machine games, including those from Infocom.

This project adds a small amount of code to integrate the included modules. Each module (picocalc-text-starter and frotz), which are unmodified, work together to provide a complete experience for playing Z-Machine games on the PicoCalc.

> [!NOTE]
> This project is not affiliated with the original Frotz project or its developers. It is a port to the PicoCalc platform, which is based on the Pico-series microcontrollers.


# Features

- Play Z-Machine games on the PicoCalc
- Supports the PicoCalc's LCD display, keyboard, audio output and SD card
- Provides a 40x32 character display with an easy to read font
- Emulates a phosphor display, with white, green or amber phosphor (F10 to cycle through modes)
- Full line editing including history
- Store stories on an SD card in the Stories directory (`/Stories`)
- Includes a simple story selector to choose which story to play
- Supports saving and restoring, quitting and restarting stories

> [!TIP]
> The phosphor display is a visual effect that simulates the look of an old CRT display. It can be cycled any point during gameplay by pressing F10. The story selector is fixed to the white phosphor.

# Getting Started

Flash the PicoCalc with the latest release and reboot your PicoCalc.

A simple story selector is presented when you turn on the PicoCalc. This lists the stories stored in the `/stories` directory on the SD card. You select a story to play, and press enter to begin.

> [!TIP]
>A Pico 2 (W) or other RP2350-based device is recommended. The stories are loaded from the SD card into RAM. The RP2350-based boards have 520 KiB of RAM over the RP2040's 264 KiB, which allows for larger stories to be loaded. Saying that, many stories from Infocom will play on a Pico 1.

# Modules

- [PicoCalc Text Starter](https://github.com/BlairLeduc/picocalc-text-starter) - This module provides the drivers for the LCD display, keyboard, audio and file system support for the SD card.

- [Frotz](https://gitlab.com/DavidGriffith/frotz) – An interpreter for all Infocom and other Z-machine games. Complies with the Z-Machine Standard version 1.1.

_Note: the settings.ini file is not used by this project right now. The settings are hardcoded in the `picocalc/init.c` file._