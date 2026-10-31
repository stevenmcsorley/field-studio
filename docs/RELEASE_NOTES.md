# Field Studio 0.2.1 beta 1

Reliability update to the portable multi-platform beta of the independent synthesizer, sampler and four-track stereo tape instrument. Exact OP-1 field sound, feature and visual parity is not established.

## Licensing

Original Field Studio code/graphics: **MIT**. Combined app/plugin binaries with JUCE: **AGPLv3**, with complete corresponding source provided at this release. Free to use, including commercially; redistribution of the combined build must comply with JUCE/AGPLv3 terms. See LICENSING.md in the downloads.

## Downloads

- **Windows x64:** standalone + VST3.
- **macOS universal (Apple Silicon and Intel, macOS 11+):** standalone + VST3 + AU. Both architectures are checked with lipo; tests execute on the CI runner's native architecture.
- **Linux x86_64:** standalone + VST3, built on Ubuntu 22.04 (glibc 2.35+).
- **Source:** project and pinned JUCE dependency together, including build scripts and dependency licence files.

Each download has a SHA-256 sidecar. Binary bundles include installation instructions, dependency notices and a per-file hash manifest. These are portable archives, not signed installers. Windows binaries are unsigned; macOS binaries are not Developer ID signed/notarized. Commercial DAWs, physical audio devices and MIDI controllers still need hands-on validation.

## Fixed in this beta

- Large record/capture/album reserve allocation and prefaulting happen outside the live lock.
- Tape edits use independent prepared state and reject stale sources. Old history and media are reclaimed after unlocking.
- Project/new-project DSP preparation and resampling, sample/patch preparation, imported-sample serialization and UI waveform calculations move out of the live lock.
- Tape pools have a shared 2 GiB default reservation cap, with COM controls and recoverable admission failures. Save/export work is serialized per processor to reduce overlapping buffers.
- Stop and newer-project operations invalidate suspended preparations. Five allocation-pause test paths verify pilot audio continues during heavy preparation.

## Known limitations

The short commit/UI lock still exists: a contended callback can still produce silence. The final command/telemetry migration, asynchronous UI job coordination, total working-memory admission and real device/DAW deadline checks remain unfinished. This is not a seamless live-performance release. Synchronous preparation can still stall the UI thread if initiated there.

The pool cap covers paged tape storage, not all process RAM; separate formats/processes may have separate caps. Retained history/saves count against their pool reservations. Clear unused media and discard history to release them. Back up projects before testing. See docs/KNOWN_ISSUES.md for the precise scope.

Downloads remain unsigned/not notarized: no signing certificates are available. Physical Faderfox/BeatStep, audio-device/commercial-DAW compatibility and exact hardware equivalence remain unverified. MIDI learn, reusable profiles and guide-based screen improvements from 0.2.0 are retained.

## Verification

Release publication is gated on Linux, Windows and macOS build/tests, core ASan/UBSan, all package manifests/checksums and complete target downloads. The Linux suite loads the actual VST3 module. Automated success does not substitute for audible device or commercial-DAW testing.
