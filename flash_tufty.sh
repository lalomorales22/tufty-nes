#!/bin/bash
# Flash Tufty 2350 with NES emulator using picotool (RECOMMENDED)
# picotool is much more reliable than cp to /Volumes/RP2350

UF2_FILE="$HOME/Desktop/tufty/pico-nes/bin/Release/tufty-nes-TFT-PARALLEL-PWM-305.uf2"

echo "================================================"
echo "  Tufty 2350 NES Emulator Flasher"
echo "================================================"
echo ""
echo "UF2 file: $UF2_FILE"

if [ ! -f "$UF2_FILE" ]; then
    echo "ERROR: UF2 file not found!"
    exit 1
fi

echo "Size: $(ls -lh "$UF2_FILE" | awk '{print $5}')"
echo ""

# Check if picotool is available
if ! command -v picotool &> /dev/null; then
    echo "ERROR: picotool not found. Install with: brew install picotool"
    exit 1
fi

# Try to detect device state
echo "Checking for device..."
if picotool info 2>/dev/null | grep -q "RP2350"; then
    echo "Device found in BOOTSEL mode."
elif picotool info -f 2>/dev/null | grep -q "target"; then
    echo "Device found running firmware. Rebooting into BOOTSEL..."
    picotool reboot -f -u
    sleep 3
else
    echo "No device detected."
    echo ""
    echo "  Hold BOOTSEL and plug in USB cable"
    echo "  (or hold BOOTSEL + press RESET if already plugged in)"
    echo ""
    echo "Waiting for device..."
    TIMEOUT=60
    COUNT=0
    while ! picotool info 2>/dev/null | grep -q "RP2350"; do
        sleep 1
        COUNT=$((COUNT + 1))
        if [ $COUNT -ge $TIMEOUT ]; then
            echo "ERROR: Timed out after ${TIMEOUT}s waiting for device"
            exit 1
        fi
        printf "."
    done
    echo ""
    echo "Device found!"
fi

echo ""
echo "Flashing firmware..."
picotool load "$UF2_FILE"
RESULT=$?

if [ $RESULT -ne 0 ]; then
    echo "ERROR: picotool load failed (exit code: $RESULT)"
    exit 1
fi

echo ""
echo "Verifying flash..."
picotool verify "$UF2_FILE"

echo ""
echo "Rebooting into application mode..."
picotool reboot

echo ""
echo "DONE! The Tufty should reboot with the NES emulator now."
