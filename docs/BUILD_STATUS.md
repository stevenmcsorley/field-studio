# Release build status

Field Studio 0.4.2 is gated on Linux x86_64, Windows x64 and macOS universal standalone/plugin builds and tests; nine MCP tests; three core ASan/UBSan suites; a separate production audio-ownership ASan/UBSan suite; and verification of OS/source packages and checksums.

The 0.4.2 candidate adds a focused tape-motion test and native recording previews. Only that focused motion check was run locally for this pass; the release workflow retains its required cross-platform and ownership/sanitizer gates. The new production-path suite exercises uninterrupted pilot audio and MIDI during control contention and concurrent media/document operations, with allocation/free instrumentation. [Release workflow results](https://github.com/stevenmcsorley/field-studio/actions/workflows/build.yml) record cross-platform and sanitizer outcomes. The [0.4.2 release](https://github.com/stevenmcsorley/field-studio/releases/tag/v0.4.2) is published only after all gates succeed.

The original project code is MIT licensed; combined JUCE binaries use AGPLv3 with complete corresponding source. Downloads are unsigned portable bundles. macOS contains arm64/x86_64 and targets macOS 11 or later; CI executes only its runner's native architecture.

Owner-reported VST operation, automation, Faderfox mapping and BeatStep use are recorded in the README. Exact device/DAW coverage, long-session latency/listening, signing and hardware equivalence remain open. See [known issues](KNOWN_ISSUES.md).
