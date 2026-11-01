# Field Studio 0.2.2 beta 2

Reliability update to the portable multi-platform beta of the independent synthesizer, sampler and four-track stereo tape instrument. Exact OP-1 field sound, feature and visual parity is not established.

## Licensing

Original Field Studio code/graphics: **MIT**. Combined app/plugin binaries with JUCE: **AGPLv3**, with complete corresponding source provided at this release. Free to use, including commercially; redistribution of the combined build must comply with JUCE/AGPLv3 terms. See LICENSING.md in the downloads.

## Downloads

- **Windows x64:** standalone + VST3.
- **macOS universal (Apple Silicon and Intel, macOS 11+):** standalone + VST3 + AU. Both architectures are checked with lipo; tests execute on the CI runner's native architecture.
- **Linux x86_64:** standalone + VST3, built on Ubuntu 22.04 (glibc 2.35+).
- **Source:** project and pinned JUCE dependency together, including build scripts and dependency licence files.

Each download has a SHA-256 sidecar. Binary bundles include installation instructions, dependency notices and a per-file hash manifest. These are portable archives, not signed installers. Windows binaries are unsigned; macOS binaries are not Developer ID signed/notarized. Commercial DAWs, physical audio devices and MIDI controllers still need hands-on validation.

## Added in beta 2

- Recreated the full-LCD mixer: four audio-driven track meters, pan indicators, coloured level readouts and mute/solo indication. Shift + T1–T4 toggles track mutes on the mixer page. Click a channel to mute; Shift-click to solo.
- Recreated the EQ LCD with three band faders, a control-response illustration and red Amount fader. Orange now controls EQ Amount; per-band/overall toggles affect audio and persist in projects/DAW state. Shift + orange retains the existing drive extension. Click band columns to toggle, Amount column to bypass, MAX EQ for full amount and CLEAN for zero amount.
- Includes the new tape reel LCD, position-driven motion, loop/time/speed controls and panel keys 4–6 for momentary brake/reverse/chop.

The EQ response illustration and meter scales are original approximations, not measured hardware responses or calibrated VU meters. Existing filter crossovers, drive and dynamics algorithms remain unchanged. See docs/MIXER_EQ.md for controls and limits.

## Retained from beta 1

- Recreated CWO cow LCD, Dimension and DSynth screens, Delay circles, and permanent panel button legends, including secondary icons on sound keys 1–8. The README now shows CWO.
- CWO now blends lower/upper frequency-shifted sidebands, includes both at midpoint, and introduces dry sound as Feedback rises. Its internal mechanisms respond to controls with continuous motion.
- Delay Range moves the selected ring and centre together. Input Level controls new material entering the loop while preserving dry audio and circulating echoes. Slow settings add buffer colour. Held controls no longer produce invented circle wobble.
- CWO/Delay Shift fine-adjusts all four native controls. The optional overall wet/dry override is in COM; new selections and active native imports use full module output. Existing saved blend settings are preserved.
- Added engine/preset browsing, persistent dated snapshots, nested user folders, rename/copy/move/delete and 45 original starter presets. Fixed shifted number-key preset shortcuts.
- Project saving and WAV export run on a bounded cancellable worker. Earlier prepared-media, tape reservation and stale-operation reliability improvements are retained.

CWO/Delay sound can differ from earlier beta versions following the signal-path corrections. Back up projects before upgrading. Public hardware demonstrations informed these changes; exact hardware transfer curves, animation timing and present-firmware parity remain unverified. See docs/research/EFFECT_VISUAL_MOTION.md and docs/PARITY.md.

## Known limitations

The short commit/UI lock still exists: a contended callback can still produce silence. The final command/telemetry migration, asynchronous UI job coordination, total working-memory admission and real device/DAW deadline checks remain unfinished. This is not a seamless live-performance release. Synchronous preparation can still stall the UI thread if initiated there.

The pool cap covers paged tape storage, not all process RAM; separate formats/processes may have separate caps. Retained history/saves count against their pool reservations. Clear unused media and discard history to release them. Back up projects before testing. See docs/KNOWN_ISSUES.md for the precise scope.

Downloads remain unsigned/not notarized: no signing certificates are available. Physical Faderfox/BeatStep, audio-device/commercial-DAW compatibility and exact hardware equivalence remain unverified. MIDI learn, reusable profiles and guide-based screen improvements from 0.2.0 are retained.

## Verification

Release publication is gated on Linux, Windows and macOS build/tests, core ASan/UBSan, all package manifests/checksums and complete target downloads. The Linux suite loads the actual VST3 module. Automated success does not substitute for audible device or commercial-DAW testing.
