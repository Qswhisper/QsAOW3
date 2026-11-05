"""Run standalone C++ policy/store tests on Linux/macOS or an explicit Android device."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import secrets
import shutil
import struct
import subprocess
import tempfile
import time

from common import ROOT, compiler, executable, ndk_path, run, sdk_path
from content import MAGIC, RECORD, fnv


def catalogue_fixture(root):
    """Synthetic wire-format files for parser/corruption tests, never game content."""
    destination = root / 'offline-campaign'
    destination.mkdir(parents=True)
    payload = bytearray()
    for chapter in range(5):
        data = bytes([chapter + 1]) * 64
        digest = hashlib.sha256(data).hexdigest()
        name = f'offline-campaign/{digest}.ggs'
        (root / name).write_bytes(data)
        checksum = fnv(data)
        checksum = checksum if checksum < 0x80000000 else checksum - 0x100000000
        payload += RECORD.pack(1, chapter, chapter, chapter - 1, 100 + chapter, 1, len(data), checksum, name.encode())
    (destination / 'catalog.bin').write_bytes(struct.pack('<4I', MAGIC, 2, 5, fnv(payload)) + payload)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--android', action='store_true')
    parser.add_argument('--serial', help='Required with --android; never select the first connected device')
    parser.add_argument('--adb-port', type=int, default=5037)
    parser.add_argument('--sdk', type=Path)
    parser.add_argument('--ndk', type=Path)
    parser.add_argument('--filter', help='Run test names containing this text')
    args = parser.parse_args()
    if args.android and not args.serial:
        parser.error('--android requires --serial')
    if os.name == 'nt' and not args.android:
        parser.error('The native tests require POSIX; use Linux/WSL or --android on Windows')
    tests = sorted((ROOT / 'native').glob('*_test.cpp'))
    if args.filter:
        tests = [p for p in tests if args.filter in p.stem]
    if not tests:
        raise ValueError('No matching tests')
    required = {p.stem for p in tests}
    if required & {'item_inventory_test', 'modification_store_test', 'local_progress_stack_test'}:
        required.add('supply_policy_test')
    if 'modification_store_test' in required:
        required.add('item_inventory_test')
    tests = sorted((ROOT / 'native' / (name + '.cpp') for name in required),
                   key=lambda path: (path.stem != 'supply_policy_test', path.stem))
    local = ROOT / '.local'
    local.mkdir(exist_ok=True)
    adb = None
    if args.android:
        sdk = sdk_path(args.sdk)
        cc = compiler(ndk_path(sdk, args.ndk))
        adb = [str(sdk / 'platform-tools' / ('adb.exe' if os.name == 'nt' else 'adb')),
               '-P', str(args.adb_port), '-s', args.serial]
        response = subprocess.run(adb + ['shell', 'getprop', 'ro.product.cpu.abi'], capture_output=True, check=True, timeout=20)
        abi = response.stdout.decode().strip()
        target = {'x86_64': 'x86_64-linux-android24', 'arm64-v8a': 'aarch64-linux-android24'}.get(abi)
        if not target:
            raise ValueError(f'Unsupported test ABI: {abi}')
    else:
        cc = os.environ.get('CXX') or shutil.which('clang++') or shutil.which('g++')
        if not cc:
            raise ValueError('Install a C++17 compiler or set CXX')
    results = []
    # Keep host fixtures short as well: CI checkout paths and macOS TMPDIR
    # can exceed the retained 64-byte disk-ABI fixture path buffers.
    with tempfile.TemporaryDirectory(prefix='q-', dir=local if adb else '/tmp') as directory:
        work = Path(directory)
        # Several retained disk-ABI fixtures have 64-byte path buffers. Keep
        # the temporary prefix short enough to exercise their real assertions.
        remote = '/data/local/tmp/q-' + secrets.token_hex(2) if adb else None
        if adb:
            run(adb + ['shell', 'mkdir', remote], capture_output=True, timeout=20)
        try:
            for source in tests:
                name = source.stem
                case = work / name
                case.mkdir()
                binary = case / 'test'
                root = (remote + '/' + name + '/') if adb else case.as_posix() + '/'
                prefix = (remote + '/') if adb else work.as_posix() + '/'
                flags = [f'--target={target}', '-static-libstdc++'] if adb else []
                command = [cc, *flags, '-std=c++17', '-O2', '-UNDEBUG', '-pthread', f'-DQSAOW3_TEST_TMP="{prefix}"', str(source), '-o', str(binary)]
                run(command, capture_output=True, timeout=120)
                arguments = [root]
                if name == 'campaign_catalog_test':
                    catalogue_fixture(case)
                elif name == 'ai_tech_dependencies_test':
                    shutil.copyfile(ROOT / 'tests/fixtures/ai-tech-dependencies.txt', case / 'graph.txt')
                    arguments = [root + 'graph.txt']
                elif name == 'rifle_research_effects_test':
                    shutil.copyfile(ROOT / 'tests/fixtures/rifle-research.txt', case / 'levels.txt')
                    arguments = [root + 'levels.txt']
                elif name in ('contract_store_test', 'quest_store_test', 'skill_store_test'):
                    arguments = []  # Optional external legacy-file arguments are not needed.
                elif name in ('career_star_store_test', 'formal_credit_store_test', 'supply_policy_test'):
                    arguments = [root + 'output.bin']
                elif name in ('item_inventory_test', 'modification_store_test', 'local_progress_stack_test'):
                    dependency = 'item_inventory_test' if name == 'modification_store_test' else 'supply_policy_test'
                    dependency_root = (remote + '/' + dependency + '/') if adb else (work / dependency).as_posix() + '/'
                    arguments = [dependency_root + 'output.bin']
                    if name != 'local_progress_stack_test':
                        arguments.append(root + 'output.bin')
                if adb:
                    # Only a freshly created /data/local/tmp directory is used.
                    run(adb + ['push', str(case), remote + '/'], capture_output=True, timeout=40)
                    run(adb + ['shell', 'chmod', '755', root + 'test'], capture_output=True, timeout=20)
                    execute = adb + ['shell', root + 'test', *arguments]
                else:
                    execute = [str(binary), *arguments]
                start = time.monotonic()
                result = subprocess.run(execute, capture_output=True, timeout=60)
                if result.returncode:
                    print((result.stdout + result.stderr).decode('utf8', 'replace'))
                    results.append(dict(test=name, passed=False, exit_code=result.returncode))
                    print(f'FAIL: {name} exited {result.returncode}', flush=True)
                    continue
                text = result.stdout.decode('utf8', 'replace').strip()
                results.append(dict(test=name, passed=True, seconds=round(time.monotonic() - start, 3)))
                print(name + ': ' + (text.splitlines()[-1] if text else 'passed'), flush=True)
        finally:
            if adb:
                # remote is generated above, never supplied by a user or derived from saves.
                assert remote.startswith('/data/local/tmp/q-') and len(remote) == 22 and '..' not in remote
                run(adb + ['shell', 'rm', '-rf', remote], capture_output=True, timeout=30)
    report = ROOT / '.local/test-results.json'
    passed = all(row['passed'] for row in results)
    report.write_text(json.dumps(dict(passed=passed, tests=results), indent=2) + '\n', encoding='utf8')
    print(f'{"PASS" if passed else "FAIL"}: {len(results)} standalone tests; report {report}')
    if not passed:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
