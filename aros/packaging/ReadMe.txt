OpenSWOS for AROS 0.1.0-rc2 - SDL3

Use the ZIP for your ABI: v11 and v1 are different builds. Extract into a
writable game drawer, then open a Shell and CD to that drawer.

FIRST INSTALL
You need your own SWOS 96/97 data. This archive contains no original game data.
Extract your two Amiga disk images from their ZIPs first. In the game drawer:

  Stack 2000000
  swosdata --adf "Work:Disks/SWOS1.adf" --adf "Work:Disks/SWOS2.adf" --out assets

Replace those two example paths with your own. Keep data/, setup/ and data_json/
from this ZIP beside SWOS: the converter and game need these runtime resources.
Do not delete assets/ after conversion. Alternatively, convert your own PC copy:

  swosdata --dos "Work:Games/OriginalSWOS" --out assets

START
  Execute Start
or, for fullscreen with sharp pixel scaling:
  Execute Start-Fullscreen

The CPU renderer is the default. OpenGL is optional: start SWOS with
--renderer=accel instead of --renderer=software to compare on your hardware.
Alt+Enter changes fullscreen. With acceleration the renderer and textures are
recreated before the native window is replaced; if GL recreation fails, the
game tries the CPU renderer. This path passed v11 softpipe tests; confirmation
on real GPU drivers is still needed. Start forces windowed mode and
Start-Fullscreen forces desktop fullscreen, independently of SWOS.ini.
A missing bootstrap graphics bank now shows an import-data message and exits.
VSync depends on the SDL3/driver path; this candidate does not claim tear-free
presentation. Sound needs an AHI device configured in AROS.

SAVES / UPGRADES
Settings are in SWOS.ini and saves/replays in save/ beside the executable.
Back up both before upgrading. Extract updates into a new drawer and copy your
assets, settings and saves there; these are never bundled in a clean release.

CONTROLS
Cursor keys move; CTRL or Return fires/confirms. Your own keys can be assigned
in OPTIONS -> CONTROLS -> REDEFINE KEYS. See docs/CONTROLS.md.
Local two-player supports keyboard + pad, or two pads; player 2 has no keyboard.
The title-screen fire press selects player 1's pad. Full physical two-player
match acceptance is pending release testing. Network play is compiled in;
see docs/NETPLAY.md. Internet play depends on the server and network setup.

PLEASE REPORT
Build ABI, machine/GPU, input devices and the steps to reproduce a problem.
For two-player testing, check independent movement/fire, a complete match,
half-time, disconnect/reconnect and return to menus. Preserve SWOS.ini and
any replay from save/ when reporting a gameplay problem.

STATUS
This is an early AROS release.
Pi has a separate experimental package. Silent startup/menu and cached apkg
installation were checked in QEMU; real Pi/audio/full-match checks remain open.
Pi is not included in this ZIP.
The Slint setup GUI is not ported; swosdata is the native command-line setup.
Source pins, reproducible patches and build scripts are under port-source/.
OpenSWOS is GPLv3; see docs/LICENSE and third-party notices under licenses/.
