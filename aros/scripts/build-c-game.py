#!/usr/bin/env python3
"""Build the selected C game with explicitly ABI-matched AROS dependencies."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
p = argparse.ArgumentParser(description=__doc__)
for name in ('cc', 'sdk', 'out'):
    p.add_argument('--' + name, required=True, type=Path)
p.add_argument('--source', type=Path, default=Path('work/openswos-c'))
p.add_argument('--jobs', type=int, default=4)
p.add_argument('--arch', choices=('x86_64', 'aarch64'), default='x86_64')
p.add_argument('--abi', choices=('v11', 'v1'),
               help='Recorded target ABI; compiler, SDK and dependencies must match')
p.add_argument('--backend', choices=('sdl2','sdl3'), default='sdl3')
p.add_argument('--sdl-prefix', type=Path)
p.add_argument('--netplay', choices=('on', 'off'), default='on',
               help='Complete build by default; off is an explicit offline reference')
p.add_argument('--cxx', type=Path, help='ABI-matched C++ compiler for GGPO-X')
a = p.parse_args()
if a.abi is None:
    if a.arch != 'x86_64':
        p.error('--abi must be explicit for aarch64; do not assume v11 on Pi')
    a.abi = 'v11'
a.source = a.source.resolve()
a.out = a.out.resolve()
a.out.mkdir(parents=True, exist_ok=True)
pin = json.loads((Path(__file__).resolve().parents[1] / 'upstreams.json').read_text())['repositories']['openswos-c']['commit']
head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=a.source, text=True).strip()
if head != pin:
    p.error('source HEAD differs from the recorded upstream pin')
if a.netplay == 'on' and not a.cxx:
    p.error('--cxx is required for the complete network-enabled build')
if a.netplay == 'on' and not (a.source / 'third_party/ggpo-x/src/include/ggponet.h').is_file():
    p.error('initialize the pinned third_party/ggpo-x submodule before building')
ggpo_pin = None
if a.netplay == 'on':
    ggpo_pin = subprocess.check_output(['git', 'ls-tree', 'HEAD', 'third_party/ggpo-x'], cwd=a.source, text=True).split()[2]
    ggpo_head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=a.source / 'third_party/ggpo-x', text=True).strip()
    if ggpo_pin != ggpo_head:
        p.error('GGPO-X HEAD differs from the pinned upstream submodule')
if a.backend == 'sdl3':
    if not a.sdl_prefix:
        p.error('--sdl-prefix is required for native SDL3')
    prefix = a.sdl_prefix.resolve()
    sdl_flags = '-DSWOS_SDL3_BUILD=1 -DSDL3_AROS_STATIC -DSDL_ENABLE_OLD_NAMES -I' + str(prefix / 'include')
    extra_library_dir = a.sdk / 'SDK/Extras/lib'
    library_dirs = [prefix / 'lib']
    if extra_library_dir.is_dir():
        library_dirs.append(extra_library_dir)
    library_dirs.append(a.sdk / 'lib')
    sdl_libs = str(prefix / 'lib/libSDL3_static.a')
    sdl_libs += ''.join(' -L' + str(d.resolve()) for d in library_dirs[:-1])
    sdl_libs += ' -lGL -liconv -lpthread'
    sdl_archive = prefix / 'lib/libSDL3_static.a'
else:
    sdl_flags = '-I' + str(a.sdk.resolve() / 'include/SDL2')
    sdl_libs = '-lSDL2 -lGL'
    sdl_archive = a.sdk / 'lib/libSDL2.a'
cmd = ['make' , '-j' + str(a.jobs), 'NETPLAY=' + ('1' if a.netplay == 'on' else '0'), 'UNAME_S=AROS',
       'UNAME_M=' + a.arch, 'X86_64=1' if a.arch == 'x86_64' else 'ARM64=1',
       'CC=' + str(a.cc.absolute()),
       'ARCHFLAGS=--sysroot=' + str(a.sdk.resolve()) + (' -m64' if a.arch == 'x86_64' else ''),
       'SDL2_CFLAGS=' + sdl_flags,
       'SDL2_LIBS=' + sdl_libs, 'OBJDIR=' + str(a.out / 'obj'),
       'TARGET=' + str(a.out / 'SWOS'), 'OUTDIR=' + str(a.out)]
if a.netplay == 'on':
    cmd += ['CXX=' + str(a.cxx.absolute()),
            # The v11 SDK has select but no poll.h. ENet already has a select
            # implementation when HAS_POLL is absent; do not invent poll stubs.
            'ENET_PLATFORM_DEFS=-DHAS_SOCKLEN_T=1 -DHAS_FCNTL=1 '
            '-DHAS_INET_PTON=1 -DHAS_INET_NTOP=1 -DHAS_MSGHDR_FLAGS=1 '
            '-I' + str(a.source / 'include')]
if a.arch == 'aarch64':
    # Upstream ARM64 selects Linux by default. This is native AROS, including
    # its linker; do not inherit Linux rpaths or a Linux runtime label.
    cmd += ['BUILD_OS=aros', 'BUILD_ARCH=aarch64']
with (a.out / 'build.log').open('w') as f:
    subprocess.run(cmd, cwd=a.source, stdout=f, stderr=subprocess.STDOUT, check=True)
header = (a.out / 'SWOS').read_bytes()[:64]
expected_machine = 183 if a.arch == 'aarch64' else 62
if (len(header) < 64 or header[:6] != b'\x7fELF\x02\x01' or header[7] != 15 or
        int.from_bytes(header[18:20], 'little') != expected_machine):
    raise RuntimeError('output is not ELF64 little-endian AROS for ' + a.arch)
files = [a.out / 'SWOS', sdl_archive]
if a.backend == 'sdl3':
    for library in ('libGL.a', 'libiconv.a', 'libpthread.a'):
        candidates = [d / library for d in library_dirs]
        found = next((f for f in candidates if f.is_file()), None)
        if found is not None:
            files.append(found)
else:
    files.append(a.sdk / 'lib/libGL.a')
diff = subprocess.check_output(['git', 'diff', '--binary'], cwd=a.source)
(a.out / 'build.json').write_text(json.dumps({
    'backend': a.backend, 'netplay': a.netplay, 'arch': a.arch, 'abi': a.abi,
    'purpose': 'complete network-enabled game' if a.netplay == 'on' else 'offline reference', 'upstream': pin,
    'command': cmd, 'patch_sha256': hashlib.sha256(diff).hexdigest(),
    'source_sha256': {str(f.relative_to(a.source)): hashlib.sha256(f.read_bytes()).hexdigest() for folder in ('src','include') for f in sorted((a.source/folder).rglob('*')) if f.is_file() and f.suffix in ('.c','.h','.inc')},
    'ggpo_pin': ggpo_pin,
    'network_source_sha256': {str(f.relative_to(a.source)): hashlib.sha256(f.read_bytes()).hexdigest() for folder in ('third_party/ggpo-x/src','third_party/enet') for f in sorted((a.source/folder).rglob('*')) if a.netplay == 'on' and f.is_file() and f.suffix in ('.c','.h','.cpp')},
    'compiler': subprocess.check_output([str(a.cc), '--version'], text=True),
    'cxx': subprocess.check_output([str(a.cxx), '--version'], text=True) if a.cxx else None,
    'sha256': {str(f): hashlib.sha256(f.read_bytes()).hexdigest() for f in files},
}, indent=2) + '\n')
print(a.out / 'SWOS')
