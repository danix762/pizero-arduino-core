# Arduino core for the Raspberry Pi Zero (BCM2835), bare metal

An experimental Arduino core that runs sketches directly on the Raspberry Pi Zero, with
no Linux underneath. You write a normal Arduino sketch, press Upload, and the sketch
becomes the `kernel.img` the Pi boots.

**Status: early alpha, developed and tested on one board by one person.** Please report
what works and what doesn't.

## What works

| Feature | Status |
|---|---|
| GPIO, PWM (GPIO 12/13/18/19), GPIO edge interrupts | tested |
| millis / micros / delay, malloc / new, global C++ constructors | tested |
| I2C (tested with an LCD), SPI (loopback only) | tested |
| SD card with FatFs (reads; one small write) | partly tested |
| Hardware UART `Serial1` (jumper loopback only, no real adapter yet) | partly tested |
| USB device: serial, keyboard, mouse | tested |
| Background USB service on a timer interrupt (`USB_BG_ENABLE`) | experimental, off by default |
| USB host, Windows/macOS upload, Pi Zero W / 2 W | untested |

## Requirements

- Linux (the upload tool is Linux only so far)
- Arduino IDE 2.x
- The ARM bare-metal toolchain, installed system-wide:
  - Arch / Manjaro: `sudo pacman -S arm-none-eabi-gcc arm-none-eabi-newlib arm-none-eabi-binutils`
  - Debian / Ubuntu: `sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi`
- A Raspberry Pi Zero (original BCM2835 board), a micro-SD card, and a card reader

## Install the core

1. Copy this repository's contents to `~/Arduino/hardware/pizero/bcm2835/`
   (create the folders if needed), so that `boards.txt` is directly inside `bcm2835`.
2. Restart the Arduino IDE.
3. Choose **Tools > Board > Raspberry Pi Zero**.

## Prepare the SD card

The card needs a single FAT32 partition. The Raspberry Pi's boot ROM loads these files from it:

| File | Where it comes from |
|---|---|
| `bootcode.bin` | Raspberry Pi firmware repository, `boot/` folder |
| `start.elf` | same |
| `fixup.dat` | same |
| `config.txt` | this repository, `sdcard/config.txt` |
| `kernel.img` | created by the Upload button (your compiled sketch) |

1. Format the card as FAT32. Be careful to pick the right device, because this erases it:
   ```bash
   lsblk                                      # identify the card, for example /dev/sdX
   sudo mkfs.vfat -F 32 -n PIZERO /dev/sdX1   # replace sdX1 with YOUR card's partition
   ```
2. Download the three firmware files (do not clone the whole repository, it is huge):
   ```bash
   cd /tmp
   curl -LO https://github.com/raspberrypi/firmware/raw/master/boot/bootcode.bin
   curl -LO https://github.com/raspberrypi/firmware/raw/master/boot/start.elf
   curl -LO https://github.com/raspberrypi/firmware/raw/master/boot/fixup.dat
   ```
   If a link fails, download the same files in a browser from the `boot` folder of
   https://github.com/raspberrypi/firmware
3. Copy `bootcode.bin`, `start.elf`, `fixup.dat` and `sdcard/config.txt` to the root of the card.
4. `config.txt` must contain at least:
   ```
   kernel=kernel.img
   force_turbo=1
   ```
   `force_turbo=1` keeps the CPU and core clock at full speed. Several drivers (SPI, I2C)
   read the core clock when they start, and it should not change while a sketch runs.

## Your first sketch

The Pi Zero's green activity LED is on GPIO 47.

```cpp
void setup() {
  pinMode(47, OUTPUT);
}

void loop() {
  digitalWrite(47, HIGH);
  delay(500);
  digitalWrite(47, LOW);
  delay(500);
}
```

## Upload and run

1. Put the prepared card in your PC and make sure it is mounted.
2. Click **Upload** in the Arduino IDE. The upload tool finds the mounted Pi card and copies
   `kernel.img` onto it. Wait for "Upload complete".
3. Eject the card, put it in the Pi and power the Pi.

## Serial monitor over USB

Connect the Pi Zero's micro-USB port labelled **USB** (not PWR) to your PC. It appears as a USB
serial device (for example `/dev/ttyACM0`). Select that port in the IDE and open the
Serial Monitor. The baud rate you pick is ignored. `Serial` is a USB serial port.

## Pins and peripherals

Pin numbers are the Broadcom GPIO numbers (not the header pin numbers).

| Feature | Pins and notes |
|---|---|
| Digital I/O | `pinMode`, `digitalWrite`, `digitalRead` on GPIO 0 to 53 (header: GPIO 2 to 27) |
| PWM | `analogWrite` on GPIO 12, 13, 18, 19. 12 and 18 share a channel, as do 13 and 19. About 1 kHz, 8 bit |
| Interrupts | `attachInterrupt` on GPIO 0 to 31, modes RISING, FALLING, CHANGE |
| `Serial` | USB serial |
| `Serial1` | hardware UART: GPIO 14 = TX (header pin 8), GPIO 15 = RX (header pin 10), 3.3 V logic |
| `Wire` (I2C) | GPIO 2 = SDA, GPIO 3 = SCL |
| `SPI` | GPIO 10 = MOSI, GPIO 9 = MISO, GPIO 11 = SCLK. Chip select is any GPIO you drive yourself |
| SD card | FatFs: `#include "ff.h"`, then `f_mount`, `f_open`, and so on. The card you boot from is the one you read |
| USB | `Keyboard`, `Mouse` |

Minimal SD example:

```cpp
#include "ff.h"
static FATFS fs;

void setup() {
  Serial.begin(115200);
  delay(1500);
  if (f_mount(&fs, "", 1) == FR_OK) {
    DIR dir; FILINFO fno;
    f_opendir(&dir, "/");
    while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0]) Serial.println(fno.fname);
    f_closedir(&dir);
  }
}
void loop() {}
```

Writes are disabled by default (`FF_FS_READONLY` in `cores/arduino/ffconf.h`).

## Warnings

- GPIO is **3.3 V only**. Never connect 5 V signals directly, including the pull-ups of some
  I2C modules that run from 5 V.
- Back up any card you care about. Writing to the boot card is only lightly tested.
- If the program crashes, the core prints a crash report (type, address, registers) over USB
  serial and blinks the activity LED.

## Known limits

- No USB host mode, no Windows/macOS upload tool
- SD is polled single-block, not fast
- Only the original Pi Zero (BCM2835) has been tried
- Many Arduino helper functions are missing (see the issue tracker)

## License

MIT for this repository's own code (see `LICENSE`). Third-party code keeps its own licenses;
see `THIRD_PARTY_NOTICES.md`.
