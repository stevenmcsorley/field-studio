# MIDI routing and clock

Open **COM → MIDI routing & clock**. Standalone input/output ports are selected in **Options → Audio/MIDI Settings**; a plugin uses its host's MIDI routing. A physical USB/DIN adapter or OS-exposed BLE port is a host device, not emulated OP-1 hardware.

## Routing

- Choose an omni or single-channel input and an output channel1–16. Typing/clicking the app keyboard uses the chosen input channel (channel1 in omni).
- Receive filters separately control notes/sustain, other channel messages and transport/clock. Changing input channel or disabling notes releases held voices. Sustain is independent per channel; releasing one channel does not terminate the same pitch held by another.
- MIDI thru is off by default. Enable it to forward accepted short messages with their sample positions and remap channel messages to the chosen output. Notes/sustain and other outgoing messages have separate filters. Clock messages are not echoed.
- Controller output sends notes and four engine macro controls on CC46–49. Absolute mode sends7-bit positions; relative mode uses two's-complement deltas (1–63 positive,65–127 negative), with the initial knob position as its baseline. This encoding is an explicit software convention, not verified against hardware.
- Sequencer-generated gated notes and releases are emitted with sample offsets. Manual incoming notes are not duplicated by this sequencer path. Overflow emits an all-notes-off message. Thru is bounded to2048 short events per block, with an all-notes-off on overflow; SysEx thru is unsupported.
- **MIDI monitor / settings** shows event count, last message bytes, effective tempo, output channel and SPP origin. It is a point-in-time dialog.

## Clock and transport

**Internal** uses the project tempo. **Receive MIDI clock** estimates tempo from24 pulses per quarter note, with a robust interval window and jitter/outlier handling. After two seconds without valid clocks, the estimate expires and playback falls back to the project tempo; tape does not automatically stop. DAW tempo/transport takes priority when host follow is enabled. The implementation changes tempo-driven sequencing/LFO timing; it does not reproduce the hardware's tape-speed correction law.

**Send MIDI clock** emits24 PPQN while playing, with Start/Stop/Continue and SPP when starting away from zero or seeking. Sending follows tempo rather than tape varispeed, brake or loop wraps. Clock output can follow a DAW's tempo/transport. Incoming clocks are never copied back out. Set/reset the outgoing SPP origin using the playhead commands. That origin is a software time offset in seconds.

Incoming Start rewinds tape and sequencer and starts playback. Continue resumes tape; Stop stops it and cancels tape performance actions. SPP positions the deterministic sequencer in sixteenth notes without seeking tape, matching the documented destination. Repositioning does not trigger a note immediately; the next natural sixteenth boundary starts the requested step.

## Controller coverage

The public [OP-1 field MIDI reference](https://teenage.engineering/guides/op-1) establishes the CC numbers, message destinations and trigger thresholds. Software parameter curves and hardware routing equivalence remain unverified.

| Input | Software behaviour |
|---|---|
| Notes, bend, CC64 | Sound, configured bend range, per-channel sustain |
| CC1–4 | Four independent MIDI-LFO inputs |
| CC7/9/10 on channels1–4 | Tape-track level/mute/pan |
| CC46–49 | Source controls; sampler-drum tuning/in/out/mode; DBox uses its modeled source controls |
| CC50–53 | Synth ADSR or DBox transient envelope controls |
| CC54–57 | Patch effect macros |
| CC58–61 | MIDI-LFO route depths; other LFOs use rate/depth/target plus waveform or manual Element input |
| CC62/63, value≥64 | Deferred patch randomization/reset (original software policy, current engine/audio retained) |
| CC70–73 | Master effect macros |
| CC74–77 | Master left/right level, drive amount and drive release |
| CC78/79/80/81 | Record level, octave down/up, documented piecewise tempo map, click level |
| CC82–88, value≥64 | Tape bar navigation, start/end, loop-in/out/toggle |
| CC90–93 | Master EQ low/mid/high and synth/drum mode |
| Program0–15 / CC102 on channels1–8 | Deferred synth/drum slot recall |
| CC104/105, value≥64 | Tape stop/play |
| CC123 / all-sound-off | Release notes owned by that channel |

A patch reset restores modeled source/envelope/FX/LFO/play-setting defaults, preserving the engine, loaded audio and sample regions. Randomization affects those same controls. Slot recall and patch actions run on the message thread; they are not sample-accurate. The four compressor destinations follow the guide's master-out control order; the DSP remains original.

Not established: exact hardware controller curves, relative encoding, filter defaults, PLL/timeout policy, SPP-origin semantics or complete per-device menus. Audio-pulse PO/1/16 sync, BLE advertising/USB-device identity, aftertouch/MPE, NRPN and manufacturer SysEx are not implemented. These features and the shared-lock dropout problem prevent a full hardware-MIDI or live-performance parity claim.
