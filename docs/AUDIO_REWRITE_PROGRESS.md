# Audio ownership rewrite — live paged storage

Status: foundations and live paged tape/capture storage implemented, 2026-10-31. The application still uses `engineLock`. Record preparation, project loading, control edits and remaining state work can still interrupt playback. This is not a completed dropout-free rewrite.

## Integrated into the application

### Keyboard handoff

`RtQueue.h` and `RtKeyboard.h` replace JUCE MidiKeyboardState in the processor. One message-thread producer feeds a bounded queue to the audio consumer; UI/received-key indicators use separate atomic bits. Audio no longer calls processNextMidiBuffer or UI keyboard listeners. Overflow and Escape publish an ordered panic fence: older queued keys are discarded while newer notes survive. GUI and host notes retain separate ownership, so releasing one source cannot cut the other. Channel sustain and channel-wide reset controls apply to both sources. GUI note release examines GUI-owned keys, not unrelated received-key indicators.

Incoming MIDI staging is preallocated and inspects at most 2048 host events plus 512 GUI events per block. Releases received during a skipped, zero-sized or oversized callback are retained in a bounded queue and replayed before newer notes, including MIDI output. Overflow falls back to panic. Unsupported oversized/SysEx events are rejected; excess input fails closed by releasing notes. These limits do not prove allocation freedom for the entire callback or its outgoing host buffer.

### MIDI parameter notification

Owned JUCE Float/Choice/Int/Bool parameters are updated through their silent atomic-value setValue path. Their native value remains immediately visible to the VST3 wrapper and DSP, preventing a host update equal to an older display value from being incorrectly discarded.

The message thread later sends value-change notifications without setting the value again. Notification snapshots/acknowledgements cannot overwrite newer automation. Tests cover reentrant host changes, saving before notification, and restoring a state equal to the old display value. State serialization copies current native parameter values, not stale APVTS display mirrors. Only the project's audited built-in parameter types are supported by this silent-update assumption; custom valueChanged overrides need a realtime audit.

### Immutable sample assets

Each sample voice retains its own immutable stereo audio, original rate, root note and playback bounds/settings. Prepared installs exchange handles without audio-thread allocation or deletion; existing notes retain the old asset while later notes use the new one. Atomic reference release defers destruction to a control-thread collector. Registry identity rejects foreign handles and survives Engine moves; moved-from engines can be reused safely.

Prepared tickets are pinned until their single-use install or cancellation. A consumed/retired raw ticket must not be reused after collection. Future queued jobs still need generation/epoch checks and acknowledgement-based cancellation.

File-import preparation and old processor sample-buffer destruction now occur outside the engine lock. The small installation/parameter transaction still takes it. Capture/patch/project compatibility paths still use the legacy locked operations. No claim of uninterrupted importing/loading is made.

## Live tape, albums and sample capture

`TapeStorage.h/.cpp` provides sparse4096-frame stereo pages, a fixed three-level64-way persistent tree, O(1) snapshots and bounded copy-on-write mutation. One write copies at most one32KiB sample payload and three fixed child tables. Each64-child table is512bytes, plus node metadata/alignment. Growing to a far position creates logical zeros without filling a long vector gap; shrinking then regrowing cannot resurrect old samples.

Pools allocate and explicitly touch their storage off audio. Writer/snapshot release marks retirement bits; one worker reclaims unreachable resources and recycles IDs. Exhaustion leaves the requested write unchanged. Published snapshots support concurrent readers/copies/releases. Each pool has exactly one allocator/writer and one collector, and must outlive every buffer using it. Concurrent prepared edits require a separate pool or an exclusive writer handoff.

`TapeAudio` owns the pool lifetime alongside its buffer. Track audio, history, clipboard payloads, all eight tape banks, album sides and sample capture now use paged storage. Track snapshots retain roots instead of copying all samples. Destructive edits and resampling use explicit control-only preparation; clipboard extraction still visits the selected range. Full edits are not yet worker-prepared revision commits.

Record arming allocates/prefaults the reserve on the control thread. The callback uses only bounded stereo `tryWrite`; a sparse seek leaves logical zero space rather than resizing/zero-filling a long vector gap. Exhaustion preserves the last committed stereo sample and stops recording without clearing playback/monitoring. The editor reports exhausted reserves. Pools are collected on control paths under existing ownership serialization; collector registry/ownership must migrate before removing that lock.

Project saves retain tape/album/capture generations under the lock and flatten those generations after unlocking. Mix exports likewise retain track/mixer state then render outside the lock. The version-1 planar float payloads are unchanged, so previous project files remain readable. Imported sample serialization, metadata capture, capture finalization and project restoration still contain locked work. No claim of fully uninterrupted saving or importing follows.

The current reserve is per pool: for N required pages, 2N+16 payload pages and 6N+64 nodes, with a complete COW-pass spare check before control mutations. This can use substantially more prefaulted memory than the old vector reserve. There is no application-wide byte cap or admission policy for overlapping saves yet. Pinned generations can exhaust a reserve; recording stops explicitly instead of allocating in the callback.

## Validation

Eight local CTest suites pass: engine, RT queue/notifications, tape storage, native processor/UI/project, MIDI, preset parsing, parameter translation and actual Linux VST3 loading. New core paths have allocation **and deallocation** guards. Engine tests include long-note asset turnover, root/rate preservation, cancellation, foreign handles, move reuse and concurrent collection. Tape tests include randomized vector-oracle comparisons, exhaustion, truncation and20000-version concurrent snapshot/reclaimer stress. Live integration tests include sparse recording after a 300-second seek, immutable concurrent reader generations, copy/move pool lifetime, actual pinned-generation exhaustion/recovery, continued monitoring after reserve failure, and restoring saves made during sample/album capture. Queue tests include500000 concurrent messages and stale parameter acknowledgements, including an A→B→A control race.

ASan/UBSan pass for engine, storage and queue components. ThreadSanitizer could not start in this environment (`unexpected memory mapping`, including a non-PIE attempt); no TSan pass is claimed. CI includes a reproducible core ASan/UBSan job. Physical devices, commercial DAWs, callback deadline stress and continuous save/edit playback acceptance remain outstanding.

## Remaining migration

1. Move pool provisioning/collection and destructive edits to prepared worker jobs with document revisions; add a global media budget and save admission policy.
2. Move all production Editor/control mutations behind bounded commands and all reads to owned telemetry snapshots.
3. Add prepared project commits, immutable save/export generations, revision/rate-epoch checks and off-thread retirement/backpressure.
4. Remove public mutable Engine access and the callback lock only after that ownership migration is coherent.
5. Run the whole-processor continuous-audio, allocation/free/lock and timing gates in LIVE_AUDIO_DESIGN.md.
