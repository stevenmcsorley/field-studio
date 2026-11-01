# Reliability status — 0.4.0

The shared callback/editing lock has been removed from the production audio path. Prepared commits, callback-owned rendering, retained snapshots and deferred reclamation replace it. See [design and evidence](LIVE_AUDIO_DESIGN.md).

| Remaining issue | Status |
|---|---|
| Physical audio deadlines and long sessions | Synthetic concurrency and memory-safety checks do not certify latency, crackle-free playback or every driver/DAW. Use COM → Audio / performance and record backend, device, sample rate and buffer size in reports. |
| External capture | Adaptive clock bridging and recovery fades are implemented. Independent device clocks, underflows, overruns and source changes can still affect recordings. Physical loopback/microphone validation remains open. |
| GUI responsiveness | Heavy file/media work uses bounded workers. Preset/device operations and synchronous host/direct APIs can still stall their caller. |
| Host lifecycle | Prepare/release/destruction require quiescent callbacks. State APIs are non-realtime. Blocks exceeding prepared scratch capacity are rejected and counted. |
| Memory admission | Tape pools share a default 2 GiB reservation cap, adjustable in COM. This excludes decoded samples, flattened exports/state, framework memory and total process RAM. Separate processes/plugin binaries can have separate caps. |
| Audible edit boundaries | Deliberate replacement, cuts or full project reset can produce audible discontinuities. Zero skipped blocks does not promise click-free arbitrary editing. |
| Signing/installers | Portable downloads are unsigned. Apple Developer ID and Windows signing certificates are unavailable; no notarization or publisher-trusted signatures are claimed. |
| Device/DAW coverage | Owner reports working VST operation, automation, Faderfox assignments and BeatStep use; exact host/OS/controller details were not recorded. Broader testing remains open. |
| LFO reference limits | Original vector reconstruction and documented controls are implemented. Exact pixel/animation timing, random statistics and hardware modulation laws remain unmeasured. G-force uses a labelled manual substitute. See LFO.md. |
| Hardware equivalence | Exact OP-1 synth/effect sound, curves, screens and every gesture remain unverified. See PARITY.md. |

Nine native suites include a dedicated production ownership suite; nine MCP tests cover protocol and live control. Core and live-ownership sanitizer gates are part of publication. Legacy direct-engine plugin fixtures use explicit offline mode and are not proof of production concurrency.
