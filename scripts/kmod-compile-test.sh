#!/bin/bash
#
# Compile test for OpenZFS kernel module on illumos.
# Verifies that individual .c files compile with kernel flags against
# both OpenZFS and illumos UTS headers.
#

set -e

OZ=${OZ:-/opt/openzfs_build/openzfs}
UTS=${UTS:-/opt/openzfs_build/smartos-live/projects/illumos/usr/src/uts}
CC=${CC:-gcc}
OBJDIR=${OBJDIR:-/tmp/openzfs-kmod-test}

mkdir -p "$OBJDIR"

# Kernel compilation flags (from uts/Makefile.uts + uts/intel/Makefile.intel)
KCFLAGS="-m64 -std=gnu11"
KCFLAGS+=" -ffreestanding -fno-builtin -fno-strict-aliasing -fno-common"
KCFLAGS+=" -mno-red-zone -mcmodel=kernel"
KCFLAGS+=" -fno-omit-frame-pointer"
KCFLAGS+=" -Wall -Wno-missing-braces -Wno-sign-compare"
KCFLAGS+=" -Wno-unused-parameter -Wno-missing-field-initializers"
KCFLAGS+=" -Wno-type-limits -Wno-switch -Wno-parentheses"
KCFLAGS+=" -Wno-unused-variable -Wno-unused-function"

# Kernel preprocessor defines
# _KERNEL is the illumos convention; __KERNEL__ is what OpenZFS checks
KDEFS="-D_KERNEL -D__KERNEL__ -D_SYSCALL32 -D_SYSCALL32_IMPL -D_ELF64"
KDEFS+=" -D_DDI_STRICT -Dsun -D__sun -D__SVR4 -D__illumos__"
KDEFS+=" -DBUILDING_ZFS -DHAVE_CONFIG_H"

# Include paths (order matters: OpenZFS SPL shadows illumos headers via #include_next)
# Note: do NOT -include ccompile.h for kernel mode; it breaks #include_next.
# Instead, it gets included naturally through the zfs_context.h header chain.
KINC="-include $OZ/zfs_config.h"
KINC+=" -include $OZ/include/sys/simd_config.h"
KINC+=" -I$OZ/include/os/illumos/spl"
KINC+=" -I$OZ/include/os/illumos/zfs"
KINC+=" -I$OZ/include"
KINC+=" -I$UTS/common"
KINC+=" -I$UTS/intel"
KINC+=" -I$UTS/i86pc"
KINC+=" -I$UTS/common/fs/zfs"

pass=0
fail=0
skip=0

compile_file() {
    local src="$1"
    local name=$(basename "$src" .c)
    local obj="$OBJDIR/${name}.o"

    printf "  %-40s " "$name"
    if $CC $KCFLAGS $KDEFS $KINC -c -o "$obj" "$src" 2>"$OBJDIR/${name}.err"; then
        echo "OK"
        pass=$((pass + 1))
    else
        echo "FAIL"
        fail=$((fail + 1))
        head -5 "$OBJDIR/${name}.err" | sed 's/^/    /'
    fi
}

echo "=== OpenZFS kernel module compile test ==="
echo "Compiler: $CC"
echo "UTS:      $UTS"
echo "OpenZFS:  $OZ"
echo ""

if [ "$1" = "--file" ] && [ -n "$2" ]; then
    echo "--- Testing single file ---"
    compile_file "$2"
else
    # Test a representative set of files
    echo "--- SPL kernel files (module/os/illumos/spl/) ---"
    for f in "$OZ"/module/os/illumos/spl/*.c; do
        [ -f "$f" ] && compile_file "$f"
    done
    [ $pass -eq 0 ] && [ $fail -eq 0 ] && echo "  (no files yet)"

    echo ""
    echo "--- ZFS OS kernel files (module/os/illumos/zfs/) ---"
    for f in "$OZ"/module/os/illumos/zfs/*.c; do
        [ -f "$f" ] && compile_file "$f"
    done
    [ $pass -eq 0 ] && [ $fail -eq 0 ] && echo "  (no files yet)"

    echo ""
    echo "--- Platform-neutral ZFS files (module/zfs/) sample ---"
    for f in spa_misc.c arc.c dmu.c zio.c txg.c; do
        compile_file "$OZ/module/zfs/$f"
    done
fi

echo ""
echo "=== Results: $pass passed, $fail failed ==="
