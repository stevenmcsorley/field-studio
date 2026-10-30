# Validation record

2026-10-30, Linux x86_64. This records tested implementation behaviour, not OP–1 field equivalence.

The final Release build produced standalone and VST3 artifacts. All four CTest targets pass:

- **Engine:** tuning/envelopes, all15 engine branches including per-pad DBox, finite output,12-band vocoder, independent FX, sample trim/loop/reverse/one-shot/pad/choke behaviour, stereo record/overdub, undo/redo/lift/drop/splice, rate conversion, loops/reverse, stem routing, offline coloration, LFO routing,999-step Endless, latched Hold, Finger bank selection, drawn Sketch pitch/time/rests, swing/gate and Tombola collision audio/rotating boundaries/clear/pause. Selected renders assert zero `operator new` calls.
- **Plugin:** sample-accurate MIDI note onset, sustain release, WAV import, independent synth/drum patch slots, memo recall, sample/DBox-pad/Sketch/Finger/tape-bank persistence, compressed project and host state,44.1→48kHz tape conversion, invalid-load preservation, input/output sampling, live A/B capture/playback/export/reload, host tempo/start/stop/small seek/varispeed, populated stereo stem buses, editor construction, typing-note release and rendering every engine view.
- **Preset:** synthetic AIFF/AIFC container and strict JSON tests, malformed lengths/padding/duplicates/Unicode/nesting/number limits, raw/unknown-property preservation and all15 metadata engine names. A separate run also passes comparisons against167 public research fixtures.
- **VST3 host:** the actual shared object loads and processes through SDK interfaces with four optional stems disabled and enabled. Tests cover instrument enumeration, initialization, MIDI eventoffset128, finite/nonzero main audio, silence before onset, written/finite empty stems, note-off and clean teardown. See [scope](VST3_SMOKE.md).

The updated core suite also passes AddressSanitizer and UndefinedBehaviorSanitizer. Large test-only engine objects were moved to the heap after ASan exposed a harness stack overflow; production processors are heap-created by the plugin factory. This does not constitute full GUI/plugin sanitization or a long-session stress test.

Native screenshot generation passes. Images in `docs/screenshots` cover all15 engine choices, the five main modes, module pages and all8 sequencer selections. They were regenerated with truncating output streams and inspected for Unicode symbols, geometry, readable controls and actual Sketch/Tombola state. The constructor resize-order crash is fixed and native editor creation is now mandatory in the integration test. No pixel-level comparison against every hardware screen has passed.

The standalone process remained alive throughout an intentional four-second Xvfb startup smoke test and logged no errors. It was terminated by the test timeout (exit124). Physical-device listening, microphone capture and manual MIDI-device setup remain unverified; buffer tests are not proof of a successful real audio-device session.

## Reproduce

From the project root:

```sh
xvfb-run -a ctest --test-dir build --output-on-failure
build/field_preset_tests_artefacts/Release/field_preset_tests \
  ../op1-field-research/references/te-op1 \
  ../op1-field-research/fixture-inventory.json
xvfb-run -a build/field_plugin_tests_artefacts/Release/field_plugin_tests docs/screenshots

g++ -std=c++17 -O1 -g1 -fsanitize=address,undefined -fno-omit-frame-pointer \
  -I Source Source/Engine.cpp tests/engine_tests.cpp -o /tmp/field-engine-tests-asan
/tmp/field-engine-tests-asan
```

The optional fixture inventory is research material, not bundled app content. Headless GUI tests require Xvfb; use the desktop display on normal systems.

## Boundaries

Windows/macOS CI jobs and AU/universal-macOS configuration exist, but those builds have not run in this workspace. No commercial DAW/editor embedding, signing, installers, full hardware MIDI matrix, maximal-memory session or measured OP–1 field sound comparison is validated. The firmware investigation recovered no executable DSP or screen assets.

The tape and album buffers are RAM-backed. Six-minute tracks/eight banks are nominal capacities; all full sessions can exceed practical memory and the512MiB project import limit. Undo has up to7 snapshots with a soft256MiB budget and is not persisted. Heavy control-thread loading/saving/edits can cause skipped audio blocks under the wrapper's non-blocking lock. The allocation-free core checks do not prove callback-wide deadline compliance or glitch-free live editing. See [remaining parity gaps](PARITY.md).

## Latest envelope/import checks

The four Release CTests pass after adding supported preset-marker mapping and melodic play settings. The updated core sanitizer suite passes. New core assertions measure ADSR stages against a constant sample and cover legato envelope continuity, portamento, held-key fallback, polyphony and unison allocation. Integration tests create AIFF files containing real sample data plus APPL JSON and verify unequal drum regions/tuning/reverse, sampler root/trim, source-metadata/state preservation, unsupported-engine rejection and explicit modeled-preset limitations. Synthetic mapping tests exercise legacy mono and stereo drum marker scales. A separate private audit accepts24/24 WAV files from the official Snaps pack; reports and provenance are in the sibling research compatibility directory, not redistributed with the app.

### Sample/output gain correction (2026-10-30)

Removed the fixed 0.19 melodic / 0.42 drum voice attenuation for the sampler only. Synth/DBox calibration remains unchanged. Master range is now 0–2 (unity at 1, +6.02 dB at 2); saved physical volume values retain their meaning. Previously saved sample projects will play louder because of the corrected source gain. Stereo constant-sample regression tests check unity amplitude, clean 2x output gain and fade to silence. Existing ADSR amplitude assertions were updated to unity sample gain while preserving the original timing checks. No peak normalization modifies imported audio.

### Tape phase (2026-10-30)

All five local CTest suites pass after the tape extension. Core tests cover grouped masked edits/history, overlapping slides, mixer-aware merge/bounce, count-in timing/clicks, control cancellation and recording level/pan; callback allocation checks remain enabled. Wrapper tests check group-copy/lift/drop destinations, atomic undo, beat/eighth/bar grids, countdown hold/cancel and project persistence of record/edit settings. The native tape screenshot was regenerated and visually checked. Hardware timing/gesture/curve matching and drop-free live edits remain unverified.

### MIDI phase (2026-10-30)

All six local CTest suites pass after enabling plugin MIDI output. MIDI helper tests verify24PPQN timing, jitter/tempo change/timeout handling, transport/SPP, fractional and segmented clock timestamps, no output echo in external-clock mode, per-channel sustain/shared-pitch ownership and allocation-free preallocated helpers. Helper/test code passes ASan+UBSan. Core tests cover SPP step playback without tape seek, click level and bounded timestamped sequencer events including note releases, unison/manual-note deduplication and overflow panic. Processor tests verify channel filtering, timed thru/remapping, filter note release, CC destinations, persistent routing state, clock receive/send and sequencer note output. Physical MIDI devices, real DAW routing and hardware timing/policy matching remain unverified.

### Screen phase (2026-10-30)

All six suites pass with the state-driven UI pass. The final native integration/screenshot run also passed after marker-ordering and capture-badge corrections. Availability tests repeatedly refresh synced LFO/manual Element/Amp/glide controls; render checks span all15engines×4pages, every patch/masterFX and6LFOsources×6shapes. Default and1000×520 screens were inspected, with representative artifacts in docs/screenshots. The latest tape/MIDI core also passes a fresh ASan+UBSan build. UI artwork and animation timing are not hardware-matched measurements; see SCREENS.md.
