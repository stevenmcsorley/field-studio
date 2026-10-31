# Audio ownership rewrite — staged media preparation

The 0.2.1 beta moves the large media operations outside the live engine lock. The final short commit/UI lock remains. See [KNOWN_ISSUES.md](KNOWN_ISSUES.md) for fixed and outstanding items; this is not a completed dropout-free rewrite.

## Implemented boundaries

Keyboard events use a bounded handoff with ordered panic fences and independent host/GUI ownership. MIDI parameter changes update audited built-in atomic parameter values and defer listener notification without overwriting newer automation. Sample voices retain immutable audio/rate/root/bounds; reclamation happens off audio.

Tape, history, clipboard payloads, all eight banks, albums and capture use sparse 4096-frame stereo pages and fixed-depth copy-on-write trees. Pools are prefaulted on control threads. Callback writes do not allocate, free, collect or acquire the budget/collector mutexes. Exhaustion stops recording while retaining committed stereo audio and monitoring.

`TapeAudio::preparedCopy` creates an independent writable pool from a retained immutable generation. Record arming uses it outside `engineLock`. Tape edits operate on private prepared state, then verify source pool/root/length, cuts/mixer metadata, selection, document epoch and request revision before a short commit. Stale work is rejected. Clear does not allocate a replacement pool, so it can work under a full budget. Retired history is moved out and destroyed after unlocking.

Project loading validates/decompresses data, constructs DSP, resamples active/inactive banks, prepares samples and parses controls before acquiring the commit lock. New-project preparation follows the same retirement policy. Commit exchanges prepared/old state; old engines, tapes, samples, slots and capture pools are destroyed after unlocking. Full project replacement intentionally replaces the old voices/transport.

Imported samples are immutable shared control snapshots. File import, capture finalization and patch recall prepare their payloads before publication, with request/document cancellation. Patch save and project save retain snapshots, then encode samples/metadata/slots outside the lock. Mix and album export render outside it. A per-processor non-audio encoding mutex serializes saves/exports; state I/O is not realtime-safe.

UI previews retain sample/tape snapshots under a short guard and calculate their decimation after unlocking. The timer obtains pool ownership handles, then collects pages off the live lock. A collector-only mutex serializes reclamation with control preparation; the callback never takes it. Validated bank swaps avoid a full audio sanitation scan.

## Resource admission

Tape pools reserve bytes before allocation/prefaulting. Default cap: 2 GiB, shared by instances using the same loaded binary. COM can select 512 MiB, 1, 2 or 4 GiB, show reservations and discard history. The cap cannot be lowered beneath live reservations. Pool construction failure releases its reservation through RAII. Other plugin formats/processes may have independent budgets. Decode/sample/flattened buffers are not covered by this cap; it is not a total RAM limit.

## Validation

Eight local CTest suites pass, as do core engine, storage and queue ASan/UBSan checks. Processor tests suspend actual large allocations during five preparation paths and verify continued pilot audio, unchanged skip count, and rejection after Stop or a newer project. Core tests cover prepared edits/undo/stale rejection, clear at a full cap, pool budget lifetime, randomized tape oracles and concurrent snapshot/reclaimer use. TSan remains unavailable in this environment; no TSan pass is claimed.

## Remaining architecture

1. Replace short commit/UI locking with audio-owned commands, acknowledged prepared handles and owned telemetry; remove public mutable Engine access.
2. Add a bounded asynchronous UI job coordinator with cancellation and shutdown ownership. Current preparation APIs are synchronous control calls.
3. Bound all media/serialized working memory and simultaneous retained generations, beyond the tape-pool cap.
4. Complete callback allocation/free/lock and timing instrumentation, full concurrent load/save/edit/undo/bank stress, and physical device/DAW validation.

`processBlock` still clears output if `engineLock` is unavailable. The large-work allocation tests do not prove that the short commit window can never contend. Do not label this release seamless live-performance-ready.
