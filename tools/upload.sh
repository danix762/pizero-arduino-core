#!/bin/bash

# Arduino Pi Zero upload script
# Usage: upload.sh <path-to-kernel.img>

set -e

KERNEL_IMG="$1"

if [ -z "$KERNEL_IMG" ]; then
    echo "Usage: $0 <kernel.img>"
    exit 1
fi

if [ ! -f "$KERNEL_IMG" ]; then
    echo "Error: $KERNEL_IMG not found"
    exit 1
fi

echo "Looking for Raspberry Pi SD card..."

# Find a mounted FAT partition that looks like a Pi boot volume
BOOT_MOUNT=""

for mountpoint in $(findmnt -n -o TARGET -t vfat 2>/dev/null); do
    if [ -f "$mountpoint/bootcode.bin" ] || [ -f "$mountpoint/start.elf" ] || [ -f "$mountpoint/config.txt" ]; then
        BOOT_MOUNT="$mountpoint"
        break
    fi
done

# Also check common locations if nothing found
if [ -z "$BOOT_MOUNT" ]; then
    for candidate in /media/$USER/* /run/media/$USER/* /mnt/*; do
        if [ -d "$candidate" ]; then
            if [ -f "$candidate/bootcode.bin" ] || [ -f "$candidate/start.elf" ] || [ -f "$candidate/config.txt" ]; then
                BOOT_MOUNT="$candidate"
                break
            fi
        fi
    done
fi

if [ -z "$BOOT_MOUNT" ]; then
    echo "Error: Could not find a Raspberry Pi SD card."
    echo "Make sure the SD card is plugged into this computer and mounted."
    exit 1
fi

echo "Found Pi boot volume at: $BOOT_MOUNT"
echo "Copying kernel.img ..."

cp -f "$KERNEL_IMG" "$BOOT_MOUNT/kernel.img"
sync

echo "Upload complete!"
echo "You can now put the SD card back into the Pi Zero."
