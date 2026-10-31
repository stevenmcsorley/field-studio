#!/usr/bin/env python3
"""Create portable OS bundles and optional complete corresponding source."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import platform
import re
import shutil
import stat
import subprocess
import zipfile

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build-dir', default='build')
parser.add_argument('--output-dir', default='dist')
parser.add_argument('--architecture', default=platform.machine())
parser.add_argument('--source', action='store_true')
parser.add_argument('--release', action='store_true', help='Require a chosen project licence')
args = parser.parse_args()
cmake_text=(root/'CMakeLists.txt').read_text()
base_version = re.search(r'project\(FieldStudio VERSION ([\d.]+)', cmake_text).group(1)
version=base_version
version_file = root/'RELEASE_VERSION'
if version_file.exists():
    version = version_file.read_text().strip()
    if not re.fullmatch(r'\d+\.\d+\.\d+(?:-[a-zA-Z0-9.-]+)?', version):
        raise SystemExit('Invalid RELEASE_VERSION')
if args.release and not all((root/name).is_file() for name in ('LICENSE','LICENSING.md','LICENSES/AGPL-3.0.txt')):
    raise SystemExit('Choose a project licence and add LICENSE before distributing a release.')
release = root / args.build_dir / 'FieldStudio_artefacts' / 'Release'
system = platform.system()
arch = {'AMD64':'x86_64','aarch64':'arm64'}.get(args.architecture,args.architecture)
os_label = {'Darwin':'macos','Windows':'windows','Linux':'linux'}.get(system)
if os_label is None:
    raise SystemExit(f'Unsupported packaging platform: {system}')
label = f'Field-Studio-{version}-{os_label}-{arch}'
output = root/args.output_dir
output.mkdir(parents=True,exist_ok=True)
destination = output/label
standalone = release/'Standalone'
vst = release/'VST3'/'Field Studio.vst3'
if not standalone.is_dir() or not vst.is_dir():
    raise SystemExit('Build the standalone and VST3 targets first.')
module_info=vst/'Contents/Resources/moduleinfo.json'
module_version=re.search(r'"Version"\s*:\s*"([^"]+)"',module_info.read_text())
if not module_version or module_version.group(1)!=base_version:
    raise SystemExit('VST3 binary version does not match CMake project; finish rebuilding before packaging.')
revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
dirty=bool(subprocess.check_output(['git','status','--porcelain'],cwd=root,text=True).strip())
juce_revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root/'vendor/JUCE',text=True).strip()
juce_dirty=bool(subprocess.check_output(['git','status','--porcelain'],cwd=root/'vendor/JUCE',text=True).strip())
expected_juce=re.search(r'GIT_TAG ([a-f0-9]{40})',cmake_text).group(1)
if juce_revision!=expected_juce:
    raise SystemExit('vendor/JUCE is not the pinned dependency revision.')
if args.release and (dirty or juce_dirty):
    raise SystemExit('Release packages require a clean, committed source tree.')
build_info={'version':version,'binary_version':base_version,'revision':revision,'juce_revision':juce_revision,'juce_source_dirty':juce_dirty,'platform':os_label,'architecture':arch,'source_dirty':dirty,'project_license_included':(root/'LICENSE').is_file(),'original_code_license':'MIT','combined_binary_license':'AGPL-3.0-only'}
if system == 'Darwin' and arch == 'universal':
    for path in (standalone/'Field Studio.app/Contents/MacOS/Field Studio',vst/'Contents/MacOS/Field Studio',release/'AU/Field Studio.component/Contents/MacOS/Field Studio'):
        subprocess.run(['lipo',str(path),'-verify_arch','arm64','x86_64'],check=True)
if destination.exists():
    shutil.rmtree(destination)
destination.mkdir()
(destination/'BUILD_INFO.json').write_text(json.dumps(build_info,indent=2)+'\n')
for folder in ('Standalone','VST3'):
    shutil.copytree(release/folder,destination/folder,symlinks=True)
if system == 'Darwin':
    if not (release/'AU').is_dir():
        raise SystemExit('macOS release must include AU as well as VST3.')
    shutil.copytree(release/'AU',destination/'AU',symlinks=True)
for name in ('README.md','LICENSE','LICENSING.md','RELEASE_VERSION'):
    if (root/name).exists():
        shutil.copy2(root/name,destination/name)
if (root/'LICENSES').is_dir():
    shutil.copytree(root/'LICENSES',destination/'LICENSES')
shutil.copytree(root/'docs',destination/'docs',symlinks=True)
shutil.copytree(root/'presets',destination/'Controller profiles') if (root/'presets').exists() else None
instructions = {
    'Linux': '''Run ./run.sh on a graphical desktop. Copy the entire VST3/Field Studio.vst3 bundle to ~/.vst3/ and rescan plugins in your DAW.
Built on Ubuntu 22.04 x86_64; requires glibc 2.35 or newer and standard ALSA, X11, FreeType/fontconfig and OpenGL runtime libraries. Other distributions have not been device-tested.
''',
    'Windows': '''Run Standalone/Field Studio.exe. Copy the entire VST3/Field Studio.vst3 bundle to C:\\Program Files\\Common Files\\VST3\\ (administrator permission) or a VST3 directory supported by your DAW, then rescan plugins.
Windows x64 build. If Windows reports missing Microsoft runtime DLLs, install Microsoft's Visual C++ 2015-2022 x64 Redistributable from https://learn.microsoft.com/cpp/windows/latest-supported-vc-redist . This portable beta has no installer or code signature. Check the publisher/source before opening any security prompt; do not disable Windows security globally.
''',
    'Darwin': '''Drag Standalone/Field Studio.app to Applications. Copy VST3/Field Studio.vst3 to ~/Library/Audio/Plug-Ins/VST3/ and AU/Field Studio.component to ~/Library/Audio/Plug-Ins/Components/, then rescan plugins/restart the DAW.
Universal Apple Silicon + Intel build; macOS 11 or newer. This beta is not Developer ID signed or notarized. macOS or a DAW may refuse it. If macOS offers an exception after verifying the download, use System Settings > Privacy & Security > Open Anyway for this app; do not disable Gatekeeper globally. We have not validated loading in Logic or other commercial DAWs.
'''
}
(destination/'INSTALL.md').write_text('# Field Studio '+version+'\n\nOriginal Field Studio code: MIT. Combined app/plugin binaries with JUCE: AGPLv3; matching complete source is provided alongside the binaries. Read LICENSING.md and LICENSES/AGPL-3.0.txt before redistributing.\n\n'+instructions[system]+'''
Connect a MIDI controller by USB (or a MIDI interface). Select its input under Options > Audio/MIDI Settings in the standalone, or route MIDI to the plugin in the DAW. Right-click a knob and choose MIDI learn; see docs/CONTROLLERS.md. Controller profiles and learned assignments can be saved.

This is a development beta. Heavy edits, record preparation and project loading can interrupt audio; it is not yet certified for seamless live performance. Back up projects. See docs/AUDIO_REWRITE_PROGRESS.md and docs/BUILD_STATUS.md for tested scope and limits.

The accompanying source archive includes build scripts and the pinned JUCE source. Binary downloads retain dependency notices in THIRD-PARTY-NOTICES/. SHA256.json hashes each regular file in this bundle; the adjacent .sha256 hashes the ZIP download.
''',encoding='utf-8')
notices=destination/'THIRD-PARTY-NOTICES'
juce=root/'vendor/JUCE'
if not (juce/'LICENSE.md').exists():
    raise SystemExit('A pinned vendor/JUCE checkout is required to bundle dependency notices.')
for path in juce.rglob('*'):
    if not path.is_file() or '.git' in path.relative_to(juce).parts:
        continue
    name=path.name.lower()
    explicit = any(part in str(path.relative_to(juce)) for part in ('jpglib/README','zlib/README','oggvorbis/Ogg Vorbis Licence.txt'))
    if 'licen' in name or name in ('copying','notice','copyright') or explicit:
        target=notices/'JUCE'/path.relative_to(juce)
        target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(path,target)
if system == 'Linux':
    launcher=destination/'run.sh'
    launcher.write_text('#!/usr/bin/env bash\nset -euo pipefail\ncd "$(dirname "$0")"\nexec "./Standalone/Field Studio" "$@"\n')
    launcher.chmod(0o755)
manifest={p.relative_to(destination).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(destination.rglob('*')) if p.is_file() and not p.is_symlink()}
(destination/'SHA256.json').write_text(json.dumps(manifest,indent=2)+'\n')

def archive_paths(archive,paths):
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as bundle:
        for path,relative in paths:
            if path.is_symlink():
                info=zipfile.ZipInfo(str(relative).replace(os.sep,'/'))
                info.create_system=3
                info.external_attr=(stat.S_IFLNK|0o777)<<16
                bundle.writestr(info,os.readlink(path))
            elif path.is_file():
                bundle.write(path,relative.as_posix())
    digest=hashlib.sha256(archive.read_bytes()).hexdigest()
    archive.with_suffix(archive.suffix+'.sha256').write_text(digest+'  '+archive.name+'\n')
    print(archive)

archive_paths(output/(label+'.zip'),((p,p.relative_to(output)) for p in sorted(destination.rglob('*'))))
if args.source:
    # Corresponding source is self-contained: project + pinned dependency, not
    # just GitHub's auto source tarball that omits the JUCE checkout.
    tracked=subprocess.check_output(['git','ls-files','-z'],cwd=root).decode().split('\0')
    source_label=f'Field-Studio-{version}-source'
    sources=[(root/name,Path(source_label)/name) for name in tracked if name and (root/name).is_file() and not name.startswith('vendor/')]
    metadata=output/'source-build-info.json'
    metadata.write_text(json.dumps(dict(build_info,platform='source',architecture='source'),indent=2)+'\n')
    sources.append((metadata,Path(source_label)/'BUILD_INFO.json'))
    sources.extend((p,Path(source_label)/'vendor/JUCE'/p.relative_to(juce)) for p in sorted(juce.rglob('*')) if '.git' not in p.relative_to(juce).parts and (p.is_file() or p.is_symlink()))
    archive_paths(output/(source_label+'.zip'),sources)
