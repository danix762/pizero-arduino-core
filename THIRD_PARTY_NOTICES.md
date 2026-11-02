
This core includes code from other projects. Each keeps its own license; this
repository's MIT license does not apply to the files listed here.

- **FatFs** (`cores/arduino/ff.c`, `ff.h`, `ffconf.h`, `ffunicode.c`, `diskio.h`):
  Copyright (C) ChaN. Licensed under the FatFs license; see the headers of those files.
  http://elm-chan.org/fsw/ff/
  (`cores/arduino/diskio.c` is original glue code for this core.)
- **TinyUSB** (`cores/arduino/usb/tinyusb/`): MIT License, Copyright (c) Ha Thach.
  https://github.com/hathach/tinyusb
- **Arduino core API files** (`cores/arduino/Print.h`, `Print.cpp`, `Printable.h`, `Stream.h`,
  `Stream.cpp`, `WString.h`, `WString.cpp`, `WCharacter.h`): included unmodified from
  https://github.com/arduino/ArduinoCore-avr. Copyright (c) Arduino LLC and contributors,
  licensed under the GNU Lesser General Public License v2.1 (or later, as stated in each
  file's header). A copy of the license is in `licenses/LGPL-2.1.txt`.
