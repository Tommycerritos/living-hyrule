"""Reject private assets/build outputs in staged changes and outgoing history.

No third-party dependencies. Existing official upstream history is excluded from
push checks. Hooks are guardrails, not DRM: never bypass them to add game assets.
"""
import argparse
import pathlib
import subprocess
import sys

FORBIDDEN_EXTENSIONS = set('z64 n64 v64 otr o2r sav sra state iso wad exe dll lib pdb obj bin zip 7z rar mpq tar gz bz2 png jpg jpeg bmp tga dds wav mp3 ogg flac fbx blend'.split())
FORBIDDEN_DIRS = {'roms', 'baserom', 'runtime', 'private-assets', 'extracted-assets', 'backups', 'save', 'mods', 'build', 'dist', '.venv'}
FORBIDDEN_NAMES = {'shipofharkinian.json', 'shipofharkinian.ini', 'imgui.ini', 'timesplitdata.json', '.env'}
ROM_MAGICS = {bytes.fromhex(x) for x in ('80371240', '37804012', '40123780')}

def git(*args):
    return subprocess.check_output(['git', *args])

def check_blob(oid, name):
    path = pathlib.PurePosixPath(name.lower())
    if (path.suffix.lstrip('.') in FORBIDDEN_EXTENSIONS or
            FORBIDDEN_DIRS.intersection(path.parts[:-1]) or
            path.name in FORBIDDEN_NAMES or path.name.startswith('.env.')):
        return f'{name}: private asset, binary, runtime, or archive path'
    size = int(git('cat-file', '-s', oid))
    if size > 10 * 1024 * 1024:
        return f'{name}: exceeds 10 MiB; review provenance before changing policy'
    data = git('cat-file', 'blob', oid)
    if data[:4] in ROM_MAGICS:
        return f'{name}: Nintendo 64 ROM header detected'
    if data.startswith((b'PK\x03\x04', b'7z\xbc\xaf\x27\x1c', b'Rar!', b'MPQ\x1a', b'MZ')):
        return f'{name}: renamed archive or executable detected'
    return None

def staged_blobs():
    names = set(git('diff', '--cached', '--name-only', '--diff-filter=ACMRT', '-z').split(b'\0'))
    for entry in git('ls-files', '--stage', '-z').split(b'\0'):
        if not entry:
            continue
        meta, name = entry.split(b'\t', 1)
        mode, oid, stage = meta.split()
        if name in names and mode != b'160000':
            yield oid.decode(), name.decode('utf-8', 'surrogateescape')

def outgoing_blobs(refs):
    # A blob added and then deleted is still inspected if it is in outgoing history.
    rows = git('rev-list', '--objects', *refs, '--not', '--remotes=upstream').splitlines()
    for row in rows:
        oid, _, name = row.partition(b' ')
        if name and git('cat-file', '-t', oid.decode()).strip() == b'blob':
            yield oid.decode(), name.decode('utf-8', 'surrogateescape')

def main():
    parser = argparse.ArgumentParser()
    choice = parser.add_mutually_exclusive_group(required=True)
    choice.add_argument('--staged', action='store_true')
    choice.add_argument('--push', action='store_true')
    choice.add_argument('--history', nargs='+')
    args = parser.parse_args()
    if args.staged:
        blobs = staged_blobs()
    else:
        refs = args.history or [row.split()[1] for row in sys.stdin if row.strip() and set(row.split()[1]) != {'0'}]
        blobs = outgoing_blobs(refs) if refs else []
    errors = [error for oid, name in blobs if (error := check_blob(oid, name))]
    if errors:
        print('Living Hyrule repository guard BLOCKED this operation:', file=sys.stderr)
        print('\n'.join(errors), file=sys.stderr)
        print('Keep ROMs and extracted assets under C:\\ZeldaDev\\roms or assets, outside Git.', file=sys.stderr)
        return 1
    print('Living Hyrule repository guard passed.')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
