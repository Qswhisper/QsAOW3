"""Verify an APK's identity, local inputs, ABI and actual manifest permissions."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

from common import PROJECT, ROOT, base_apk


def xml_strings(blob):
    position = 8
    while position < len(blob):
        kind, header, size = struct.unpack_from('<HHI', blob, position)
        if kind == 1:
            count, _, flags, start, _ = struct.unpack_from('<5I', blob, position + 8)
            strings = []
            for index in range(count):
                cursor = position + start + struct.unpack_from('<I', blob, position + header + index * 4)[0]
                if flags & 0x100:
                    def length(offset):
                        value = blob[offset]
                        return (((value & 127) << 8) | blob[offset + 1], offset + 2) if value & 128 else (value, offset + 1)
                    _, cursor = length(cursor)
                    size_in_bytes, cursor = length(cursor)
                    strings.append(blob[cursor:cursor + size_in_bytes].decode('utf8'))
                else:
                    units = struct.unpack_from('<H', blob, cursor)[0]
                    cursor += 2
                    if units & 0x8000:
                        units = ((units & 0x7fff) << 16) | struct.unpack_from('<H', blob, cursor)[0]
                        cursor += 2
                    strings.append(blob[cursor:cursor + units * 2].decode('utf-16-le'))
            return strings
        position += size
    raise ValueError('Missing binary XML string pool')


def manifest(blob):
    strings = xml_strings(blob)
    position = 8
    elements = []
    while position < len(blob):
        kind, header, size = struct.unpack_from('<HHI', blob, position)
        if kind == 0x102:
            name = strings[struct.unpack_from('<I', blob, position + 20)[0]]
            start, stride, count = struct.unpack_from('<HHH', blob, position + 24)
            attributes = {}
            for index in range(count):
                attribute = position + 16 + start + stride * index
                key = strings[struct.unpack_from('<I', blob, attribute + 4)[0]]
                value = struct.unpack_from('<I', blob, attribute + 16)[0]
                attributes[key] = strings[value] if blob[attribute + 15] == 3 else value
            elements.append((name, attributes))
        position += size
    return elements


def verify(apk):
    expected = json.loads((ROOT / 'data/assets-manifest.json').read_text(encoding='utf8'))
    with zipfile.ZipFile(apk) as archive:
        if len(archive.namelist()) != len(set(archive.namelist())):
            raise ValueError('Duplicate ZIP entries')
        elements = manifest(archive.read('AndroidManifest.xml'))
        root = next(attrs for tag, attrs in elements if tag == 'manifest')
        assert root['package'] == PROJECT['package']
        application = next(attrs for tag, attrs in elements if tag == 'application')
        launcher = next(attrs for tag, attrs in elements if tag == 'activity' and attrs.get('name') == 'com.trophit.MyUnityPlayerActivity')
        assert application['label'] == launcher['label'] == PROJECT['name']
        permissions = [attrs['name'] for tag, attrs in elements if tag == 'uses-permission']
        assert 'android.permission.INTERNET' not in permissions
        assert not any('BILLING' in value for value in permissions)
        assert hashlib.sha256(archive.read('lib/arm64-v8a/libil2cpp.so')).hexdigest() == PROJECT['base']['il2cpp_sha256']
        for row in expected:
            body = archive.read('assets/offline/' + row['path'])
            assert len(body) == row['size'] and hashlib.sha256(body).hexdigest() == row['sha256'], row['path']
        records = [f'{row["size"]}\t{row["path"]}' for row in expected]
        assert archive.read('assets/offline/install.list').decode().splitlines() == records
    return dict(passed=True, package=PROJECT['package'], application=PROJECT['name'],
                offline_inputs=len(expected), internet_permission=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apk', type=Path)
    args = parser.parse_args()
    print(json.dumps(verify(args.apk or base_apk()), indent=2))
