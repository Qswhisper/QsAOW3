"""Build the offline native adaptation on the immutable bootstrap APK."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import tempfile
import zipfile

from common import ROOT, PROJECT, base_apk, compiler, executable, extract_assets, ndk_path, run, sdk_path, sha256
from content import validate_catalogues

ALLOWED_ROOTS = {'AddressablesCatalogs', 'Resources', 'ResourcesCache', 'offline-bundles', 'offline-campaign',
                 'offline-catalogs', 'offline-cooperative', 'offline-dictionaries', 'offline-replay',
                 'offline-scenarios', 'offline-seeds'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--base', type=Path)
    parser.add_argument('--sdk', type=Path)
    parser.add_argument('--ndk', type=Path)
    parser.add_argument('--assets', type=Path, help='Optional complete staged offline inputs')
    parser.add_argument('--output', type=Path, default=ROOT / 'dist/QsAOW3.apk')
    parser.add_argument('--keystore', type=Path, help='Omit to use a newly generated local development key')
    parser.add_argument('--key-alias', default=os.environ.get('QSAOW3_KEY_ALIAS', 'art3offline'))
    args = parser.parse_args()
    base = base_apk(args.base)
    sdk = sdk_path(args.sdk)
    ndk = ndk_path(sdk, args.ndk)
    build_tools = sdk / 'build-tools' / PROJECT['build_tools_version']
    aligner = build_tools / ('zipalign.exe' if os.name == 'nt' else 'zipalign')
    signer = build_tools / 'lib/apksigner.jar'
    if not aligner.is_file() or not signer.is_file():
        raise ValueError(f'Install build-tools;{PROJECT["build_tools_version"]}')
    output = args.output.resolve()
    if output == base:
        raise ValueError('The immutable bootstrap cannot be the output path')
    output.parent.mkdir(parents=True, exist_ok=True)
    local = ROOT / '.local'
    local.mkdir(exist_ok=True)
    key = args.keystore.resolve() if args.keystore else local / 'development.keystore'
    env = dict(os.environ)
    if args.keystore and 'QSAOW3_STORE_PASSWORD' not in env:
        raise ValueError('Set QSAOW3_STORE_PASSWORD for the supplied keystore')
    env.setdefault('QSAOW3_STORE_PASSWORD', 'android')
    env.setdefault('QSAOW3_KEY_PASSWORD', env['QSAOW3_STORE_PASSWORD'])
    if not key.exists():
        if args.keystore:
            raise ValueError('Supplied signing key does not exist; refusing to replace it')
        run([executable('keytool'), '-genkeypair', '-keystore', key,
             '-storepass:env', 'QSAOW3_STORE_PASSWORD', '-keypass:env', 'QSAOW3_KEY_PASSWORD',
             '-alias', args.key_alias, '-keyalg', 'RSA', '-keysize', '2048', '-validity', '3650',
             '-dname', 'CN=QsAOW3 Development'], env=env)
    with tempfile.TemporaryDirectory(prefix='build-', dir=local) as temporary:
        work = Path(temporary)
        assets = args.assets.resolve() if args.assets else work / 'assets'
        if not args.assets:
            extract_assets(base, assets)
        validate_catalogues(assets)
        with zipfile.ZipFile(base) as archive:
            elf = archive.read('lib/arm64-v8a/libil2cpp.so')
            if hashlib.sha256(elf).hexdigest() != PROJECT['base']['il2cpp_sha256']:
                raise ValueError('IL2CPP binary ABI mismatch')
            if hashlib.sha256(archive.read('lib/arm64-v8a/libart3originalmain.so')).hexdigest() != PROJECT['base']['loader_sha256']:
                raise ValueError('Original Unity loader mismatch')
        library = work / 'libmain.so'
        print('Compiling the ARM64 offline adaptation', flush=True)
        run([compiler(ndk), '--target=aarch64-linux-android24', '-std=c++17', '-shared', '-fPIC', '-O2',
             '-static-libstdc++', '-DART3_MAIN=1', '-fvisibility=hidden', '-Wl,-z,max-page-size=16384',
             f'-ffile-prefix-map={ROOT}=.', ROOT / 'native/offline_bootstrap.cpp',
             '-llog', '-ldl', '-landroid', '-o', library])
        records = []
        for path in sorted(assets.rglob('*')):
            if not path.is_file():
                continue
            relative = path.relative_to(assets).as_posix()
            if relative in ('install.list', 'manifest.json'):
                continue
            if relative.split('/')[0] not in ALLOWED_ROOTS or '..' in Path(relative).parts:
                raise ValueError(f'Unexpected offline input: {relative}')
            records.append(dict(path=relative, size=path.stat().st_size, sha256=sha256(path)))
        if len(records) < 381:
            raise ValueError('Incomplete offline input set')
        records.sort(key=lambda row: row['path'])
        install_list = ''.join(f'{r["size"]}\t{r["path"]}\n' for r in records).encode()
        unsigned, aligned = work / 'unsigned.apk', work / 'aligned.apk'
        with zipfile.ZipFile(base) as source, zipfile.ZipFile(unsigned, 'w') as target:
            for entry in source.infolist():
                if entry.filename.startswith(('META-INF/', 'assets/offline/')):
                    continue
                data = library.read_bytes() if entry.filename == 'lib/arm64-v8a/libmain.so' else source.read(entry)
                target.writestr(entry, data)
            for row in records:
                target.write(assets / row['path'], 'assets/offline/' + row['path'], compress_type=zipfile.ZIP_STORED)
            target.writestr('assets/offline/install.list', install_list, compress_type=zipfile.ZIP_DEFLATED)
        run([aligner, '-f', '-P', '16', '4', unsigned, aligned])
        print('Signing and verifying the APK', flush=True)
        java = executable('java')
        run([java, '-jar', signer, 'sign', '--ks', key, '--ks-key-alias', args.key_alias,
             '--ks-pass', 'env:QSAOW3_STORE_PASSWORD', '--key-pass', 'env:QSAOW3_KEY_PASSWORD',
             '--out', output, aligned], env=env)
        run([java, '-jar', signer, 'verify', '--verbose', output])
        run([aligner, '-c', '-P', '16', '4', output])
        summary = dict(project=PROJECT['name'], version=PROJECT['version'], package=PROJECT['package'],
                       base_sha256=PROJECT['base']['sha256'], apk_sha256=sha256(output),
                       native_sha256=sha256(library), offline_inputs=len(records),
                       signing='supplied key' if args.keystore else 'local development key')
        output.with_suffix('.build.json').write_text(json.dumps(summary, indent=2) + '\n', encoding='utf8')
        print(json.dumps(summary, indent=2))


if __name__ == '__main__':
    main()
