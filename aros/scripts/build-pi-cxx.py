#!/usr/bin/env python3
"""Build isolated native Pi C++ runtimes from the recorded AROS source snapshot.

Requires CMake/Ninja and the native AArch64 toolchain. Never installs into the
shared SDK. The emitted native-cxx.py is passed to build-c-game.py --cxx.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('source', 'toolchain', 'out'):
        parser.add_argument('--' + name, required=True, type=Path)
    parser.add_argument('--jobs', type=int, default=4)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    source, tools, out = (getattr(args, k).resolve() for k in ('source', 'toolchain', 'out'))
    for input_path in (source, tools):
        if out == input_path or out.is_relative_to(input_path) or input_path.is_relative_to(out):
            parser.error('output must be separate from all inputs')
    if out.exists():
        parser.error('use a fresh output directory')
    pin = json.loads((repo / 'patches/pi-cxx/source.json').read_text())
    manifest = {str(p.relative_to(source)): sha(p)
                for part in pin['components'] for p in sorted((source / part).rglob('*'))
                if p.is_file()}
    digest = hashlib.sha256(json.dumps(manifest, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    if len(manifest) != pin['files'] or digest != pin['canonical_manifest_sha256']:
        parser.error('runtime sources differ from the recorded snapshot')
    cdefs = tools / 'sdk/include/sys/cdefs.h'
    if sha(cdefs) != pin['sdk_cdefs_sha256']:
        parser.error('SDK cdefs.h differs; review the overlay before using another SDK')
    out.mkdir(parents=True)
    (out / 'source-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    overlay = out / 'sdk-overlay/include/sys'
    overlay.mkdir(parents=True)
    shutil.copyfile(cdefs, overlay / 'cdefs.h')
    commands = []
    def run(command, log, cwd=None):
        commands.append(command)
        (out / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n')
        with (out / log).open('w') as stream:
            subprocess.run(command, cwd=cwd, stdout=stream, stderr=subprocess.STDOUT, check=True)
    run(['patch', '-p1', '-i', str(repo / 'patches/pi-cxx/sdk-cdefs.patch')],
        'sdk-overlay.log', out / 'sdk-overlay')
    # This recorded snapshot predates the upstream AROS TLS-storage fix.
    # Apply it to an owned copy; never edit the supplied runtime tree.
    abi_source = out / 'source-overlay/libcxxabi'
    shutil.copytree(source / 'libcxxabi', abi_source)
    for name in ('libcxx', 'libunwind', 'cmake'):
        (abi_source.parent / name).symlink_to(source / name, target_is_directory=True)
    run(['patch', '-p1', '-i', str(repo / 'patches/pi-cxx/libcxxabi-aros-pthread-tls.patch')],
        'libcxxabi-tls-patch.log', abi_source)
    cmake_source = out / 'cmake-source'
    cmake_source.mkdir()
    (cmake_source / 'CMakeLists.txt').write_text(
        'cmake_minimum_required(VERSION 3.20)\nproject(AROSNativeCXX LANGUAGES C CXX ASM)\n'
        'set(LLVM_ENABLE_RUNTIMES "libunwind;libcxx;libcxxabi")\n' +
        ''.join(f'add_subdirectory("{abi_source if name == "libcxxabi" else source / name}" {name})\n'
                for name in ('libunwind', 'libcxx', 'libcxxabi')))
    flags = '-D_GNU_SOURCE -nodefaultlibs -I' + str(source / 'libcxxabi/include')
    options = dict(CMAKE_SYSTEM_NAME='Generic', CMAKE_TRY_COMPILE_TARGET_TYPE='STATIC_LIBRARY',
                   CMAKE_BUILD_TYPE='Release', CMAKE_SYSROOT=str(tools / 'sdk'),
                   CMAKE_POSITION_INDEPENDENT_CODE='OFF', CMAKE_C_FLAGS=flags,
                   CMAKE_CXX_FLAGS=flags + ' -nostdinc++ -D_LIBCPP_PROVIDES_DEFAULT_RUNE_TABLE',
                   LIBCXX_CXX_ABI='libcxxabi', LIBCXX_HAS_PTHREAD_API='ON',
                   LIBCXX_ENABLE_THREADS='ON', LIBCXX_INCLUDE_BENCHMARKS='OFF',
                   LIBCXX_INCLUDE_TESTS='OFF', LIBCXX_ENABLE_STATIC_ABI_LIBRARY='OFF',
                   LIBUNWIND_INCLUDE_TESTS='OFF', LIBCXXABI_INCLUDE_TESTS='OFF',
                   LIBCXXABI_USE_LLVM_UNWINDER='ON', LIBCXXABI_HAS_PTHREAD_API='ON')
    for key, suffix in (('C_COMPILER', 'clang'), ('CXX_COMPILER', 'clang++'),
                        ('ASM_COMPILER', 'clang'), ('AR', 'ar'), ('RANLIB', 'ranlib')):
        options['CMAKE_' + key] = str(tools / ('bin/aarch64-aros-' + suffix))
    for component, name in (('LIBCXX', 'cxx'), ('LIBCXXABI', 'cxxabi'), ('LIBUNWIND', 'unwind')):
        options.update({component + '_ENABLE_SHARED': 'OFF', component + '_ENABLE_STATIC': 'ON',
                        component + '_USE_COMPILER_RT': 'ON',
                        component + '_SHARED_OUTPUT_NAME': name + '-unused-shared'})
    build = out / 'runtime'
    run(['cmake', '-G', 'Ninja', '-S', str(cmake_source), '-B', str(build)] +
        ['-D' + k + '=' + v for k, v in options.items()], 'configure.log')
    run(['cmake', '--build', str(build), '--target', 'cxx_static', 'cxxabi_static',
         'unwind_static', '-j' + str(args.jobs)], 'build.log')
    wrapper = out / 'native-cxx.py'
    wrapper.write_text('#!/usr/bin/env python3\nimport os,sys\nfrom pathlib import Path\n'
        'r=Path(__file__).resolve().parent\nargs=sys.argv[1:]\n'
        "flags=['-I'+str(r/'sdk-overlay/include'),'-D_GNU_SOURCE',"
        "'-D_LIBCPP_PROVIDES_DEFAULT_RUNE_TABLE','-nostdinc++','-nostdlib++',"
        "'-isystem',str(r/'runtime/include/c++/v1')]\n"
        "if not any(a in args for a in ('-c','-E','-S','-M','-MM','-fsyntax-only',"
        "'--version','-dumpmachine','-dumpversion')):\n"
        " args += ['-L'+str(r/'runtime/lib'),'-lc++','-lc++abi','-lunwind','-lpthread']\n"
        f'compiler={str(tools / "bin/aarch64-aros-clang++")!r}\n'
        'os.execv(compiler,[compiler,*flags,*args])\n')
    wrapper.chmod(0o755)
    probe = out / 'runtime-probe'
    run([str(wrapper), '-std=gnu++17', str(repo / 'tests/native/pi_cxx_runtime_probe.cpp'),
         '-o', str(probe)], 'probe-build.log')
    run(['python3', str(repo / 'tests/native/check_pi_tls.py'), '--readelf',
         str(tools / 'bin/aarch64-aros-readelf'), str(probe)], 'probe-relocations.log')
    header = probe.read_bytes()[:64]
    if header[:6] != b'\x7fELF\x02\x01' or header[7] != 15 or int.from_bytes(header[18:20], 'little') != 183:
        raise RuntimeError('probe is not native ELF64 AROS AArch64')
    outputs = [wrapper, probe, *(build / 'lib' / name for name in ('libc++.a', 'libc++abi.a', 'libunwind.a'))]
    (out / 'build.json').write_text(json.dumps(dict(
        scope='native compile/link only; execution pending', source_manifest_sha256=digest,
        sdk_cdefs_sha256=sha(cdefs), sha256={str(p): sha(p) for p in outputs}), indent=2) + '\n')
    print(wrapper)


if __name__ == '__main__':
    main()
