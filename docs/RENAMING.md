# Field Studio → OPVYN

OPVYN is the new name of this independent, open-source instrument inspired by the Teenage Engineering OP-1 field. The repository is now `stevenmcsorley/opvyn`. GitHub redirects old repository links. The local working-directory name can remain `field-studio`; a fresh clone uses `opvyn`.

## What changes

- App/plugin name and panel branding: **OPVYN**.
- Standalone binary: `OPVYN`, `OPVYN.exe` or `OPVYN.app`.
- Plugin bundles: `OPVYN.vst3` and macOS `OPVYN.component`.
- Download folders/archive prefix: `OPVYN-`.
- New MCP manifest/server display name: `opvyn`.

Tagged releases through 0.4.2 retain the original branding and filenames. Version 0.4.3 is the first OPVYN release. Renaming the repository does not rebuild or replace those downloads. Current source and future build artifacts use OPVYN.

## Existing projects, presets and controllers

The `.field` project extension, `FIELD_STUDIO` state signature/version, parameter IDs/order, MIDI mapping IDs, preset formats and original copyright attribution are unchanged. Existing files need no conversion.

The JUCE plugin manufacturer code `Fstd`, plugin code `Fld1`, bundle ID `studio.field.instrument` and CMake target `FieldStudio` are retained deliberately. With VST2 replacement disabled, JUCE generates VST3 class IDs from the manufacturer/plugin codes, not the display name. AU type/subtype/manufacturer codes are retained as well. These choices preserve plugin identity, but physical host upgrade/rescan behaviour still needs checking in each DAW.

Replace the old plugin bundle rather than leaving both Field Studio and OPVYN in scanned plugin folders: they intentionally have the same plugin identity. Restart/rescan the DAW, then check existing sessions. Keep a backup of sessions and the previous bundle until your host has reopened them successfully. A stale host cache may still show the old name.

User presets remain in the existing `Field Studio/Presets` application-data directory. MCP discovery remains in `FieldStudio/control`; the packaged server remains at `integrations/field-studio/server.py`, with the same endpoint schemas and `fieldstudio` JavaScript API. Existing client configurations can keep their old label/path. These internal names are compatibility identifiers, not missed branding changes.

Standalone audio/MIDI preferences are managed by JUCE under the application name. OPVYN may start with fresh device preferences; select your interface and MIDI inputs in Options → Audio/MIDI Settings. Presets and projects remain in their existing locations.

No DSP, synthesis parameters or tape/recording behaviour is changed by the rename. Original native images under `docs/screenshots` may document older releases; the README hero preview reflects current branding.

Rename validation: Linux standalone and VST3 builds completed, the generated processor/controller class IDs matched the previous bundle, and a portable Linux package passed checksum/content verification. The README hero was rendered from the renamed native editor. No broad local suite or CI watch was run. Windows/macOS packaging paths were updated; fresh binary validation there remains the release workflow’s responsibility.
