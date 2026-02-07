#!/usr/bin/env bash
#
# flash_tufty.sh - Build and flash the Tufty NES emulator
#
# Usage:
#   ./flash_tufty.sh          # Full clean build + flash
#   ./flash_tufty.sh --flash  # Flash only (skip build)
#   ./flash_tufty.sh --build  # Build only (skip flash)
#

set -euo pipefail

# ── Configuration ───────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_DIR/build"
ELF_FILE="$PROJECT_DIR/bin/Release/tufty-nes-TFT-PARALLEL-PWM-305.elf"

# Toolchain paths - adjust these for your system
PICO_SDK_PATH="${PICO_SDK_PATH:-/Users/minibrain/pico-sdk}"
ARM_TOOLCHAIN="${ARM_TOOLCHAIN:-/Users/minibrain/arm-toolchain/bin}"
PICOTOOL="${PICOTOOL:-picotool}"
MAKE="${MAKE:-/usr/bin/make}"

ARM_GCC="$ARM_TOOLCHAIN/arm-none-eabi-gcc"
ARM_GPP="$ARM_TOOLCHAIN/arm-none-eabi-g++"

# ── Helpers ─────────────────────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

info()  { echo -e "${CYAN}[INFO]${NC}  $*"; }
ok()    { echo -e "${GREEN}[OK]${NC}    $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC}  $*"; }
err()   { echo -e "${RED}[ERROR]${NC} $*" >&2; }

# ── Preflight checks ───────────────────────────────────────────────
preflight() {
    local fail=0

    if [ ! -d "$PICO_SDK_PATH" ]; then
        err "Pico SDK not found at $PICO_SDK_PATH"
        err "Set PICO_SDK_PATH environment variable to your pico-sdk location"
        fail=1
    fi

    if [ ! -x "$ARM_GCC" ]; then
        err "ARM GCC not found at $ARM_GCC"
        err "Set ARM_TOOLCHAIN environment variable to your toolchain bin/ directory"
        fail=1
    fi

    if ! command -v "$PICOTOOL" &>/dev/null; then
        err "picotool not found. Install with: brew install picotool"
        fail=1
    fi

    if [ $fail -ne 0 ]; then
        exit 1
    fi
}

# ── ROM table generation ───────────────────────────────────────────
generate_roms() {
    if [ ! -f "$PROJECT_DIR/src/rom_table.h" ]; then
        info "Generating ROM table header..."
        python3 "$PROJECT_DIR/tools/convert_roms.py"
        ok "ROM table generated"
    else
        # Regenerate if any ROM is newer than the header
        local rom_newer=0
        for rom in "$PROJECT_DIR/ROMs/"*.nes; do
            [ -f "$rom" ] || continue
            if [ "$rom" -nt "$PROJECT_DIR/src/rom_table.h" ]; then
                rom_newer=1
                break
            fi
        done
        if [ $rom_newer -eq 1 ]; then
            info "ROM files changed, regenerating ROM table..."
            python3 "$PROJECT_DIR/tools/convert_roms.py"
            ok "ROM table regenerated"
        else
            info "ROM table is up to date"
        fi
    fi
}

# ── Build ───────────────────────────────────────────────────────────
build() {
    info "Starting clean build..."

    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"

    info "Running CMake..."
    cd "$BUILD_DIR"
    PICO_SDK_PATH="$PICO_SDK_PATH" cmake -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_COMPILER="$ARM_GCC" \
        -DCMAKE_CXX_COMPILER="$ARM_GPP" \
        -DCMAKE_ASM_COMPILER="$ARM_GCC" \
        -DCMAKE_MAKE_PROGRAM="$MAKE" \
        "$PROJECT_DIR"
    cd "$PROJECT_DIR"

    info "Compiling (make -j4)..."
    "$MAKE" -C "$BUILD_DIR" -j4

    if [ -f "$ELF_FILE" ]; then
        ok "Build succeeded: $ELF_FILE"
    else
        err "Build failed - ELF file not found"
        exit 1
    fi
}

# ── Flash ───────────────────────────────────────────────────────────
flash() {
    if [ ! -f "$ELF_FILE" ]; then
        err "ELF file not found: $ELF_FILE"
        err "Run a build first: ./flash_tufty.sh --build"
        exit 1
    fi

    info "Checking for Tufty in bootloader mode..."
    if ! "$PICOTOOL" info &>/dev/null; then
        warn "Device not detected in bootloader mode."
        echo ""
        echo "  To enter bootloader mode:"
        echo "    1. Unplug USB"
        echo "    2. Hold the A button"
        echo "    3. Plug USB back in while holding A"
        echo "    4. Release A"
        echo ""
        read -rp "Press Enter when the device is in bootloader mode..."

        if ! "$PICOTOOL" info &>/dev/null; then
            err "Still can't detect device. Check USB connection."
            exit 1
        fi
    fi

    ok "Device detected"
    info "Flashing firmware..."
    "$PICOTOOL" load -f -x "$ELF_FILE"
    ok "Flash complete - device is rebooting"
}

# ── Main ────────────────────────────────────────────────────────────
main() {
    echo ""
    echo "========================================="
    echo "  Tufty NES - Build & Flash"
    echo "========================================="
    echo ""

    preflight

    case "${1:-}" in
        --flash)
            flash
            ;;
        --build)
            generate_roms
            build
            ;;
        *)
            generate_roms
            build
            flash
            ;;
    esac

    echo ""
    ok "Done!"
}

main "$@"
