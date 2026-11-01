# Audio and UI performance

Beta 9 reduces recurring DSP work and GUI interference. It does not complete the audio-ownership rewrite or establish dropout-free playback.

## Settings and diagnostics

Open **COM → Audio / performance** for diagnostics, **48 kHz/256 frames** (5.33 ms per buffer), **48 kHz/512 frames** (10.67 ms), or **output-only playback**. Existing settings stay unchanged until selected. Presets preserve selected devices, reject changes during recording/sampling and attempt rollback on setup failure. Output-only disables device input; re-enable it in Audio/MIDI Settings before input sampling. Plugin audio settings remain host-controlled.

Diagnostics report negotiated backend/device, rate, buffer size, device-reported latency and xruns in standalone, plus callback mean/peak load, deadline overruns, skipped blocks and separate capture queue/recovery statistics. Reset app counters, reproduce the problem and reopen diagnostics; device xruns are not reset. A 44.1 kHz/1024-frame setup contributes 23.22 ms per app buffer before server/device delays. Buffer duration is not measured round-trip latency. See [JUCE devices](https://docs.juce.com/master/classjuce_1_1AudioIODevice.html) and [PipeWire latency](https://docs.pipewire.org/devel/page_latency.html).

## Changes

- Cache unchanged oscillator frequency, sample pitch increment and filter coefficients; precompute DNA hash cells using the original rounding.
- Fixed-size triple-buffer GUI telemetry removes the routine synth display engine lock. Separate preset metadata caching and retained pool handles reduce maintenance contention.
- Repaint changing LCD/value/meter regions and cache/throttle media previews.
- Adaptive resampling follows independent external-capture clocks. Excess backlog is discarded rather than retained as increasing delay; recovery can click and is counted. Main device/host input is unaffected.
- MCP state includes render load, peak, overruns and skipped blocks.

## Measurements and verification

Build Release with BUILD_TESTING enabled, then run `build/field_performance_bench_artefacts/Release/field_performance_bench`, using `xvfb-run -a` when headless. Timings are not CTest thresholds.

Committed [before](benchmarks/before-beta9.json) and [after](benchmarks/beta9.json) runs use Linux, 48 kHz/256 frames, 100 warm-up and 700 measured callbacks per scenario. Mean callback time improved by **17–53% across six active workloads**; all seven output checksums match. These are synthetic render timings, affected by scheduling/CPU state, not physical latency measurements. Checksums do not prove hardware sound equivalence.

Offscreen painting measured 23.6 ms for the full editor versus 1.37 ms for an LCD-sized clipped region. This demonstrates smaller repaint cost, not live FPS or overall app speedup.

Regression tests cover concurrent snapshot integrity, GUI refresh while the engine lock is held, and ±500 ppm independent-clock drift over 320 simulated seconds each with bounded queues and no underruns/drops/recoveries. Deliberate backlog recovery is tested separately. Eight native suites, nine MCP tests and three core sanitizer suites passed locally.

## Remaining limits

The callback still clears a contended block if the engine lock is unavailable. Explicit controls, edits and loading can still contend; complete audio-thread ownership remains planned. Physical Linux playback, long sessions, loopback latency and driver/DAW combinations require further validation. Capture backlog recovery can click. No global PipeWire settings are changed.

## Beta 10 control and preparation

Frequent UI mixer level/pan/mute/solo, sampler/DBox pad parameters, scrub, play/rewind, track selection, selected sequencer edits and Sketch point updates can be queued as fixed payloads. Audio drains at most 32 ordinary commands per callback. There are 512 queue slots; excess submissions are rejected and reported. Stop/panic use separate atomic requests and invalidate older queued commands. Tape performance holds/releases use an atomic mailbox, so a full queue cannot lose a release. This queue does not replace every remaining control guard. Guarded save/preset/memo snapshots, bank changes and preparation boundaries also flush at most 512 queued controls, including when callbacks are suspended, so paused-host saves retain edits; that legacy control-side handoff is serialized by the engine guard.

New/Open/Import, tape record preparation, prepared tape edits, sample capture preparation/finalization and album recording preparation have one background media worker per processor, without an unbounded backlog. The existing save/export worker remains separate. Prepared results are published by the message thread, with existing source/document/rate/revision checks. **COM → Cancel background loading/editing** cancels preparation before publication starts. Stop cancels pending preparation, but does not throw away captured audio already being finalized. Finalization cannot be cancelled through this menu once admitted. Cancellation is cooperative: ongoing decode/allocation finishes before its temporary resources are reclaimed. Shutdown rejects a waiting publication and joins the worker. Host state APIs and direct internal synchronous APIs retain their existing contracts; they are not real-time safe.

Shift-scrub selection slides wait until release and an available worker, retaining the latest requested destination. Preset browsing/loading, device enumeration/open, undo/redo publication and some short control/read paths still run synchronously. Tape history and prepared project commits still use the engine guard and can interrupt audio. Full audio-owned project/document publication, complete callback auditing and a total process-memory budget remain unfinished.

Separate capture backlog recovery blends from the previous output into the new input over 64 frames. Underruns fade to silence; resumed input fades in. These bounded transitions reduce abrupt splices but are not a guarantee that every discontinuity is inaudible. At 48 kHz, 64 frames is 1.33 ms. Queue recovery and underrun counters remain visible.

Added regression coverage exercises worker/control publication, cancellation, exceptions, shutdown while publication waits, saturated-queue stop ordering, callback allocation/free checks, paused-host save and queued-edit conflict checks, capture transition continuity and concurrent control/render traffic at 44.1/48/96/192 kHz with 64/512-frame buffers. A nonzero input pilot detects unexpectedly cleared blocks in that isolated control workload. These are synthetic tests, not physical-driver certification, a complete concurrent load/save/edit acceptance pass, or a whole-processor allocation audit.
