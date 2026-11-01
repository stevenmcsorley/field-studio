# Continuous audio ownership — 0.3.0

The production `processBlock` owns a private render document. It never acquires `engineLock`: that mutex now serializes only the editing model and control-side publication. Effects, synth algorithms, preset formats and parameter IDs are unchanged by this migration.

## Ownership boundary

- `DocumentGuard` refreshes the editing model from acknowledged snapshots, records changes and prepares a commit off audio. Related controls coalesce before consumption. Parameter batches use a serial so audio retains the previous complete batch during preparation.
- The callback installs a prepared commit at a block boundary. Generation, sample rate, lifecycle epoch and source-media checks reject stale replacements. Per-property mixer/pad/sequence changes preserve unrelated MIDI edits made during preparation.
- Live tape, album and sampling writes use preallocated paged COW storage. Snapshot publication uses a three-slot ownership exchange. Saves, exports and previews retain stable generations and flatten them off audio.
- Root, pool, sample and bank-metadata registries pin owners before publication. The callback exchanges retained handles; acknowledged packets and last heap owners are destroyed on the control thread. Existing sampler voices keep their immutable sample until release.
- Stop/panic and performance releases have independent atomic delivery. Keyboard/MIDI handoff is bounded; host-notification mirrors are deferred. Source-device shutdown waits for readers on the control side.
- Recording stops are acknowledged before destructive edits or bank changes. Capture finalization uses the completed take returned by the callback, including its last recorded block.

`EngineDocument.cpp`, `AudioOwnership.cpp` and `OwnedSnapshots.h` implement this boundary. The public processor `engine` is the editing model, not the live engine. Legacy direct-engine fixtures explicitly opt into offline mode; they are not production concurrency evidence.

## Automated evidence

The `audio_ownership` suite exercises the production callback at 44.1, 48, 96 and 192 kHz with 32, 64 and 512-frame blocks. It holds the editing mutex on another thread while checking nonzero finite pilot output and MIDI release. Concurrent saves, exports, imports, project loads, edits, undo/redo and bank changes run during continuous callbacks. Callback allocation/deallocation hooks check those paths. Additional cases cover compound parameter publication, concurrent MIDI mixer changes, bank persistence and capture finalization. Synth/effect comparisons against the prior offline engine path check unchanged output.

The release pipeline runs this suite on Linux, Windows and macOS, plus a separate Linux ASan/UBSan build. Core tests independently stress snapshot exchange, retained sample/tape lifetime and bounded storage. Passing tests establish the exercised paths, not exhaustive concurrency proof or physical-device deadline certification.

## Practical limits

Host state APIs, preparation and device changes are non-realtime operations. Hosts must quiesce callbacks for prepare/release/destruction. A paused host leaves prepared edits pending until rendering resumes; state saves retain the editing model's desired changes. Host blocks exceeding prepared scratch capacity are rejected and counted in diagnostics.

The tape-pool reservation cap does not cap decoded samples, framework objects or total process RAM. Finite queues and memory admission can reject work visibly. Deliberate edits or whole-project replacement may change sound abruptly; continuous nonzero pilot output does not prove every edit is click-free. Real-device latency, external capture clock behavior, deadline timing and long-session listening remain validation work.
