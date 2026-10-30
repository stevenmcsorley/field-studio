# Build and release status

Local Linux x86_64 standalone and VST3 builds pass all six test suites. The native VST3 harness checks module loading, audio and optional stem buses; it does not substitute for commercial DAW testing.

GitHub Actions run [41111515955](https://github.com/stevenmcsorley/field-studio/actions/runs/41111515955) passed Windows and macOS build/tests but failed Ubuntu configuration because fontconfig development files were missing. Commit `4f8b24c` adds `libfontconfig1-dev`, explicit pkg-config installation and font dependency checks. The replacement three-platform run [41116464808](https://github.com/stevenmcsorley/field-studio/actions/runs/41116464808) passed build, tests and artifact upload on Linux, Windows and macOS. The subsequent tape commit `58ff589` also passed all three platforms in [run41136592201](https://github.com/stevenmcsorley/field-studio/actions/runs/41136592201). Later implementation changes need their own CI result.

Source is committed and pushed to the private `stevenmcsorley/field-studio` repository. No GitHub release is published. CI artifacts are development builds, without installers or signing.

## Reliability and release gates

1. Establish passing build/tests on Linux, Windows and macOS from the checked-in dependency list and pinned JUCE revision.
2. Test audible playback/recording on actual audio devices, physical MIDI controllers and commercial DAWs. Record OS, device/driver, sample rate, buffer size, DAW and plugin format for each result. Device enumeration and an automated plugin harness do not establish this gate.
3. Implement and validate the tape editing/performance phase of the ordered parity roadmap.
4. Replace shared live-engine locking using the ownership, prepared-media and bounded-command approach in [LIVE_AUDIO_DESIGN.md](LIVE_AUDIO_DESIGN.md). Verify uninterrupted audio during large imports, saves and edits; moving disk I/O alone is insufficient.
5. Prepare a beta: packaged binaries, dependency/licensing review, installation instructions and signing/installers where required. Public visibility and release publication are separate from the current private source repository.

The audio callback still clears its output when `engineLock` is unavailable. This is an unresolved performance blocker, not fixed by the CI dependency change.

Use `scripts/run.sh` for the development standalone and `scripts/package.py` for a local bundle containing standalone, VST3, documentation and SHA-256 hashes. Packaging does not install plugins or change DAW configuration.
