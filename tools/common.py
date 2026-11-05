"""Portable paths and hash-checked inputs. No original workstation dependencies."""
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
PROJECT = json.loads((ROOT / 'project.json').read_text(encoding='utf8'))


def sha256(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def base_apk(value=None):
    path = Path(value).expanduser().resolve() if value else ROOT / '.local/base.apk'
    if not path.is_file():
        raise ValueError('Base APK missing. Run python tools/fetch_base.py, or pass --base PATH.')
    if sha256(path) != PROJECT['base']['sha256']:
        raise ValueError('Base APK SHA256 mismatch. A different game build is not ABI-compatible.')
    return path


def sdk_path(value=None):
    candidates = [value, os.environ.get('ANDROID_SDK_ROOT'), os.environ.get('ANDROID_HOME')]
    if os.environ.get('LOCALAPPDATA'):
        candidates.append(Path(os.environ['LOCALAPPDATA']) / 'Android/Sdk')
    candidates += [Path.home() / 'Android/Sdk', Path.home() / 'Library/Android/sdk']
    for candidate in candidates:
        if candidate and Path(candidate).expanduser().is_dir():
            return Path(candidate).expanduser().resolve()
    raise ValueError('Android SDK missing. Pass --sdk PATH or set ANDROID_HOME.')


def ndk_path(sdk=None, value=None):
    candidate = value or os.environ.get('ANDROID_NDK_HOME')
    path = Path(candidate).expanduser().resolve() if candidate else Path(sdk) / 'ndk' / PROJECT['ndk_version']
    if not (path / 'source.properties').is_file():
        raise ValueError(f'NDK missing: install ndk;{PROJECT["ndk_version"]}, or pass --ndk PATH.')
    return path


def compiler(ndk):
    host = {'Windows': 'windows-x86_64', 'Linux': 'linux-x86_64', 'Darwin': 'darwin-x86_64'}.get(platform.system())
    if not host:
        raise ValueError('Unsupported build host')
    return Path(ndk) / 'toolchains/llvm/prebuilt' / host / 'bin' / ('clang++.exe' if os.name == 'nt' else 'clang++')


def executable(name):
    result = shutil.which(name)
    if not result:
        raise ValueError(f'{name} missing from PATH')
    return result


def run(arguments, **kwargs):
    return subprocess.run([str(item) for item in arguments], check=True, **kwargs)


def extract_assets(apk, destination):
    """Extract only the checked offline allowlist; never import profiles or preferences."""
    expected = json.loads((ROOT / 'data/assets-manifest.json').read_text(encoding='utf8'))
    destination = Path(destination)
    destination.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(apk) as archive:
        for row in expected:
            relative = Path(row['path'])
            if relative.is_absolute() or '..' in relative.parts:
                raise ValueError('Unsafe asset path')
            data = archive.read('assets/offline/' + relative.as_posix())
            if len(data) != row['size'] or hashlib.sha256(data).hexdigest() != row['sha256']:
                raise ValueError(f'Offline input hash mismatch: {relative}')
            target = destination / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
    return expected
