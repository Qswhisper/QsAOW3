"""Check public source structure, local links, C++ includes and accidental private files."""
from pathlib import Path
import re

from common import ROOT

SKIP = {'.git', '.local', '.venv', '__pycache__', 'build', 'dist'}
FORBIDDEN_EXTENSIONS = {'.apk', '.aab', '.keystore', '.jks', '.p12', '.pem', '.key', '.ggs', '.bin', '.so', '.o', '.exe'}
FORBIDDEN_TEXT = [r'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----',
                  r'\bgh[pousr]_[A-Za-z0-9]{30,}', r'\bgithub_pat_[A-Za-z0-9_]{30,}',
                  r'[A-Z]:[/\\]Users[/\\]', r'/home/[^/\s]+/', r'/Users/[^/\s]+/']


def main():
    files = [p for p in ROOT.rglob('*') if p.is_file() and not any(part in SKIP for part in p.relative_to(ROOT).parts)]
    for path in files:
        relative = path.relative_to(ROOT).as_posix()
        if path.suffix.lower() in FORBIDDEN_EXTENSIONS or path.stat().st_size > 5 * 1024 * 1024:
            raise ValueError(f'Unexpected public file: {relative}')
        text = path.read_text(encoding='utf8')
        if path.name != 'check_source.py':
            for pattern in FORBIDDEN_TEXT:
                if re.search(pattern, text):
                    raise ValueError(f'Private/history marker in {relative}')
        if path.suffix in ('.h', '.cpp', '.inc'):
            for included in re.findall(r'#\s*include\s*"([^"]+)"', text):
                target = (path.parent / included).resolve()
                if not target.is_relative_to(ROOT) or not target.is_file():
                    raise ValueError(f'Missing or external include in {relative}: {included}')
        if path.suffix == '.md':
            for link in re.findall(r'\[[^\]]*\]\(([^)]+)\)', text):
                if '://' in link or link.startswith('#'):
                    continue
                target = (path.parent / link.split('#')[0]).resolve()
                if not target.is_relative_to(ROOT) or not target.is_file():
                    raise ValueError(f'Broken local link in {relative}: {link}')
    print(f'PASS: {len(files)} public source files; links/includes and private-file checks')


if __name__ == '__main__':
    main()
