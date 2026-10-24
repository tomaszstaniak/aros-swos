---
type: guide
updated: 2026-10-21
---

# Reconstructing sources

Read `upstreams.json` for URL and full commit. From repository root, on a fresh clone:

```sh
git clone https://github.com/benbaker76/OpenSWOS.git upstream/openswos-c
# Replace PIN with the full commit in upstreams.json:
git -C upstream/openswos-c checkout --detach PIN
git clone --no-hardlinks upstream/openswos-c work/openswos-c
git -C work/openswos-c checkout --detach PIN
```

Apply each patch in `patches/openswos-c/series` in order with `git -C work/openswos-c apply --binary ../../patches/openswos-c/FILENAME`. Never edit upstream. Save selected deltas as binary-capable patches with rationale, full base, predecessors, target scope, actual verification and upstream status; commit patches and series in this repository. Local work-copy commits do not replace saved patches. Do not recreate existing work without preserving edits, local files, stashes, refs and reflogs and verifying reconstruction in isolation. No automated patch management tools are implemented.

The older `openswos` directories and patch series are historical and retained.
The selected primary source is `openswos-c`; do not reuse the C# pin.

## Pinned C submodules

The complete game uses GGPO; the own-data converter uses swosgfx. Initialize
only these required submodules at the parent pin, then apply their own patches:

```sh
git -C work/openswos-c submodule update --init third_party/ggpo-x third_party/swosgfx
# ggpo-x: c56fd83e920e0516e2ffc982953ec321dcf11059
# swosgfx: f55434620f82f2e469114f81d8d5e0a802d97acb
```

Apply `patches/ggpo-x/series` inside `work/openswos-c/third_party/ggpo-x`
and `patches/swosgfx/series` inside `work/openswos-c/third_party/swosgfx`.
Resolve the patch filenames from this repository root. The parent network
patch and GGPO boundary patch must be used together. The parent converter
path patch requires the swosgfx path patch. No gitlink revision is advanced;
these are explicit deltas against the recorded submodule pins.
