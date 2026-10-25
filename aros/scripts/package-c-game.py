#!/usr/bin/env python3
"""Stage an AROS test candidate from a verified build, without original SWOS data."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--build', required=True, type=Path)
p.add_argument('--converter', required=True, type=Path)
p.add_argument('--out', required=True, type=Path)
p.add_argument('--source', type=Path, default=ROOT / 'work/openswos-c')
a = p.parse_args()
source = a.source.resolve()
receipt = json.loads((a.build / 'build.json').read_text())
pin = json.loads((ROOT / 'upstreams.json').read_text())['repositories']['openswos-c']['commit']
if receipt['upstream'] != pin or receipt['backend'] != 'sdl3':
    p.error('requires the selected pinned C upstream and SDL3')
if (receipt['arch'], receipt['abi']) not in {('x86_64', 'v11'), ('x86_64', 'v1'), ('aarch64', 'v1')} or receipt['netplay'] != 'on':
    p.error('requires a complete supported native networking build')
pi = receipt['arch'] == 'aarch64'
machine = 183 if pi else 62
if subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=source, text=True).strip() != pin:
    p.error('source checkout differs from the selected pin')
if a.out.exists():
    p.error('output already exists; choose a fresh candidate filename')
for name, digest in receipt['source_sha256'].items():
    if hashlib.sha256((source / name).read_bytes()).hexdigest() != digest:
        p.error('source no longer matches the build receipt: ' + name)
files = {}
for name, path in [('SWOS', a.build / 'SWOS'), ('swosdata', a.converter)]:
    data = path.read_bytes()
    if len(data) < 64 or data[:6] != b'\x7fELF\x02\x01' or data[7] != 15 or int.from_bytes(data[18:20], 'little') != machine:
        p.error(name + ' must be an ELF64 AROS ' + receipt['arch'] + ' executable')
    files[name] = data
    if name == 'SWOS' and receipt['sha256'].get(str(path.resolve())) != hashlib.sha256(data).hexdigest():
        p.error('game binary differs from its build receipt')
# Only immutable, tracked upstream runtime resources are distributable inputs.
# Never recurse over a developer's bin/assets/save directories.
tracked = subprocess.check_output(['git', 'ls-files', '-z', 'bin'], cwd=source).decode().split('\0')
for name in tracked:
    runtime_dirs = ('data', 'data_json', 'setup', 'grafs', 'grafs_aga', 'grafs_dos',
                    'grafs_2020', 'grafs_arcade', 'grafs_jag', 'sound', 'sound_dos')
    if (any(name.startswith('bin/' + d + '/') for d in runtime_dirs) and
            not name.startswith('bin/data_json/teams/')):
        files[name[4:]] = subprocess.check_output(['git', 'show', pin + ':' + name], cwd=source)
for name in ['LICENSE', 'docs/CONTROLS.md', 'docs/NETPLAY.md', 'docs/SAVE.md']:
    files['docs/' + Path(name).name] = (source / name).read_bytes()
for sub in ['ggpo-x', 'enet', 'miniz', 'swosgfx', 'opal', 'opl_models']:
    directory = source / 'third_party' / sub
    license_file = next((directory / name for name in ('LICENSE', 'LICENSE.txt') if (directory / name).is_file()), None)
    if license_file:
        files['licenses/' + sub + '.txt'] = license_file.read_bytes()
for directory in ['openswos-c', 'ggpo-x', 'swosgfx', 'sdl3', 'libiconv']:
    patch_dir = ROOT / 'patches' / directory
    series = patch_dir / 'series'
    names = ['series']
    for line in series.read_text().splitlines():
        name = line.strip()
        if not name or name.startswith('#'):
            continue
        if Path(name).name != name:
            p.error('patch series must contain plain filenames')
        names += [name, str(Path(name).with_suffix('.json'))]
    for name in names:
        files['port-source/patches/' + directory + '/' + name] = (patch_dir / name).read_bytes()
for name in ['scripts/build-c-game.py', 'scripts/build-c-deps.py', 'upstreams.json', 'dependencies.lock.json', 'docs/patching.md', 'docs/raspberry-pi.md']:
    files['port-source/' + name] = (ROOT / name).read_bytes()
for license_file in sorted((ROOT / 'packaging/licenses').glob('*.txt')):
    files['licenses/' + license_file.name] = license_file.read_bytes()
files['Start'] = b'FailAt 21\nStack 2000000\nSWOS --renderer=software --fullscreen=off --scale=3 --scaling=sharp\n'
files['Start-Fullscreen'] = b'FailAt 21\nStack 2000000\nSWOS --renderer=software --fullscreen=desktop --scaling=sharp\n'
if pi:
    for name in ('Start', 'Start-Fullscreen'):
        files[name] = files[name].replace(b'Stack 2000000\n', b'Stack 2000000\nSet SDL_AUDIODRIVER dummy\nSet SWOS_AUDIO_OFF 1\n')
    for name in ['scripts/build-pi-cxx.py', 'tests/native/check_pi_tls.py', 'tests/native/pi_cxx_runtime_probe.cpp']:
        files['port-source/' + name] = (ROOT / name).read_bytes()
    for path in sorted((ROOT / 'patches/pi-cxx').iterdir()):
        if path.is_file():
            files['port-source/patches/pi-cxx/' + path.name] = path.read_bytes()
files['ReadMe.txt'] = (ROOT / ('packaging/ReadMe-Pi.txt' if pi else 'packaging/ReadMe.txt')).read_bytes()
files['port-source/scripts/package-c-game.py'] = Path(__file__).read_bytes()
files['port-source/packaging/ReadMe.txt'] = files['ReadMe.txt']
# Full compiler commands and SDK paths stay in the private build receipt.
public_receipt = {k: v for k, v in receipt.items() if k != 'command'}
public_receipt['sha256'] = {Path(name).name: digest for name, digest in receipt['sha256'].items()}
for name in ('compiler', 'cxx'):
    if public_receipt.get(name):
        public_receipt[name] = '\n'.join(line for line in public_receipt[name].splitlines()
                                          if not line.startswith('InstalledDir:')) + '\n'
files['build.json'] = json.dumps(public_receipt, indent=2).encode() + b'\n'
manifest = {name: hashlib.sha256(data).hexdigest() for name, data in sorted(files.items())}
files['SHA256SUMS'] = ''.join(digest + '  ' + name + '\n' for name, digest in manifest.items()).encode()
root = 'OpenSWOS-' + ('aarch64-' if pi else '') + receipt['abi']
a.out.parent.mkdir(parents=True, exist_ok=True)
with zipfile.ZipFile(a.out, 'x', compression=zipfile.ZIP_DEFLATED) as archive:
    for name, data in sorted(files.items()):
        info = zipfile.ZipInfo(root + '/' + name, (2026, 10, 23, 0, 0, 0))
        info.compress_type = zipfile.ZIP_DEFLATED
        info.create_system = 3
        info.external_attr = (0o100755 if name in ('SWOS', 'swosdata', 'Start', 'Start-Fullscreen') else 0o100644) << 16
        archive.writestr(info, data)
with zipfile.ZipFile(a.out) as archive:
    assert archive.testzip() is None
    for name, digest in manifest.items():
        assert hashlib.sha256(archive.read(root + '/' + name)).hexdigest() == digest
print(str(a.out), len(files), 'files; all archived hashes verified; no original game data')
