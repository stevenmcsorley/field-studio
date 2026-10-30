#!/usr/bin/env python3
"""Package locally built binaries without installing or publishing them."""
from pathlib import Path
import hashlib
import json
import platform
import shutil
import zipfile

root = Path(__file__).resolve().parents[1]
release = root / 'build' / 'FieldStudio_artefacts' / 'Release'
system = platform.system()
architecture = platform.machine()
label = f'Field-Studio-0.1.0-{system.lower()}-{architecture}'
destination = root / 'dist' / label
standalone = release / 'Standalone'
vst = release / 'VST3' / 'Field Studio.vst3'
if not standalone.exists() or not vst.exists():
    raise SystemExit('Build the standalone and VST3 targets first.')
if destination.exists():
    shutil.rmtree(destination)
destination.mkdir(parents=True)
shutil.copytree(standalone, destination / 'Standalone')
shutil.copytree(vst, destination / 'VST3' / vst.name)
if system == 'Darwin' and (release / 'AU').exists():
    shutil.copytree(release / 'AU', destination / 'AU')
(destination / 'README.md').write_text((root / 'README.md').read_text().replace('../op1-field-research/FIRMWARE_FINDINGS.md', 'research/FIRMWARE_FINDINGS.md'))
research=destination / 'research'
research.mkdir()
for name in ('FIRMWARE_FINDINGS.md','SPECIFICATION.md'):
    source=root.parent / 'op1-field-research' / name
    if source.exists(): shutil.copy2(source,research / name)
(destination / 'INSTALL.md').write_text('Run ./run.sh on a Linux graphical desktop. The standalone executable is in Standalone/. Copy the entire VST3/Field Studio.vst3 bundle to ~/.vst3 or add VST3/ to your DAW plugin scan paths. Source and build instructions are in the accompanying workspace project; Windows and macOS binaries are not included in this Linux bundle.\n')
shutil.copytree(root / 'docs', destination / 'docs', ignore=shutil.ignore_patterns('screenshots'))
license_path = root / 'vendor' / 'JUCE' / 'LICENSE.md'
if license_path.exists():
    shutil.copy2(license_path, destination / 'JUCE-LICENSE.md')
sdk_license = root / 'vendor' / 'JUCE' / 'modules' / 'juce_audio_processors_headless' / 'format_types' / 'VST3_SDK' / 'LICENSE.txt'
if sdk_license.exists():
    shutil.copy2(sdk_license, destination / 'VST3-SDK-LICENSE.txt')
if system == 'Linux':
    launcher = destination / 'run.sh'
    launcher.write_text('#!/usr/bin/env bash\nset -euo pipefail\ncd "$(dirname "$0")"\nexec "./Standalone/Field Studio" "$@"\n')
    launcher.chmod(0o755)
manifest = {}
for path in sorted(destination.rglob('*')):
    if path.is_file():
        manifest[str(path.relative_to(destination))] = hashlib.sha256(path.read_bytes()).hexdigest()
(destination / 'SHA256.json').write_text(json.dumps(manifest, indent=2) + '\n')
archive = destination.parent / (destination.name + '.zip')
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as bundle:
    for path in sorted(destination.rglob('*')):
        if path.is_file():
            bundle.write(path, path.relative_to(destination.parent))
print(archive)
