#!/usr/bin/env python3
"""Check download checksums, embedded manifests and required OS/source contents."""
from pathlib import Path, PurePosixPath
import argparse
import hashlib
import json
import stat
import zipfile

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('directory',type=Path)
parser.add_argument('--all-platforms',action='store_true')
args=parser.parse_args()
archives=sorted(args.directory.glob('*.zip'))
if not archives:
    raise SystemExit('No download archives found')
found=set()
revisions=set()
versions=set()
for archive in archives:
    expected=archive.with_suffix('.zip.sha256').read_text().split()[0]
    if hashlib.sha256(archive.read_bytes()).hexdigest()!=expected:
        raise SystemExit(f'Invalid download checksum: {archive.name}')
    with zipfile.ZipFile(archive) as bundle:
        names=set(bundle.namelist())
        for name in names:
            path=PurePosixPath(name)
            if path.is_absolute() or '..' in path.parts or '\\' in name:
                raise SystemExit(f'Unsafe archive path: {name}')
        roots={PurePosixPath(name).parts[0] for name in names}
        if len(roots)!=1:
            raise SystemExit(f'Archive must contain one root folder: {archive.name}')
        root=next(iter(roots))+'/'
        metadata=json.loads(bundle.read(root+'BUILD_INFO.json'))
        revisions.add(metadata['revision']);versions.add(metadata['version'])
        if args.all_platforms and (metadata['source_dirty'] or root+'LICENSE' not in names):
            raise SystemExit(f'Release source is dirty or project licence missing: {archive.name}')
        if archive.name.endswith('-source.zip'):
            required=['CMakeLists.txt','Source/PluginProcessor.cpp','vendor/JUCE/CMakeLists.txt','vendor/JUCE/LICENSE.md','scripts/build.sh']
            found.add('source')
        else:
            manifest=json.loads(bundle.read(root+'SHA256.json'))
            for name,digest in manifest.items():
                if hashlib.sha256(bundle.read(root+name)).hexdigest()!=digest:
                    raise SystemExit(f'Invalid bundled file checksum: {name}')
            required=['README.md','INSTALL.md','docs/CONTROLLERS.md','THIRD-PARTY-NOTICES/JUCE/LICENSE.md']
            if '-windows-' in archive.name:
                found.add('windows');required+=['Standalone/Field Studio.exe','VST3/Field Studio.vst3/Contents/x86_64-win/Field Studio.vst3']
            elif '-macos-universal' in archive.name:
                found.add('macos');required+=['Standalone/Field Studio.app/Contents/MacOS/Field Studio','VST3/Field Studio.vst3/Contents/MacOS/Field Studio','AU/Field Studio.component/Contents/MacOS/Field Studio']
            elif '-linux-' in archive.name:
                found.add('linux');required+=['run.sh','Standalone/Field Studio','VST3/Field Studio.vst3/Contents/x86_64-linux/Field Studio.so']
                for name in ('run.sh','Standalone/Field Studio'):
                    mode=bundle.getinfo(root+name).external_attr>>16
                    if not mode&stat.S_IXUSR:
                        raise SystemExit(f'Executable permission missing: {name}')
            else:
                raise SystemExit(f'Unknown OS package: {archive.name}')
        for name in required:
            if root+name not in names:
                raise SystemExit(f'Missing {name} in {archive.name}')
    print('Verified '+archive.name)
if args.all_platforms and found!={'windows','macos','linux','source'}:
    raise SystemExit(f'Incomplete release targets: {found}')
if args.all_platforms and (len(revisions)!=1 or len(versions)!=1):
    raise SystemExit('Release downloads do not share one source revision and version')
