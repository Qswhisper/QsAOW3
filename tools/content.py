"""Stage or rebuild content-addressed campaign catalogues without capture archives."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from common import ROOT, base_apk, extract_assets

MAGIC = 0x43523341
RECORD = struct.Struct('<8i96s')


def fnv(data):
    value = 2166136261
    for byte in data:
        value = ((value ^ byte) * 16777619) & 0xffffffff
    return value


def catalogue(rows, assets, cooperative=False):
    if not 1 <= len(rows) <= 256:
        raise ValueError('Catalogue must contain 1..256 entries')
    index = {item['id']: item for item in json.loads((ROOT / 'data/mission-index.json').read_text())}
    identities = set()
    payload = bytearray()
    for row in rows:
        faction = row['faction']
        chapter = row['mission'] if cooperative else row['chapter']
        recorded = chapter if cooperative else row['recorded']
        previous = -1 if cooperative else row['previous']
        source = row['source']
        relative = Path(source)
        if relative.is_absolute() or '..' in relative.parts or not source.startswith('offline-campaign/') or relative.suffix != '.ggs':
            raise ValueError('Source must be a content-addressed offline-campaign/*.ggs path')
        if faction not in (1, 2) or (faction, chapter) in identities:
            raise ValueError('Duplicate or invalid route identity')
        identities.add((faction, chapter))
        data = (assets / relative).read_bytes()
        digest = hashlib.sha256(data).hexdigest()
        if digest != row['sha256'] or relative.stem != digest or not 32 <= len(data) <= 16 * 1024 * 1024:
            raise ValueError('Route input hash/size/name mismatch')
        if cooperative:
            if (faction, chapter, row['map'], row['own_side'], row['ally_side']) != (2, 973, 558, 1, 3):
                raise ValueError('Unknown cooperative seat contract; implement and verify it first')
            flags = 0
        else:
            a, b = index.get(chapter), index.get(recorded)
            if not a or not b or any(v['fractionId'] != faction or v['type'] not in (1, 2) or v['players'] != 1 for v in (a, b)):
                raise ValueError('Chapter/recorded mission/faction mismatch')
            if b['mapId'] != row['map']:
                raise ValueError('Original map identity mismatch')
            projection = row.get('presentation', 'chapter')
            if projection not in ('chapter', 'recorded') or row.get('growth', 'retained') not in ('retained', 'canonical-own-v1'):
                raise ValueError('Unknown route projection or growth contract')
            if projection == 'chapter' and a['mapId'] != row['map']:
                raise ValueError('Chapter map mismatch')
            if projection == 'recorded':
                ordinal = chapter - (0 if faction == 1 else 200)
                if not 0 <= ordinal < 20 or recorded != (189 if faction == 1 else 389) - ordinal or b['gai'] != f'gai_f{faction}m':
                    raise ValueError('Unknown original variant projection')
            if row['kind'] not in ('embedded', 'filtered', 'legacy-archived'):
                raise ValueError('Unknown route provenance kind')
            if (projection == 'recorded' or row.get('growth') == 'canonical-own-v1') and row['kind'] != 'filtered':
                raise ValueError('Projected/rebased routes require filtered original content')
            flags = int(row['kind'] == 'embedded') | (2 if row.get('growth') == 'canonical-own-v1' else 0) | (4 if projection == 'recorded' else 0)
        if len(source.encode()) >= 96:
            raise ValueError('Route path exceeds native disk ABI')
        checksum = fnv(data)
        signed_checksum = checksum if checksum < 0x80000000 else checksum - 0x100000000
        payload += RECORD.pack(faction, chapter, recorded, previous, row['map'], flags, len(data), signed_checksum, source.encode())
    if not cooperative:
        parents = {(r['faction'], r['chapter']): r['previous'] for r in rows}
        for key in parents:
            visited = set()
            cursor = key
            while parents[cursor] != -1:
                if cursor in visited:
                    raise ValueError('Campaign prerequisite cycle')
                visited.add(cursor)
                cursor = cursor[0], parents[cursor]
                if cursor not in parents:
                    raise ValueError('Missing campaign prerequisite')
    return struct.pack('<4I', MAGIC, 1 if cooperative else 2, len(rows), fnv(payload)) + payload


def validate_catalogues(assets, write=False):
    for configuration, target, coop in [('campaign_routes.json', 'offline-campaign/catalog.bin', False),
                                         ('cooperative_routes.json', 'offline-cooperative/catalog.bin', True)]:
        config = json.loads((ROOT / 'content' / configuration).read_text(encoding='utf8'))
        body = catalogue(config['routes'], assets, coop)
        path = assets / target
        if write:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(body)
        elif path.read_bytes() != body:
            raise ValueError(f'{target} differs from content configuration; run content.py rebuild')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=('stage', 'validate', 'rebuild'))
    parser.add_argument('--base', type=Path)
    parser.add_argument('--assets', type=Path, default=ROOT / '.local/assets')
    args = parser.parse_args()
    assets = args.assets.resolve()
    if args.action == 'stage':
        if assets.exists() and any(assets.iterdir()):
            raise ValueError('Stage into an empty directory; existing content will not be overwritten')
        extract_assets(base_apk(args.base), assets)
    validate_catalogues(assets, write=args.action == 'rebuild')
    print('Content catalogues verified')


if __name__ == '__main__':
    main()
