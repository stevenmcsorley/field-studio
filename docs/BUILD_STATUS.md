# Build and release status

Local Linux x86_64 standalone and VST3 builds pass all eight test suites. The native VST3 harness checks module loading, audio and optional stem buses; it does not substitute for commercial DAW testing.

GitHub Actions run [41111515955](https://github.com/stevenmcsorley/field-studio/actions/runs/41111515955) passed Windows and macOS build/tests but failed Ubuntu configuration because fontconfig development files were missing. Commit `4f8b24c` adds `libfontconfig1-dev`, explicit pkg-config installation and font dependency checks. The replacement three-platform run [41116464808](https://github.com/stevenmcsorley/field-studio/actions/runs/41116464808) passed build, tests and artifact upload on Linux, Windows and macOS. The subsequent tape commit `58ff589` also passed all three platforms in [run41136592201](https://github.com/stevenmcsorley/field-studio/actions/runs/41136592201). MIDI commit `ab6f133` passed all three platforms in [run41138333174](https://github.com/stevenmcsorley/field-studio/actions/runs/41138333174). Screen commit `2ecccc1` also passed all three platforms in [run41139607866](https://github.com/stevenmcsorley/field-studio/actions/runs/41139607866). Ownership foundation commit `6ed7b36` passed Linux, Windows, macOS and the core ASan/UBSan job in [run 41190451799](https://github.com/stevenmcsorley/field-studio/actions/runs/41190451799). The live paged-storage commit `ab0377e` also passed all three platforms and core sanitizers in [run 41191350222](https://github.com/stevenmcsorley/field-studio/actions/runs/41191350222). MIDI learn and the 0.2.0 beta packaging changes need a new CI result.

Source is committed and pushed to the `stevenmcsorley/field-studio` repository (confirmed public on 31 October 2026). No GitHub release is published. CI artifacts are development builds, without installers or signing.

## Reliability and release gates

1. Establish passing build/tests on Linux, Windows and macOS from the checked-in dependency list and pinned JUCE revision.
2. Test audible playback/recording on actual audio devices, physical MIDI controllers and commercial DAWs. Record OS, device/driver, sample rate, buffer size, DAW and plugin format for each result. Device enumeration and an automated plugin harness do not establish this gate.
3. Implement and validate the tape editing/performance phase of the ordered parity roadmap.
4. Replace shared live-engine locking using the ownership, prepared-media and bounded-command approach in [LIVE_AUDIO_DESIGN.md](LIVE_AUDIO_DESIGN.md). Verify uninterrupted audio during large imports, saves and edits; moving disk I/O alone is insufficient.
5. Prepare a beta: packaged binaries, dependency/licensing review, installation instructions and signing/installers where required. Public visibility and release publication are separate from the current private source repository.

The audio callback still clears its output when `engineLock` is unavailable. This is an unresolved performance blocker, not fixed by the CI dependency change.

Use `scripts/run.sh` for the development standalone and `scripts/package.py` for a local bundle containing standalone, VST3, documentation and SHA-256 hashes. Packaging does not install plugins or change DAW configuration.

## 0.2.0 beta packaging

The workflow now targets Ubuntu 22.04 x86_64, Windows x64 and macOS universal arm64/x86_64 with an 11.0 deployment target. It packages portable standalone/plugin bundles, installation notes, dependency notices, controller profiles and complete project + JUCE source, with download and per-file checksums. macOS payloads are checked for both architectures; CI only executes on the runner's native architecture. Tag publication is gated on all build/test/sanitizer jobs, matching source revisions/versions and a selected project licence. No change to repository visibility is made. New binary artifact uploads are gated on a project licence file; package creation/verification can run without publishing while that decision is pending.

The project licence decision remains pending; no release should be published until the chosen licence and corresponding source/distribution requirements are in place. Portable beta packaging does not complete signed installers, notarization, physical device/DAW validation or seamless live-audio ownership.
