# picocalc-frotz

This is an unofficial port of [Frotz](https://davidgriffith.gitlab.io/frotz/) for the [PicoCalc kit](https://www.clockworkpi.com/picocalc). Frotz has been actively developed since 1995 and is a popular interpreter for [Z-Machine](https://en.wikipedia.org/wiki/Z-machine) games, including those from [Infocom](https://en.wikipedia.org/wiki/Infocom).

This project adds a small amount of code to integrate the included modules. Each module ([Frotz](https://gitlab.com/DavidGriffith/frotz), [iniParser 4](https://gitlab.com/iniparser/iniparser) and [PicoCalc Text Starter](https://github.com/BlairLeduc/picocalc-text-starter)), which are unmodified, work together to provide a complete experience for playing Z-Machine games on the PicoCalc kit.

> [!NOTE]
> This project is not affiliated with the original Frotz project or its developers. It is a port to the PicoCalc kit, which is based on the [Pico-series microcontrollers](https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html).


# Features

- Play Z-Machine games on the PicoCalc
- Supports the PicoCalc's LCD display, keyboard, audio output and SD card
- Provides a 40×32 or 64×32 character display with an easy to read font
- Emulates a phosphor display, with white, green or amber phosphor (F10 to cycle through modes)
- Full line editing including history and tab completion
- Store stories on an SD card in the Stories directory (`/Stories`)
- Includes a simple story selector to choose which story to play
- Each story can be configured to use a phosphor colour and the number of columns on the display in the `settings.ini` file 
- Supports saving and restoring, quitting and restarting stories

> [!TIP]
> The phosphor display is a visual effect that simulates the look of an old CRT display. It can be cycled any point during gameplay by pressing F10. 

# Getting Started

Flash the PicoCalc with the latest release and reboot your PicoCalc.

A simple story selector is presented when you turn on the PicoCalc. This lists the stories stored in the `/Stories` directory on the SD card. You select a story to play, and press enter to begin.

You may also change the number of columns and phosphor colour. Whilst you can change the phosphor colour at any time during gameplay, the number of columns is set when you select the story and cannot be changed until you restart the story.

> [!NOTE]
> The story selector will only show stories that are in the `/Stories` directory and is limited to the first 24 stories.

> [!TIP]
>A Pico 2 (W) or other RP2350-based device is recommended. The stories are loaded from the SD card into RAM. The RP2350-based boards have 520 KiB of RAM over the RP2040's 264 KiB, which allows for larger stories to be loaded.
>
>Saying that, many, if not most, stories from the community and Infocom will play on a Pico board from the Pico 1 family.

# Settings

The following settings can be configured in the `/Stories/settings.ini` file:

- `phosphor`: The colour of the emulated phosphor display. Options are `white`, `green`, or `amber`. The default is `white`.
- `columns`: The number of columns on the screen. Options are `40` or `64`. The default is `40`. 

Example of a settings file:

```ini
# The settings.ini file for Frotz configuration

[default]
# The phosphor setting determines the color scheme of the display.
# phosphor=white|green|amber
phosphor=white

# Number of columns on the screen
# columns=40|64
columns=40

[Sampler1.z5]
phosphor=green
columns=64

[Sampler2.z3]
phosphor=amber
columns=64

[Tutorial.z3]
phosphor=white
columns=64
```

# Managing Stories

Stories are added to the `/Stories` directory on the SD card. The story selector will automatically detect new stories when you reboot the PicoCalc.

It is the perfect time to set the settings for the added story in the `settings.ini` file. The settings file is read when the story is selected. This allows each story to have its own configured settings for the phosphor colour and number of columns.

If a story does not have settings configured, the story will use the display configuration as set in the story selector.

# Modules

This project uses the following open-source projects:

- [Frotz](https://gitlab.com/DavidGriffith/frotz) – An interpreter for all Infocom and other Z-machine games. Complies with the Z-Machine Standard version 1.1.

- [iniParser 4](https://gitlab.com/iniparser/iniparser) – A simple C library offering ini file parsing services.

- [PicoCalc Text Starter](https://github.com/BlairLeduc/picocalc-text-starter) – This module provides the drivers for the LCD display, keyboard, audio and file system support for the SD card.
