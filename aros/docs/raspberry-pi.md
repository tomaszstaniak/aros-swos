# AROS Raspberry Pi build

The available native SDK is AROS 20260822 Raspberry Pi AArch64, ABIv1. This is
separate from x86_64 ABIv11. The compiler wrapper selects
`aarch64-unknown-aros`, the large code model and the matching compiler-rt.
No Linux or hosted Macaros libraries are used.

Verified 2026-10-23:

| Component | Result |
|---|---|
| GNU libiconv 1.19/libcharset | Static AArch64 libraries built; real conversion probe links |
| SDL 3.4.12 | All 187 AROS static-library units compile; window/image/input probe links |
| Selected OpenSWOS C engine, SDL3, offline | Complete game compiles and links; ELF64 little-endian AROS AArch64 verified |
| Native data converter | CLI swosdata compiles/links as AROS AArch64; conversion on Pi unverified |
| Native C++ / GGPO networking | AROS libc++, libc++abi and libunwind built against the native Pi SDK; container/exception probe and NETPLAY=1 game link pass. Native execution remains open |
| Native execution | Open: Pi pool slot is not ready for ordinary tests; input/results exchange acceptance is incomplete |

The offline binary is a build milestone. It does not establish game execution,
controls, audio or performance on Pi. The v11 complete network-enabled build
continues to compile with its own matching dependencies.

## Sources and dependencies

`dependencies.lock.json` pins the AROS contrib source and archive/patch hashes.
Get the pristine SDL 3.4.12 archive and apply `patches/sdl3/series` with `patch -p1`
in its root, in order. The first patch is the AROS contrib port; the next three
are imported unchanged from OpenLoco with provenance beside each patch. Patch
0005 allocates globally unique SDL joystick instance IDs in the AROS backend;
its collision regression is verified on x86_64 ABIv1. The
reconstructed C/header/template sources match the built snapshot exactly.

Use `SDL3/main` from the pinned AROS contrib revision for `--contrib`; its
`mmakefile.src`, `SDL3_static.c` and `SDL3_intern.h` are required. The script reads
all FILES blocks from that makefile rather than substituting a different backend.
Use GNU libiconv 1.19 with `patches/libiconv/series` applied. Supply source paths;
the build script does not download or modify them.

With the SDK, compiler tools and reconstructed sources available:

```sh
python3 scripts/build-c-deps.py \
  --cc "$RPI_TOOLS/aarch64-aros-clang" \
  --ar "$RPI_TOOLS/aarch64-aros-ar" \
  --ranlib "$RPI_TOOLS/aarch64-aros-ranlib" \
  --sdk "$RPI_SDK" \
  --sdl-source "$SDL3_SRC" \
  --contrib "$AROS_CONTRIB/SDL3/main" \
  --iconv-source "$ICONV_SRC" \
  --prefix build/pi-deps/prefix --out build/pi-deps/objects

python3 scripts/build-c-game.py \
  --arch aarch64 --abi v1 --netplay off \
  --cc "$RPI_TOOLS/aarch64-aros-clang" --sdk "$RPI_SDK" \
  --sdl-prefix build/pi-deps/prefix --out build/pi-game
```

Output directories and prefix must be separate from each other and all inputs.
The dependency manifest records compiler, commands, source/header hashes and
output hashes. The game manifest records arch, declared ABI, source, patches and
library hashes; its builder verifies ELF class, AROS OSABI and machine type.
Original SWOS data is supplied separately, as for the v11 port.

`--netplay off` above remains an offline reference. The full build uses the
separately built native C++ runtime described below; no hosted libraries are used.
The SDK label, code compilation and ELF checks cannot establish runtime ABI
compatibility; that still needs a native run on the matching installation.

## Native data converter

The CLI converter needs C only. The upstream setup CMake project enables C++
for its optional Slint GUI, so cross-compiler detection uses a static test
library; the actual converter executable still undergoes a full native link.

```sh
cmake -S work/openswos-c/tools/swosdata -B build/pi-setup \
  -DCMAKE_SYSTEM_NAME=Generic -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
  -DCMAKE_C_COMPILER="$RPI_TOOLS/aarch64-aros-clang" \
  -DCMAKE_CXX_COMPILER="$RPI_TOOLS/aarch64-aros-clang++" \
  -DSD_WITH_APP=OFF
cmake --build build/pi-setup -j4
```

This builds `bin/swosdata`; it does not port the Slint GUI or establish native
ADF conversion on Pi. The same CLI/path implementation was tested on v11.

## Native C++ runtime and full network build

The base toolchain contains a C++ compiler, but does not stage its standard
library. This is a toolchain gap, rather than an engine requirement to disable
multiplayer. `scripts/build-pi-cxx.py` builds static libc++, libc++abi and
libunwind from the available AROS-patched LLVM20.1 source snapshot. It validates
all12155 source files through the canonical manifest hash in
`patches/pi-cxx/source.json`. Supply that snapshot; the script neither downloads
nor substitutes an unpatched LLVM release. Source provenance is a local AROS
build snapshot, not a newly established clean upstream archive plus patch series.

```sh
python3 scripts/build-pi-cxx.py --source "$AROS_LLVM_TREE" \
  --toolchain "$RPI_TOOLCHAIN_ROOT" --out build/pi-cxx
python3 scripts/build-c-game.py --arch aarch64 --abi v1 --netplay on \
  --cc "$RPI_TOOLS/aarch64-aros-clang" --cxx build/pi-cxx/native-cxx.py \
  --sdk "$RPI_SDK" --sdl-prefix build/pi-deps/prefix --out build/pi-network
```

The output must be fresh. Headers, archives, commands, source manifest and
container/throw-catch probe are retained under the owned output. No shared SDK
files are modified. Threads, streams and exception support remain enabled;
`_LIBCPP_PROVIDES_DEFAULT_RUNE_TABLE` selects libc++'s supplied portable table.

The SDK `sys/cdefs.h` first requests POSIX2008 for GNU extensions, then its
XOPEN>=600 branch incorrectly reduces it to POSIX2001 even for XOPEN700.
A source-hash-checked private header overlay adds the XOPEN700 case. Without
it, including socket headers before libc++ hides declarations needed by locale.
Separately, engine patch13 includes pthread before scoped socket mappings:
later libc++ headers must not replace `swos_net_sendto/recvfrom` with SDK macros
that use a global SocketBase. Full Pi link failed on that global before the fix.

Compile/link acceptance is distinct from native execution: the probe must still
run with return0 and print `native-cxx 6` on the matching native installation.
The complete game, networking match, controls and performance also remain
unverified on Pi while ordinary pool exchange is unavailable.
