# Startup and accelerated fullscreen, 2026-10-25

Two release-user reports reopened acceptance: a fresh apkg installation without
player-owned data spun on a grey window, and accelerated fullscreen toggling
could blank the window or crash the system.

`image_cmd.c` retried required sprite-bank loads forever without pumping input.
Patch19 checks the eleven bootstrap graphics banks before native SDL/audio/window startup
and reports missing data with import instructions. Required sprite-script load
failures now report once and terminate through registered native cleanup.
Optional sounds/loading screens remain optional. This is not a complete asset
integrity validator.

The pinned SDL3 AROS backend's bordered/fullscreen native-window recreation
closes the Intuition window without rebuilding the OpenGL raster attachment.
`AROS_GL_MakeCurrent` only retargets when the context identity changes. Current
contrib `45327d2cee6bf27830de1ecef816804dc9259560` still has these paths in
`SDL3/main/SDL3-3.4.12-aros.diff`. Patch19 therefore blocks live accelerated
fullscreen toggles before any SDL window mutation and explains the limitation.
CPU fullscreen is preserved. This protection is not a GL backend repair.

Mainline AROS reviewed at `827810f5f1b4bbf3b874958994fda77ed747baa0`; ABIv11
master remains `5b5fd4cfd4f039927c0d1bb9cdd7f4fc8644d23e`. Existing SDKs and
installed systems were not changed. No reusable SDL fix has been implemented
or verified for submission: native GL window/context lifecycle remains pending.

Host function regression passes and the ordered patch reconstructs exactly.
Native v11: missing-data requester appears; OK returns to Shell. Valid private
assets start the intro; CPU fullscreen roundtrip renders in both modes.
Further native checks and release receipts are recorded with this session's
evidence. Pi build/link is separate from native Pi acceptance.

Current game upstream `35cf61c90238867db75f67d7138641b7245362ec` was also
inspected: `assets_check.c` still only checks asset data versions. No equivalent
missing-bootstrap-bank preflight was found there.

Published delivery: [0.1.0-rc2](https://github.com/tomaszstaniak/aros-swos/releases/tag/v0.1.0-rc2),
public source `94037364b340d5fd381c3157506b8199907f803d`.
[Index PR41](https://github.com/tomaszstaniak/arospkg-index/pull/41) merged;
publish workflow40215311053 succeeded and both public v2/v3 indexes were read
back with ZIP SHA256 `229cc7e41c44f32ee55c75252350a05aff3322c294fab8c16f61295caf281020`.
Pi's existing package was not changed. This update did not repeat native apkg
installation or full match/network acceptance; the archive was checked with
apkg-pack and contains the exact native-tested final binary.

Final native probes: wholly absent assets and removed CJCBITS.RAW each show
the error requester before the game window; OK returns to Shell with RC20.
Restored complete assets start normally and window close returns RC0.
The full underlying GL lifecycle repair remains open, with no reusable SDL
fix implemented or verified for upstream submission. Existing package/CPU
behavior is protected by the application guard.

Subsequent Pi follow-up on the same day: the common rc2 fix was runtime-tested
and published for Pi in the same release; see raspberry-pi.md. The earlier
"Pi unchanged" statement above describes the initial x86-only publication.
New user evidence reports incorrect accelerated presentation on x86_64 with
data, while software works; that startup/resize report remains under diagnosis.
