# OP-1 field firmware and interoperability research

Investigated 2026-10-30. Target: official OP-1 field firmware 1.7.3+1, downloaded from `https://teenage.engineering/_software/op-1/field/op1_field_firmware_1_7_3%2B1.tfw`.

## Evidence ledger

| Question | Finding | Evidence and confidence |
| --- | --- | --- |
| Which bytes? | 63,422,288-byte file; SHA-256 `16ae08d6a30838be87639db79dd2e29e349ef51771a175a1fe5575778fe25079` | `firmware-provenance.json`, independently rehashed in `raw-inventory.json`; observed |
| Container markers? | `ba be ca fe` at offset 0 and `be ef ca fe` at offset 0x40 | `header-observations.json`, observed raw bytes; semantic field meanings unverified |
| Executable or asset payload? | No ELF, ZIP local-file, XZ, SquashFS, RIFF, PNG or Zstd magic signatures in full raw image. Six gzip candidates all reject bounded zlib validation. | `raw-inventory.json`, `header-observations.json`; observed bounded-format search, does not prove those formats absent inside encryption |
| Why no readable DSP? | Every 1 MiB sample window has very high byte entropy (7.999636–7.999854 bits/byte; final short window included). | `raw-inventory.json`; observed. Encryption/compression is an inference, not a recovered cipher/key |
| REA readiness? | Codex registration is healthy; Hopper is installed. Child session does not expose REA MCP. Pinned 6.7.0 CLI is usable. Firmware scan provider is unavailable because `REA_BINWALK_COMMAND` is unset/no supplied Binwalk. | `rea-doctor.json`, `rea-firmware-scan.json`; actual REA diagnostics, not successful firmware Evidence |
| Did we decompile engines? | No. No CPU architecture, memory map, executable regions, synth algorithms or UI assets have been established. | Investigation boundary; unresolved |
| Public format reference? | Community repository documents AIFC presets and known knob layouts/ranges. | `references/te-op1`, commit `24ac03e2de2f37c43272417d732ffd1f4e69636d`; primary community researcher, not manufacturer specification |
| Is that reference consistent with fixtures? | Independent Python IFF parsing validated 167 oracle `.aif` fixtures without malformed chunk boundaries, covering 15 engine type names. APPL chunks carry `op-1` + JSON metadata. | `fixture-inventory.json`; observed local parsing of the repository's fixtures |
| Do fixture sound chunks prove DSP parity? | No. There are only two distinct SSND payload hashes; 166 fixtures share one payload. | `fixture-audio-groups.json`, `fixture-audio-observations.json`; observed. Treat as patch-format examples, not synthesized audio recordings |

No firmware was executed, flashed or modified. No client registration, provider installation, or system settings were changed. No REA native target was opened, so no active native session needs closing.

## Practical implementation reference

Community primary source: [jstnfst/te-op1](https://github.com/jstnfst/te-op1). It targets firmware 1.7.3 and includes hardware-oracle patch exports; its claims still require independent testing when used for compatibility.

Reference parameter groups in `public/layout-params.js`:

- Synth engines: amp, cluster, dbox, digital, dimension, dna, drwave, dsynth, fm, phase, pulse, sampler, string, vocoder, voltage.
- FX: cwo, delay, grid, mother, nitro, phone, punch, spring, terminal.
- LFO: element, midi, random, tremolo, value, velocity.
- Common ADSR and dbox dynamic-envelope groups.

All parameter names are captured as format facts in `fixture-inventory.json`. This is not an algorithm specification. A control called FM AMOUNT does not establish modulation scaling, operator routing, oscillator shape, oversampling, saturation, envelopes or exact audio output.

Preset format documented there and corroborated by local samples: FORM/AIFC container with FVER, COMM, APPL, SSND chunks. APPL starts `op-1` and stores JSON. Sample compression is `sowt` (little-endian signed PCM), with big-endian IFF chunk lengths and even-byte padding. Synth metadata commonly includes type/name/knobs/adsr/fx/lfo fields, while dbox uses per-pad arrays. Preserve original raw parameter values and unknown fields in an importer; do not assume every knob is normalized 0..32767. Signed and limited-range controls exist. Export key ordering requirements are a community claim that still needs a hardware round trip.

Tape format observations from the community README: four WAV tracks and tape.json per tape; tape_type IDs 0 studio, 1 vintage, 2 porta, 3 disk mini. Treat other undocumented field semantics as unresolved until fixture-backed. Their repository has no root LICENSE file in this checkout: implementation should be independently written from documented format facts; do not vendor its UI/code/sample assets as though permissively licensed.

## Useful next steps

1. Build independent AIFC preset import with strict IFF bounds checking, all 15 engine names, signed raw parameter preservation and unknown-field round trip. Test metadata against the 167 public fixtures without bundling their audio into the application.
2. Use official guide/release notes and timestamped public demos to specify each screen, key combination, tape operation and control response. Record uncertain mappings explicitly.
3. Implement desktop-native synth/tape/effects independently; label approximate DSP honestly. Test sample-accurate transport, finite/bounded output, host state restoration, MIDI note lifecycle, sample import and offline renders.
4. Firmware decompilation needs an actually decoded executable payload plus verified CPU/load mapping. High entropy alone does not establish encryption type or a usable key. Installing Binwalk would improve signature tooling but would not decrypt this image.
5. Audio parity cannot currently be established from these fixtures. Controlled note/parameter recordings from a reference device would be the strongest next sonic evidence; demos permit qualitative comparisons only.

## External references and limits

- Official firmware/release notes: https://teenage.engineering/downloads/op-1/field
- Official guide: https://teenage.engineering/guides/op-1
- Community patch-format research: https://github.com/jstnfst/te-op1
- Primary researcher account of downloadable encrypted firmware: https://wmealing.bluegum.systems/reverse-engineering-teenage-engineering.html
- OP-Z-specific research `references/opz-firmware-notes` is historical context only. Its device, container, hardware oracles and unresolved key claims do not establish OP-1 field behaviour; no scripts from it were executed.

Feature parity: unresolved (requires implementation and behaviour tests). Visual parity: unresolved (requires screen-level reference comparisons). Sonic identity: unresolved. This report establishes research inputs and concrete format facts only.
