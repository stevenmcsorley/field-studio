# Field Studio 0.3.0

Regular multi-platform release of the independent synthesizer, sampler and four-track stereo tape instrument. Portable Windows x64, macOS universal and Linux x86_64 downloads include standalone and VST3; macOS also includes AU. Complete corresponding source and SHA-256 checksums accompany the binaries. Downloads are unsigned.

## Continuous audio ownership

- Removed the shared engine lock from the production audio callback. The editing model and private render document now have separate ownership.
- Prepared project/sample loads, tape edits, undo/redo and bank changes publish at block boundaries with generation, epoch and media validation.
- Retained snapshots support save/export/preview during playback. Resource destruction and media preparation stay on the control side.
- Preserve unrelated controller changes during prepared edits and retain outgoing tape-bank state. Recording stops are acknowledged before destructive edits; sample finalization receives the complete captured take.
- Compound parameter changes publish coherently; stop/panic retain independent delivery. Capture-device shutdown waits for readers outside audio.
- Added a production ownership suite covering four sample rates and three block sizes, concurrent operations, MIDI, heap allocation/free instrumentation and synth/effect output comparisons. Added a live-ownership ASan/UBSan release gate alongside core sanitizers.

Existing effects, synth sounds, presets and parameter IDs are unchanged by this release. Projects retain their existing format.

## Included features

Dedicated synth/effect LCDs, preset snapshots and folder management, four stereo tape tracks and eight banks, tape editing/history, album recording, mixer/EQ/master controls, sequencers, vocoder and input/output sampling, generic MIDI clock and transport, MIDI learn with relative encoders, DAW automation and optional stem outputs. MCP includes local Code Mode control. See the README for screenshots, controls and setup guides.

## Validation and limits

Publication requires Linux, Windows and macOS builds/tests, MCP tests, core and live ownership sanitizer checks, and verification of all OS/source packages. Automated tests do not certify physical driver latency, every DAW or long-session glitch freedom. Deliberate edits/full project replacement can change sound abruptly. Oversized host blocks beyond prepared capacity are rejected. State APIs and device changes are non-realtime operations.

Exact OP-1 field sound/feature/visual equivalence, whole-process RAM admission, signing/installers and broader device listening tests remain open. See [known issues](KNOWN_ISSUES.md) and [ownership design](LIVE_AUDIO_DESIGN.md).
