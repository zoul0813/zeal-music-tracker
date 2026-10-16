#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
zeal=$(dirname "$root")
zgdk=${ZGDK_PATH:-$zeal/zeal-game-dev-kit}
core=${COREUTILS_PATH:-$zeal/zeal-coreutils}
zvb=${ZVB_SDK_PATH:-$zeal/Zeal-VideoBoard-SDK}
zos=${ZOS_PATH:-$zeal/zeal-dev-environment/home/Zeal-8-bit-OS}
binary=$(mktemp "${TMPDIR:-/tmp}/zmt-ui.XXXXXX")
trap 'rm -f "$binary"' EXIT HUP INT TERM
"${CC:-cc}" -std=c11 -g -fcommon -fno-builtin -funsigned-char -fsanitize=address,undefined \
    -Werror=implicit-function-declaration -Wno-undefined-inline -Wno-switch -Wno-incompatible-function-pointer-types -Wno-unused-value -Wno-int-to-void-pointer-cast -include "$root/tests/host.h" \
    -I"$root/src" -I"$zgdk/src/sound/tracker" -I"$zgdk/include" -I"$zvb/include" \
    -I"$core/libsrc/libcore" -I"$core/libsrc/libwindows" \
    -I"$zos/sdk/sdcc/include" "$root/tests/ui.c" \
    "$root/src/input.c" "$root/src/pattern.c" "$root/src/arrange.c" "$root/src/strings.c" \
    "$root/src/dialogs/file_dialog.c" "$root/src/dialogs/confirm_dialog.c" \
    "$root/src/dialogs/help_dialog.c" \
    "$zgdk/src/sound/tracker/main.c" "$zgdk/src/sound/tracker/global.c" \
    "$zgdk/src/sound/tracker/sound_control.c" "$zgdk/src/sound/tracker/notes.c" \
    "$zgdk/src/sound/tracker/note_names.c" "$zgdk/src/sound/tracker/pattern_nav.c" \
    "$zgdk/src/sound/tracker/track_getters.c" -o "$binary"
"$binary"
