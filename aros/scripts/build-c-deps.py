#!/usr/bin/env python3
"""Build private AROS SDL3/libiconv dependencies from supplied source snapshots.

No downloads, SDK writes or host library fallback. Supply patched AROS SDL3
sources, its contrib file list/static entry point, and GNU libiconv sources.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

p = argparse.ArgumentParser(description=__doc__)
for name in ('cc', 'ar', 'ranlib', 'sdk', 'sdl-source', 'contrib', 'iconv-source', 'prefix', 'out'):
    p.add_argument('--' + name, required=True, type=Path)
p.add_argument('--jobs', type=int, default=4)
a = p.parse_args()
# Preserve compiler wrapper names: resolving an aarch64-aros-clang++ symlink
# to cc.sh changes how that wrapper chooses its language driver.
for name in vars(a):
    if isinstance(getattr(a, name), Path):
        setattr(a, name, getattr(a, name).absolute() if name in ('cc', 'ar', 'ranlib') else getattr(a, name).resolve())
if a.jobs < 1:
    p.error('--jobs must be positive')
for output in (a.out, a.prefix):
    for source in (a.sdk, a.sdl_source, a.contrib, a.iconv_source):
        if output == source or source in output.parents or output in source.parents:
            p.error('output overlaps a read-only input: ' + str(output))
if a.out == a.prefix or a.out in a.prefix.parents or a.prefix in a.out.parents:
    p.error('--out and --prefix must be separate directories')
a.out.mkdir(parents=True, exist_ok=True)
a.prefix.mkdir(parents=True, exist_ok=True)
commands = []

def run(cmd, cwd, log, env=None):
    commands.append(dict(command=cmd, cwd=str(cwd), log=log))
    with (a.out / log).open('w') as f:
        subprocess.run(cmd, cwd=cwd, env=env, stdout=f, stderr=subprocess.STDOUT, check=True)

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def source_manifest(root):
    return {str(f.relative_to(root)): sha(f) for f in sorted(root.rglob('*'))
            if f.is_file() and f.suffix in ('.c', '.h', '.in')}

lock = json.loads((Path(__file__).resolve().parents[1] / 'dependencies.lock.json').read_text())
sdl_sources = source_manifest(a.sdl_source)
iconv_sources = source_manifest(a.iconv_source)
for kind, manifest in (('sdl3', sdl_sources), ('libiconv', iconv_sources)):
    digest = hashlib.sha256(json.dumps(manifest, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    if digest != lock[kind]['source_manifest_sha256']:
        p.error(kind + ' sources differ from the pinned reconstructed snapshot')
for name, expected in lock['aros_contrib']['glue_sha256'].items():
    if sha(a.contrib / name) != expected:
        p.error('contrib build glue differs from the pin: ' + name)

target = subprocess.check_output([str(a.cc), '-dumpmachine'], text=True).strip()
if 'aros' not in target:
    p.error('compiler must target AROS: ' + target)
build = a.out / 'iconv'
build.mkdir(exist_ok=True)
env = dict(os.environ, CC=str(a.cc), AR=str(a.ar), RANLIB=str(a.ranlib),
           CFLAGS='--sysroot=' + str(a.sdk) + ' -O2', CPPFLAGS='', LDFLAGS='')
build_host = subprocess.check_output([str(a.iconv_source / 'build-aux/config.guess')], text=True).strip()
run([str(a.iconv_source / 'configure'), '--host=' + target, '--build=' + build_host,
     '--disable-shared', '--enable-static', '--disable-nls', '--disable-rpath',
     '--prefix=' + str(a.prefix)], build, 'iconv-configure.log', env)
run(['make', '-j' + str(a.jobs), 'lib/localcharset.h'], build, 'iconv-charset.log', env)
run(['make', '-C', 'lib', '-j' + str(a.jobs)], build, 'iconv-library.log', env)
for source, dest in (
    ('lib/.libs/libiconv.a', 'lib/libiconv.a'),
    ('libcharset/lib/.libs/libcharset.a', 'lib/libcharset.a'),
    ('include/iconv.h.inst', 'include/iconv.h'),
    ('libcharset/include/localcharset.h.inst', 'include/localcharset.h'),
    ('libcharset/include/libcharset.h.inst', 'include/libcharset.h'),
):
    dst = a.prefix / dest
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(build / source, dst)

mm = a.contrib / 'mmakefile.src'
lines = mm.read_text().splitlines()
blocks = []
for i, line in enumerate(lines):
    if re.match(r'FILES\s*[:+]?=', line):
        for cont in lines[i:]:
            blocks.append(cont)
            if not cont.rstrip().endswith('\\'):
                break
files = re.findall(r'\$\(ARCHSRCDIR\)/(\S+)', '\n'.join(blocks))
if len(files) < 150:
    raise RuntimeError('unexpected contrib SDL3 file list')
objdir = a.out / 'sdl3'
objdir.mkdir(exist_ok=True)
units = [(a.sdl_source / (s + '.c'), objdir / (s.replace('/', '_') + '.o')) for s in files]
units.append((a.contrib / 'SDL3_static.c', objdir / 'SDL3_static.o'))
flags = ['--sysroot=' + str(a.sdk), '-std=gnu99', '-O2', '-DSDL3_AROS_STATIC',
         '-DADATE="23.10.2026"']
flags += ['-I' + str(d) for d in (a.sdl_source / 'include',
          a.sdl_source / 'include/build_config', a.sdl_source, a.sdl_source / 'src',
          a.prefix / 'include', a.sdk / 'include')]

def compile_unit(unit):
    source, obj = unit
    cmd = [str(a.cc), *flags, '-c', str(source), '-o', str(obj)]
    r = subprocess.run(cmd, capture_output=True, text=True)
    (objdir / (obj.stem + '.log')).write_text(r.stdout + r.stderr)
    return dict(source=str(source), sha256=sha(source), command=cmd, returncode=r.returncode)

with ThreadPoolExecutor(max_workers=a.jobs) as pool:
    results = list(pool.map(compile_unit, units))
failures = [r for r in results if r['returncode']]
(a.out / 'sdl3-compile.json').write_text(json.dumps(results, indent=2) + '\n')
if failures:
    raise RuntimeError('SDL3 compilation failed: ' + ', '.join(r['source'] for r in failures))
archive = a.prefix / 'lib/libSDL3_static.a'
archive.unlink(missing_ok=True)
for i in range(0, len(units), 100):
    run([str(a.ar), 'rcs', str(archive), *[str(o) for _, o in units[i:i+100]]],
        a.out, 'sdl3-archive-' + str(i) + '.log')
headers = a.prefix / 'include/SDL3'
headers.mkdir(exist_ok=True)
for h in (a.sdl_source / 'include/SDL3').glob('*.h'):
    shutil.copy2(h, headers / h.name)
manifest = dict(target=target, compiler=subprocess.check_output([str(a.cc), '--version'], text=True),
                sdk=str(a.sdk), commands=commands, contrib_sha256=sha(mm),
                sdl_sources=sdl_sources,
                contrib_sources={str(f.relative_to(a.contrib)): sha(f)
                  for f in sorted(a.contrib.rglob('*')) if f.is_file() and f.suffix in ('.c', '.h', '.src')},
                iconv_sources=iconv_sources,
                iconv_environment={k: env[k] for k in ('CC', 'AR', 'RANLIB', 'CFLAGS', 'CPPFLAGS', 'LDFLAGS')},
                outputs={str(f.relative_to(a.prefix)): sha(f)
                  for f in sorted(a.prefix.rglob('*')) if f.is_file()})
(a.out / 'build.json').write_text(json.dumps(manifest, indent=2) + '\n')
print('Built libiconv/libcharset and SDL3 (' + str(len(units)) + ' units) for ' + target)
