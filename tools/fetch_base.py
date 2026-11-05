"""Download the immutable bootstrap APK, with streaming SHA256 verification."""
import argparse
from pathlib import Path
import urllib.request

from common import PROJECT, ROOT, sha256


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / '.local/base.apk')
    args = parser.parse_args()
    destination = args.output.resolve()
    if destination.exists():
        if sha256(destination) != PROJECT['base']['sha256']:
            raise ValueError('Existing file differs from the bootstrap; refusing to overwrite it')
        print('Bootstrap already verified')
        return
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_name(destination.name + '.part')
    request = urllib.request.Request(PROJECT['base']['url'], headers={'User-Agent': 'QsAOW3-build'})
    with urllib.request.urlopen(request, timeout=60) as response, temporary.open('wb') as out:
        while block := response.read(1024 * 1024):
            out.write(block)
    if sha256(temporary) != PROJECT['base']['sha256']:
        temporary.unlink()
        raise ValueError('Downloaded bootstrap SHA256 mismatch')
    temporary.replace(destination)
    print(f'Bootstrap verified: {destination}')


if __name__ == '__main__':
    main()
