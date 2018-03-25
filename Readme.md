﻿# Palm m515/OS 4.1 emulator

This is a complete restart of my Palm OS emulator, with the last one the code got too messy and the goal too ambitious(to emulate every palm API in software and for compatibility with all Palm OS versions and devices).


# The goal of this project

To emulate every part of the Palm m515 that is used by the OS perfectly.
This means no hacks like POSE where API calls are intercepted and replaced with those that dont use the actual hardware.
It is also written in C for RetroArch so it will run on everything at the start, not just Win95 like POSE and Copilot and is being developed with modern emulator features in mind, save/load state and overclocking are available from the start.

## Why the m515?

It is the best Palm brand OS 4 device that has no special hardware builtin(cellphone, barcode scanner, keyboard attachment), it has a color screen, and available ROM dumps.

## What about accessorys?

The Palm keyboard attachment will likely be emulated later on so the PC keyboard can be used on text fields.

## OS improvements

If there is something the OS does not support(like an OS 5 API) I may add it manually, this will be in the form of a custom firmware as a ROM image or a .bps patch, not a patch to the emulator with only 1 exception "sysTrapPceNativeCall" which would require an ARM interpreter, it would also be an option to not apply this patch leaving you with an accurately emulated stock Palm m515.

These patches will not be worked on until the emulator is actually running.

## But you shouldnt waste your time on OS 4, OS 5 can do more.

I have made a video of OS 5 doing that "more", it is extremely incompatible with OS 4 games which most games are, very few games used the ARM CPU and those that did are available on other platforms, some used the 320x320 display mode.

Palm T|X vs OS 4 Games

## Credits
Musashi v3.4(last version that builds outside of mame) (68k core)

## License
<a rel="license" href="http://creativecommons.org/licenses/by-nc/3.0/us/"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by-nc/3.0/us/88x31.png" /></a><br />This work is licensed under a <a rel="license" href="http://creativecommons.org/licenses/by-nc/3.0/us/">Creative Commons Attribution-NonCommercial 3.0 United States License</a>.