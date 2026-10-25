OpenSWOS for AROS Raspberry Pi - 0.1.0-rc2 (experimental)

TARGET
Native AROS AArch64 ABIv1. This is not the x86_64 build and not a Linux program.
CPU rendering is the default. The Pi package starts silently using SDL's
dummy audio driver: audio and networking still require testing on real hardware.

YOUR OWN GAME DATA IS REQUIRED
No original SWOS data, disk images, teams, music or private saves are included.
Extract your two SWOS 96/97 Amiga ADFs from their ZIPs first. From the game drawer:

  Stack 2000000
  swosdata --adf "Work:Disks/SWOS1.adf" --adf "Work:Disks/SWOS2.adf" --out assets

Use your own paths. Alternatively import your own PC copy:

  swosdata --dos "Work:Games/OriginalSWOS" --out assets

Keep the shipped data/, data_json/ and setup/ directories beside the converter.
The created assets/ directory is private game data; retain it for later starts.

MISSING GAME DATA
Startup checks the eleven bootstrap graphics banks before opening the game.
If data is absent or incomplete, acknowledge the error and import your copy.

START
  Execute Start
or:
  Execute Start-Fullscreen

The scripts set local Shell variables SDL_AUDIODRIVER=dummy and SWOS_AUDIO_OFF=1.
To try configured AHI on real hardware, use a fresh Shell, set Stack 2000000,
and start SWOS --renderer=software directly. AHI startup hung the QEMU pilot;
working audio is not claimed. Do not change ENVARC for this experiment.

FULLSCREEN
CPU fullscreen is supported and remains the default. The source now recreates
renderer/textures for accelerated fullscreen changes; this was tested on v11
softpipe, not on Pi. Pi accelerated rendering remains unverified. Start forces
windowed mode and Start-Fullscreen forces desktop fullscreen.

CONTROLS AND SAVES
Cursor keys move; CTRL/Return fires or confirms. Remap keys in OPTIONS/CONTROLS.
See docs/CONTROLS.md and docs/NETPLAY.md. Back up SWOS.ini and save/ before updates.
Physical pads, two-human matches and Internet/NAT on Pi remain unverified.

STATUS
Intro, menu keyboard input and a rendered replay window were checked on the
rpi-1 QEMU pilot with silent audio. Real Raspberry Pi performance is unverified.
Please report your AROS version, Pi model, selected audio/input devices, startup
log and the precise steps. Experimental means hardware feedback is still needed.

SOURCE
OpenSWOS is GPLv3; see docs/LICENSE and licenses/. The matching sources asset
includes the game, native C++ runtime source snapshot, dependency sources and
ordered patches/build scripts. No original Sensible Software data is supplied.
