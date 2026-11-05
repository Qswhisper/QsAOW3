"""Disassemble a bounded RVA range from the hash-pinned original game binary."""
import argparse
from io import BytesIO
from pathlib import Path
import zipfile

from common import base_apk


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--base', type=Path)
    parser.add_argument('--rva', required=True, type=lambda value: int(value, 0))
    parser.add_argument('--bytes', type=int, default=128)
    args = parser.parse_args()
    if not 4 <= args.bytes <= 65536:
        parser.error('--bytes must be 4..65536')
    try:
        from capstone import Cs, CS_ARCH_ARM64, CS_MODE_ARM
        from elftools.elf.elffile import ELFFile
    except ImportError as error:
        raise SystemExit('Install requirements-dev.txt for binary inspection') from error
    with zipfile.ZipFile(base_apk(args.base)) as archive:
        stream = BytesIO(archive.read('lib/arm64-v8a/libil2cpp.so'))
    elf = ELFFile(stream)
    segment = next((item for item in elf.iter_segments() if item['p_type'] == 'PT_LOAD'
                    and item['p_vaddr'] <= args.rva < item['p_vaddr'] + item['p_filesz']), None)
    if segment is None:
        raise ValueError('RVA outside file-backed load segments')
    length = min(args.bytes, segment['p_vaddr'] + segment['p_filesz'] - args.rva)
    stream.seek(args.rva - segment['p_vaddr'] + segment['p_offset'])
    decoder = Cs(CS_ARCH_ARM64, CS_MODE_ARM)
    for instruction in decoder.disasm(stream.read(length), args.rva):
        print(f'{instruction.address:09x}: {instruction.mnemonic:8} {instruction.op_str}')


if __name__ == '__main__':
    main()
