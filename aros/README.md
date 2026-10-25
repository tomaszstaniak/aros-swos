# OpenSWOS for AROS

This fork ports benbaker76/OpenSWOS at `6b5997b68c5bc84dd4905a6eeab44fd412e7821d` to AROS using SDL3. The `aros` branch contains the game changes. Ordered patches, dependency pins and build scripts are preserved here for rebuilding without modifying a clean upstream checkout.

Release [0.1.0-rc3](https://github.com/tomaszstaniak/aros-swos/releases/tag/v0.1.0-rc3) provides x86_64 ABIv11 and experimental Raspberry Pi AArch64 ABIv1 packages. CPU rendering is the default, with optional OpenGL. Accelerated Alt+Enter recreates the renderer safely across native-window changes; v11 tests and GT220 user feedback passed. Original SWOS data is not included: use `swosdata` to import your own copy as described in [the player instructions](packaging/ReadMe.txt).

## Rebuild

Use a matching AROS compiler and SDK. Paths below are your own installations; do not mix ABIv1 and ABIv11 libraries.

1. Follow [patching.md](docs/patching.md) from this `aros/` directory to create the pinned `work/openswos-c` checkout and initialize GGPO-X and swosgfx. Apply all three corresponding patch series.
2. Extract SDL3 3.4.12 and GNU libiconv 1.19 from the accompanying sources asset; verify their archive hashes against `dependencies.lock.json`. Apply `patches/sdl3/series` and `patches/libiconv/series` in order to their respective roots with `patch -p1`. The exact AROS contrib build glue is in `contrib-sdl3/`.
3. Build dependencies using `python3 scripts/build-c-deps.py --help`. Supply the patched source roots, contrib glue directory, ABI-matched compiler/binutils and SDK. Choose separate private output and prefix directories. The script verifies source manifests before compiling.
4. Build the game using `python3 scripts/build-c-game.py --source work/openswos-c --cc "$CC" --cxx "$CXX" --sdk "$SDK" --sdl-prefix "$PREFIX" --abi v11 --netplay on --out build/game`. It preserves a complete local build receipt.
5. Build the native converter with `make -C work/openswos-c/tools/swosdata CC="$CC" ARCHFLAGS="--sysroot=$SDK" OUT="$PWD/build/setup"`; the output is `build/setup/swosdata`. See `--help` for packaging options.

The sources asset includes the patched GGPO-X/swosgfx sources and the complete dependency source archives, notices and glue. Its game source tree omits original-game reverse-engineering reference dumps, which are not build inputs. The public fork retains upstream history; the release game archive contains no original SWOS installation data.

AROS Raspberry Pi has a separate build path described in [raspberry-pi.md](docs/raspberry-pi.md). The experimental Pi release has an offline QEMU intro/menu/keyboard/rendered-replay smoke test with dummy audio. Real hardware, audio and networking still need testers. The Pi sources asset supplies the complete pinned LLVM runtime snapshot under `llvm-runtimes/`; use it with `scripts/build-pi-cxx.py`, then pass the generated compiler wrapper to the complete game builder.
