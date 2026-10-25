# AROS Raspberry Pi build

The available native SDK is AROS 20260822 Raspberry Pi AArch64, ABIv1. This is
separate from x86_64 ABIv11. The compiler wrapper selects
`aarch64-unknown-aros`, the large code model and the matching compiler-rt.
No Linux or hosted Macaros libraries are used.

Verified 2026-10-24 (build results plus bounded native smoke):

| Component | Result |
|---|---|
| GNU libiconv 1.19/libcharset | Static AArch64 libraries built; real conversion probe links |
| SDL 3.4.12 | All 187 AROS static-library units compile; window/image/input probe links |
| Selected OpenSWOS C engine, SDL3, offline | Complete game compiles and links; ELF64 little-endian AROS AArch64 verified |
| Native data converter | Native CLI converted the owned Amiga ADFs:117 game files,15 derived AGA files and76 team files; RC0 |
| Native C++ / GGPO networking | AROS libc++, libc++abi and libunwind built against the native Pi SDK; upstream pthread-TLS fix imported; native container/exception probe prints native-cxx6 with a space before6; full NETPLAY=1 game starts with dummy audio |
| Native execution | rpi-1 offline SD smoke passed: native apkg install/verify, converter, silent intro/menu keyboard and rendered replay. USB/network/real hardware remain unverified |

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

## Native runtime checks, 2026-10-24

The rpi-1 pilot can run application smoke tests using a private directory on its stopped system SD, even though USB exchange and networking remain unaccepted. Those limitations do not prevent an offline game start. The current test left system files unchanged.

The original full Pi build was rejected by the native loader at ELF TLS relocation549. Its libc++abi snapshot predates the existing [AROS fix7654818](https://github.com/aros-development-team/AROS/commit/7654818bf1acd25b11cd840a713dc97692bfff55). The builder now applies that imported hunk to an owned overlay, retaining threads and exceptions through libc++abi's pthread fallback. No new upstream fix or submission is claimed. A native containers/throw-catch probe printed `native-cxx6` (with a space before6) and returned to Shell. Probe and complete game have no native ELF TLS relocations.

The selected SDL3 has a private environment: the old boundary did not import explicit AmigaDOS driver selections. Patch18 forwards SDL_AUDIODRIVER and SDL_VIDEODRIVER before initialization. The same native environment probe printed `missing; FAIL` against the old adapter and `dummy; PASS` against the new one. With dummy audio, the full network-enabled game displayed normal intro/title/main menu and accepted keyboard input into FRIENDLY options; a recorded match also rendered in its SDL3 window. No network session, physical pad, complete played Pi match, audio output or real-Pi performance claim follows from this smoke.

Without the working dummy override, startup wedged the QEMU pilot with the host stack inside bcm2835_dma_write. The same emulator rendered the game with dummy audio. The saved samples identify the stalled host path, not a demonstrated defect in the game's renderer or in real hardware. The experimental Pi launcher therefore selects silent audio until hardware feedback is available.

## Published experimental Pi package

[0.1.0-rc1-pi1](https://github.com/tomaszstaniak/aros-swos/releases/tag/v0.1.0-rc1-pi1) is published separately from the x86_64 build, with complete source/runtime snapshots and no original game data. [Index PR40](https://github.com/tomaszstaniak/arospkg-index/pull/40) was reviewed and merged; publish workflow40122350837 passed.

The 17,574,302-byte ZIP has SHA-256 `ca04daedb1703f2c4982bcd008d32e8c563292a4930a945bf909675d975e2f92`; SWOS has SHA-256 `6393d5886d80ac225ef7cadcd7167c75518d6e914d00f3f80a2df03c81366488`. Native apkg0.4 installed it from a preseeded cache on rpi-1 and reported246 files as installed, none changed/missing/foreign. swosdata imported the owned ADFs on Pi with RC0. The final package launcher opened normal intro/title/main menu after this fresh conversion. Network download cannot be tested on the current pilot.

The VM is stopped/released; the entire owned SWPI1024 test directory was removed from the SD after preserving evidence. No system files, SDK or clean upstream were modified.

## Shared rc2 update, 2026-10-25

The Pi build now includes the same patch19 as v11: check eleven bootstrap
banks before opening the game and fail required sprite loads once. Native
rpi-1 cached apkg install/verify of the new ZIP return0. Both an absent assets
folder and a removed CJCBITS.RAW show the missing-data requester; the normal
system Amiga+V shortcut acknowledges it and returns to Shell. This is an
AArch64 ABIv1 test on QEMU raspi3b, not real hardware acceptance.

The accelerated live-fullscreen guard is shared application code. No Pi GL
renderer or real GPU test is claimed; the Pi launcher still uses CPU rendering
and dummy audio. Sound/network/full match/physical input limitations remain.
This update belongs to the existing rc2 release, with an architecture-specific
ZIP and complete Pi sources/runtime snapshot; rc1-pi1 assets remain immutable.

Pi rc2 publication is complete: [shared release](https://github.com/tomaszstaniak/aros-swos/releases/tag/v0.1.0-rc2),
[index PR42](https://github.com/tomaszstaniak/arospkg-index/pull/42) merged,
publish40218495812 passed. Public v2/v3 indexes were read back with Pi ZIP SHA256
`a6c8376818d39fc8e8dd70fb7a4b58992e6beb185f9d59497f941826760edd77`;
the v11 asset/hash is unchanged. Native verify reported248 files as installed,
0 changed/missing/foreign. Missing and partial-data runs returned20 after the
requester; normal silent menu and CTRL into FRIENDLY options passed. Normal
window quit was not established on the pointer-limited pilot; the bounded smoke
ended by stopping the owned VM. The owned SWPI1025 SD directory was removed
before releasing rpi-1. No original assets were published.
