# Selected C engine completion probes

These probes exercise benbaker76/OpenSWOS production code. Earlier pitch/player
probes and the root CMake/C# tests are historical and do not establish acceptance
of this engine. Track remaining workflows in [the checklist](../../docs/port-completion.md).

- `save_write_errors.c` includes the two real writers, uses real files, and injects
  only short `fwrite` and failed `fclose`. Its argument is an owned output drawer.
- `heap_alignment_probe.c` checks fundamental alignment and byte preservation
  through allocation and grow/shrink. For a full-game link, define
  `SWOS_HEAP_LINK_GAME`; otherwise it includes production `swos_io.c` directly.
- `game_persistence_probe.c` runs `write` and `read` in separate processes for
  tactics/binary I/O. `career-roundtrip [INPUT [OUTPUT]]` loads a real menu-created
  CAR/SEA and re-saves it. Defaults are PORTCAR.CAR and PORTRE.CAR in the save
  drawer beside the native executable. Keep tests in an owned drawer.
- `compare_career_saves.py BEFORE AFTER` compares all semantic JSON fields. Only
  the four raw derived competition-context buffers explicitly exempted by
  upstream are excluded. Canonical competition slots, lists and scalar state
  remain compared; missing fields and malformed input are errors.
- `game_input_probe.c` sends SDL events through the actual game input decoder:
  eight directions, fire/charge/release, keyboard plus virtual pad, two virtual
  pads and disconnect. This does not replace physical-device/full-match tests.
- `preboot_load_probe.c` reads a real file before framebuffer initialization,
  including alternate/null destinations and a missing optional asset.
- `socket_wait_probe.c` exercises real host socket readiness via select or poll.
  `aros_network_probe.c` checks the native AROS socket boundary, UDP, TCP, a worker
  pthread, and socket closure by its TLS destructor.
- `setup_path_probe.c` checks converter path joining. Its forced-AROS host build
  establishes string semantics only; native ADF conversion is checked separately.
- `career-menu-scenarios.json` records observed menu routes and required context.
  Inspect the resulting screens. Host turbo frame timing is not a native contract;
  the normal native route explicitly skips the loader first.

Link full-game probes against every production object except the **top-level**
`obj/main.o`, with the same backend, ABI and link flags as the game. Keep GGPO's
`obj/ggpo/main.o`: it implements library APIs. The native network-enabled build
uses the matching C++ linker. Never mix x86, AArch64, v11 and v1 libraries.

No original game assets or save fixtures are bundled in these tests. Real
menu-created CAR/SEA and full-match SRP fixtures stay in private evidence.

Additional persistence probes:

- `user_files_probe.c`: run `team-write`, `team-read`, `options-write`,
  `options-read`, `edt-write`, `edt-read`, `hil-write`, `hil-read` in separate
  processes. Uses real custom-team/editor/settings/highlight APIs and owned
  PORT/CUSTOM filenames. HIL uses a two-frame synthetic scene with metadata
  from the privately supplied PORTMATCH.SRP; it does not establish goal capture.
- `game_persistence_probe.c` also supports `preset-roundtrip`, `diy-roundtrip`
  and `highlights-roundtrip`, with optional input/output names as for career.
- `python3 tests/native/run_persistence_probes.py OWNED_DRAWER --assets OWN_ASSETS --competition-layout LAYOUT_JSON`
  runs 13 host production-code checks and writes a receipt with binary hashes.
  Every read is a separate process. Requires the isolated host binaries and
  private menu-created fixtures; it overwrites only named test files. Its exit
  is nonzero if any check fails or parsed JSON still needs review. In particular,
  successful internal PRE verification does not override raw JSON differences.
  `competition_layout_probe.c`, compiled against the selected production headers,
  produces LAYOUT_JSON. `compare_competition_saves.py` validates both overlapping
  raw snapshots and their typed name before normalizing only unused bytes after
  the name terminator. The World Cup fixture differs by the four-byte unused
  `MENT` suffix after `WORLD CUP\0`, duplicated in the overlapping view. All
  other bytes/fields remain checked; negative regression cases cover authored
  state changes and mismatched aliases. Native and normal-menu progression
  acceptance remain separate requirements.

