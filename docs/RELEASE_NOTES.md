# Field Studio 0.2.0 beta 1

First portable multi-platform beta of the independent synthesizer, sampler and four-track stereo tape instrument. Exact OP-1 field sound, feature and visual parity is not established.

## Licensing

Original Field Studio code/graphics: **MIT**. Combined app/plugin binaries with JUCE: **AGPLv3**, with complete corresponding source provided at this release. Free to use, including commercially; redistribution of the combined build must comply with JUCE/AGPLv3 terms. See LICENSING.md in the downloads.

## Downloads

- **Windows x64:** standalone + VST3.
- **macOS universal (Apple Silicon and Intel, macOS 11+):** standalone + VST3 + AU. Both architectures are checked with lipo; tests execute on the CI runner's native architecture.
- **Linux x86_64:** standalone + VST3, built on Ubuntu 22.04 (glibc 2.35+).
- **Source:** project and pinned JUCE dependency together, including build scripts and dependency licence files.

Each download has a SHA-256 sidecar. Binary bundles include installation instructions, dependency notices and a per-file hash manifest. These are portable archives, not signed installers. Windows binaries are unsigned; macOS binaries are not Developer ID signed/notarized. Commercial DAWs, physical audio devices and MIDI controllers still need hands-on validation.

## New in this beta

- MIDI learn for parameters, mixer, selected drum pad and basic tape transport. Absolute pickup/jump, three relative encodings, reverse direction, project/host-state persistence and reusable controller profiles. See docs/CONTROLLERS.md for Faderfox/BeatStep setup.
- Paged live tape, undo history, all eight banks, albums and sample capture. Sparse recording writes bounded pages rather than filling a long vector gap. Tape/capture save flattening and mix rendering run outside the engine lock.
- Immutable sample lifetimes, deferred parameter notifications, ordered keyboard panic and recovered MIDI releases.

## Known limitations

The shared engine lock still exists. Heavy edits, record preparation, imported sample serialization and project loads can produce silent skipped blocks. A global pool memory budget, prepared edit jobs and final command/telemetry ownership migration are unfinished. Pool exhaustion stops recording while preserving playback and recorded material. This is not a seamless live-performance release.

The app reserves and prefaults significant RAM before recording, especially at high sample rates. Avoid maximum-size sessions until the memory admission policy is complete. Back up `.field` projects before testing. Hardware sound/interaction equivalence, long-session performance, real-device/DAW compatibility, signed installers and notarization are not established by CI.

## Verification

Release publication is gated on Linux, Windows and macOS build/tests, core ASan/UBSan, all package manifests/checksums and complete target downloads. The Linux suite loads the actual VST3 module. Automated success does not substitute for audible device or commercial-DAW testing.
