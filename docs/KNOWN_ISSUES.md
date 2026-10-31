# Reliability status

This ledger distinguishes concrete fixes from the remaining performance and release gates. A passing beta build is not certification for live performance or hardware parity.

## Addressed in 0.2.1 beta 1

- Tape/album/sample record preparation no longer allocates and prefaults its large reserve while holding the live engine lock. Stop and newer-project operations invalidate suspended preparations.
- Copy/lift/drop/merge/join/slide/bounce/clear/split prepare against retained tape generations. Mutating edits use separate pools, not a second writer in the live pool. A changed source, selection, document or request cancels the commit instead of erasing newer work. Undo/redo remain grouped.
- Project loading builds and resamples a separate engine/document before publication. New-project DSP preparation also occurs outside the live lock. Full replacement deliberately resets the old project; it does not preserve unrelated old notes.
- Sample import/capture/patch preparation and patch payload encoding occur outside the lock. Immutable sample snapshots let saves encode imported samples after releasing it.
- UI waveform/sample previews use retained audio outside the lock. Bank switching no longer rescans/sanitizes all audio. Page collection and discarded-history destruction run outside the live lock.
- Tape pools have a shared reservation cap: 2 GiB by default, adjustable under **COM → Tape memory budget**. Admission is checked before allocating/prefaulting. Failure retains existing media and reports an operation error. Clear a track and discard undo/redo history to release its retained pools.
- State encoding and mix/album export are serialized per processor to reduce overlapping temporary buffers. File save/export failures are reported; destination writes remain atomic.

The tape cap covers pages, nodes and pool bookkeeping shared by instances of the same loaded binary. Separate app processes/plugin formats may have separate caps. It does **not** cap decoded samples, flattened state/export buffers, framework allocations or total process RAM. The setting lasts for the process; project files cannot raise it. Existing import/decompressed-state limits still apply.

## Still open

| Issue | Status |
|---|---|
| Short commit/UI lock | `processBlock` still uses `engineLock` and clears a contended block. Large work has moved out, but command/telemetry ownership and removal of that last lock are unfinished. No claim of zero dropouts for arbitrary concurrent controls. |
| GUI responsiveness | Preparation APIs are synchronous control calls. A long operation initiated directly on the message thread can still stall that thread; a bounded asynchronous UI job coordinator remains to be implemented. |
| Whole-processor realtime guarantees | No complete callback lock/allocation/free audit, hardware deadline certification or long-session device stress pass. Host state APIs remain non-realtime calls. |
| Trusted signing/installers | Downloads are portable unsigned betas. The maintainer has confirmed no Apple Developer ID or Windows signing certificates are available. macOS notarization and publisher-trusted Windows signatures cannot be supplied without those external credentials. |
| Device/DAW validation | Physical Faderfox/BeatStep, real audio interfaces and commercial DAWs need hands-on validation. CI tests software messages, native UI/state and Linux VST3 loading. |
| Hardware equivalence | Exact synth/effect sound, hardware parameter curves, all screens and every hardware gesture remain unverified. See PARITY.md. |

## Evidence

Eight local CTest suites and the three core ASan/UBSan suites pass the implementation checks. Processor tests deliberately pause real allocations of at least 1 MiB on the control thread during sample capture, album arming, tape arming, tape-edit preparation and 192 kHz project preparation. During those pauses, pilot audio stays nonzero without skipped blocks. Stop/new-project operations then cancel the suspended job. These tests isolate **heavy preparation**, not the remaining short commit-lock window.

Core tests additionally cover stale tape edit rejection, undo, clearing under a full budget, shared budget admission/lifetime, snapshot readers and bounded recording writes. Real-device timing, trusted signatures and exhaustive concurrency acceptance are not established by these tests.