- `json_byte_array_probe.c` compiles the real production serializer in place of
  `obj/save_format_json.o` and counts calls to its rescanning iterator. It compares
  the actual private byte-array loader against prior behavior for numbers,
  nonnumbers, nested values, empty/missing arrays, destination offsets and bounds.
  The 1024-element regression is RED with1024 rescans before patch12, GREEN with0
  afterwards. This counts algorithmic work; it imposes no machine timing budget.

- `iconv_probe.c` checks a real UTF-8 to ISO-8859-2 conversion (U+017C). Link
  against the target dependency; a successful cross-link alone is not a runtime
  pass. See [the Pi build recipe](../../docs/raspberry-pi.md).

- `pi_cxx_runtime_probe.cpp` includes socket headers before libc++ and exercises
  containers, ownership and exception unwinding. The isolated native runtime
  builder compiles/links it; a native run is still required.

- `sdl_environment_probe.c` checks the SDL2-call boundary against both libc
  `getenv` and SDL3's environment, including overwrite and empty values.
  Pass an owned log path. Native v11 RED5 mismatches -> GREEN0; the non-AROS
  host adapter also passes. Link native SDL3 probes with `src/swos_sdl3.c`.
- `run_with_log.c` launches one native DOS command with both output streams
  captured to an owned log; its exit status is the command status. This is
  test infrastructure, not an alternate game entry point.

- `check_process_shutdown.py LOG RC [--expected-return N] [--require MARKER]`
  rejects native unfreed-signal/address-access warnings even when replay playback
  completed and the Shell received RC0. Use real captured output and a separate
  return-code file; absence of an execution marker is a failure.
- `worker_shutdown_probe.c` includes the real main/report implementations and
  links all other production objects. Against an explicitly supplied owned
  loopback receiver, it checks that native cleanup leaves zero upload workers.
  `report_shutdown_receiver.py` accepts only its synthetic SHUTDOWN match ID,
  retains no report payload, and delays acknowledgement to exercise exit while
  sending. Neither test contacts the public matchmaker or reads user logs.
- `checkpoint_shutdown_probe.c` includes the real main/replay implementations
  and checks that exit joins an actual checkpoint and writes a byte-identical
  SRP file. Its one-frame data is synthetic. Link without top-level main.o and
  replay_v2.o; retain GGPO's own main.o. The upload probe instead replaces
  top-level main.o and match_report.o. Match target/backend/ABI link flags.

- `career_transfer_probe.c` links current production objects except the root
  main.o. With the private NATFREE fixture, INPUT.CAR OUTPUT.CAR generates a
  real offer and checks sale accounting; INPUT.CAR --write-offer writes
  OFFER.CAR before acceptance for normal-menu tests. SOLD.CAR --read-sold
  checks the documented500,000 host-sale fixture in a fresh process. Run with
  the production data_json tree in the working drawer. Host tests alone do
  not establish native menu acceptance; see docs/native-acceptance-20261024.md.

- `build_season_driver.py --build-log BUILD/build.log --out FRESH_TEST_DIR`
  replaces only the no-op upstream `MenuInputInjectPoint` in a private copy
  and links `career_season_driver.c` against the matching production objects.
  No production source or object is overwritten. It loads private
  save/INPUT.CAR from a cold process, chooses visible RESULT/NEXT callbacks,
  sends player fire on the squad screen and continues at the season-end hub.
  It saves END.CAR and AFTER.CAR only after returning to the next season's
  contest menu. Run with the normal resource/data_json tree and explicit
  SWOS_HARNESS_TURBO=1, SWOS_AUDIO_OFF=1,
  SWOS_TEST_NO_INJURIES_CARDS=1, SWOS_KEY_SCRIPT=50:Return.
  This tests bounded competition/menu/save progression, with injuries/cards
  disabled; it does not establish normal keyboard or performance acceptance.
  The first read and continuation use production SaveJson_LoadCareer and
  StartContest. The menu-step limit is3000; use an external wall-time limit
  too. Host dummy SDL drivers are optional; native runs can show the window.

- `check_startup_fullscreen.py SOURCE` compiles the production startup-bank
  check, fatal error and fullscreen-toggle functions with deterministic boundary
  doubles. It checks absent/partial/OCS/mixed AGA banks, failure exit, blocking
  accelerated native-window mutation and preserving CPU toggle roundtrips.
  It does not exercise a real GL context or replace native GUI acceptance.
