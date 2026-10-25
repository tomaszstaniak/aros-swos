# OpenSWOS 0.1.0-rc3 for AROS

One release contains x86_64 ABIv11 and experimental Raspberry Pi AArch64 ABIv1
packages. The v11 executable is byte-identical to the accepted accel-probe3;
Pi is rebuilt from the same source and dependency patches. Full networking
remains enabled in both binaries.

The release fixes accelerated window creation and fullscreen startup, and
replaces the temporary Alt+Enter block with a renderer/texture lifecycle that
closes GL resources before native-window replacement. Failed GL restoration
tries a CPU renderer. Start explicitly selects windowed mode; Start-Fullscreen
explicitly selects desktop fullscreen. CPU remains the default.

Native v11 tests covered eight renderer lifecycle transitions with matching
pixel readbacks, four full-game Alt+Enter transitions, menu rendering and normal
quit. The user confirmed probe3 works as required on the reported GT220 system.
Native package installation/verification is repeated for both architecture ZIPs.
The Pi pilot uses CPU and dummy audio; the game starts and renders its intro.
Pi acceleration, hardware audio, networking and real-hardware performance are
not established by that offline QEMU smoke.

No original SWOS data, imported assets, settings or saves are distributed.
Both architecture-specific ZIPs include the native swosdata importer. Complete
source archives include pinned dependencies; the Pi archive additionally keeps
the exact LLVM C++ runtime source snapshot. Earlier release assets remain
immutable.
