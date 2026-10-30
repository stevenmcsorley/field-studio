# OP–1 field software reconstruction specification

Research baseline: 2026-10-30. Target hardware OS 1.7.3+1; current official manual is 1.7.0. This is a factual functional inventory and implementation acceptance plan, not a claim that any feature is already implemented or that DSP is identical.

## Sources and evidence boundary

- [G] Official guide: https://teenage.engineering/guides/op-1
- [P] Official PDF, 1.7.0 REV10: https://assets.teenage.engineering/_img/69f248938d433104c4dd1846_original.pdf
- [D] Official firmware history: https://teenage.engineering/downloads/op-1/field
- [V] Official product visuals: https://teenage.engineering/products/op-1

Local reference PDF: `op1-field-guide-1.7.0.pdf`; provenance SHA256 in `guide-provenance.json`. PDF page numbers below are one-based file pages. Text extraction: `guide-1.7.0.txt`. Reference images belong to their publisher and are analysis references, not application assets. Public demos have not yet supplied controlled comparisons. No parameter ranges, oscillator equations, voice allocation, modulation curves, filter models, effect algorithms or exact rendered animations have been verified by hardware measurement.

## Acceptance categories

- **Functional reconstruction:** documented controls and observable outcomes can be implemented independently.
- **Measured parity:** requires reproducible screenshots/audio experiments; ordinary musical demos alone cannot establish this.
- **Hardware equivalent:** host audio/MIDI/files substitute for embedded hardware.
- **Unresolved:** exact DSP/asset/format details still need evidence.

Every named engine must have distinct working sound generation and its documented controls. A dropdown of names driving the same generic oscillator is not engine parity. Every sequencer must produce its own documented note behaviour; an animation alone is not implementation. Effects must process audio. Screen and keyboard interactions must change real audio/state.

## Physical interaction and mode state

Reference [P] pp3–11 and [V]. Three strips of controls, a wide black screen, silver low-profile body, rounded white keys with black circular accidentals, four rotary/click encoders in blue/ochre/gray/orange. Physical volume is separate. The colour of a parameter indicates its encoder; Shift adds a secondary action/fine adjustment. Encoder click can confirm, reset, or zoom according to context. Context help and a navigable built-in guide exist.

| Context | T1 | T2 | T3 | T4 |
|---|---|---|---|---|
| Synth | Engine | ADSR / play settings | Patch effect | Patch LFO |
| Drum | Drum engine | Transient envelope | Patch effect | Patch LFO |
| Tape | Track1 | Track2 | Track3 | Track4 |
| Mixer | Levels/pans | 3-band EQ | Master effect | Output/drive |
| COM | System | MIDI controller | Devices | MTP/disk |

Modes Synth/Drum/Tape/Mixer are separate; transport remains available across modes. Keyboard sound continues through the selected instrument while tape is accessed. Eight sound slots are independent for synth and drum. Slot press recalls all four patch modules; Shift+slot opens the preset browser. Browser blue selects engine, ochre selects patch. Shift+T1 changes only the engine. Shift+T3/T4 selects effect/LFO. Repeated T3/T4 toggles effect/LFO. Slot hold for two seconds stores a snapshot. Shift+Drop randomizes the patch; Shift+Lift reverts it. User folders, patch rename/copy/move/delete and snapshots are part of the browser [P pp9–11].

## Instrument engines and encoder maps

[P pp12–22]. Values listed left-to-right correspond to blue, ochre, gray, orange. Names and macro roles are documented; exact sound mathematics are unresolved.

| Engine | Primary macros | Additional controls / requirement |
|---|---|---|
| Amp | Volume; compression; tone; overdrive | External input processing; T2 tuner |
| Cluster | Wave count; wave envelope; spread; unitor | Multi-oscillator character |
| Digital | Waveshaper; octave; detune/ring modulation; digitalness | Popup numeric values |
| Dimension | Waveform; modulation; filter cutoff; resonance | Distinct stereo engine |
| DNA | Filter; wave count; wave modifier; noise | Distinct sound generation |
| Dr Wave | Wave type/length; filter; phase; chorus | Popup values |
| DSynth | Frequency; waveform; envelope; cross modulation | Shift exposes second oscillator waveform/envelope and filter cutoff; blue click envelope crossfade |
| FM | FM amount; frequency; topology; detune | Multiple topologies, not merely an FM depth knob |
| Phase | Phase shift; distortion; phase filter; phase tilt | Phase-shaping behaviour |
| Pulse | Filter; amplitude; second pulse; modulation | Dual pulse behaviour |
| Sampler | Start; loop-in; loop-out; end | Encoder click zoom per boundary; Shift direction/fine tune/loop fade/gain |
| String | Tension; decay; detune; impulse | Excited-string model |
| Voltage | Modulation; ground noise; phase filter; detune | Distinct oscillator character |
| Vocoder | Waveform; formant; bands; mix | Selected input modulates; output-to-input excluded |

Synth envelope is ADSR. Shift on envelope accesses play mode, portamento, bend range, patch volume. Mono/poly/legato choices and exact limits require validation rather than assumptions.

Chromatic stereo sampler: up to 20 seconds [V]; adjustable start, sustain loop and end; release leaves the sustain loop and continues toward sample end. Import files or capture selected input. Threshold and gain govern recording. Dual-layer A/B versus stereo handling is documented in [V] and requires representation in the sample state, not destructive flattening.

Drum [P pp35–38]: two engines, sample kit and DBox. Each keyboard key has an independent sound assignment. Sample kit slices one recording via independent tuning, in/out and play mode. Shift offers reverse, pan, fade/gain, stereo versus A+B stacking, attack/release. Zoom around in/out. Choke/stack behaviour and playback modes need explicit tests. DBox is a dual-oscillator drum synth **per key**, primary pitch/wave/envelope/cross modulation and Shift second oscillator pitch/wave/envelope/filter cutoff. Drum envelope is attack/gain/release/timing (transient shaping), not synth ADSR.

## Effect inventory

[P pp23–28]. One patch insert, plus a separately stored master insert. Patch insert records to tape; master insert affects monitoring and mixdown but is not printed into tape tracks.

| Effect | Blue | Ochre | Gray | Orange |
|---|---|---|---|---|
| CWO | Frequency | Delay | Feedback | Sideband |
| Delay | Range | Speed | Feedback | Level |
| Fazer | Frequency | Feedback | Speed | Depth |
| Grid | X size | Y size | Z feedback | Mix |
| Mother | Distance | Gate | Color | Mix |
| Nitro | Frequency | Filter follow | Feedback | Frequency |
| Phone | Tone | GSM | Baud | Telemetry |
| Punch | Frequency | Punch | Rounds | Power |
| Spring | Tone | Turns | Damping | Mix |
| Terminal | Rate | Bits | Model | Mix |

There are ten named effects. The online contents omit Fazer from its indexed list but the actual manual includes it. Nitro's two frequency-labelled controls must be distinguished by the screen/reference behaviour; do not invent a documented name for the fourth control. Exact curves, tap counts, saturation, frequency shifting and reverb response are unresolved.

## Modulation

[P pp29–34]. Six types with per-patch enable state and target addressing into engine/envelope/effect/main.

| LFO | Main controls and input |
|---|---|
| Random | Speed, amount, destination, envelope/parameter |
| Element | Source, amount, destination, parameter; source can be motion, selected external input, envelope, or main sum |
| MIDI | Four incoming CC1–4 target mappings; Shift changes destinations |
| Tremolo | Rate, pitch depth, volume depth, envelope; Shift fine rate and waveform; sine/saw/exp/square/blip |
| Value | Rate, amount, destination, parameter; click waveform; square/ramp/saw/sine; retrigger/free timing |
| Velocity | Destination depth, volume depth, destination, parameter |

Motion source needs host substitute (manual XY or available sensor), explicitly labelled. Input envelope followers must follow actual selected audio.

## Tape and editing

[P pp39–42]. Eight tapes; each has four stereo tracks, six minutes at normal speed, 32-bit recording; per-tape name, style, tempo/sync and mixer state. Four styles: Studio 4-track, Vintage 4-track, Porta 4-track, Disc Mini. Style must affect audible processing and visual recorder, not just a label. Exact characteristics remain unresolved.

Tape primary encoders: scrub / loop-out / tape speed / recording level. Shift: slide take / loop-in / speed percent / recording pan. Record, play, stop, rewind/forward and armed recording/count-in. Recording overdubs existing audio. Recording source can be live instrument or selected input. Playback speed affects recorded pitch/time. Reverse playback and live direction changes need crossfade protection.

Editing: split take, lift to clipboard, drop, Shift+lift multiple tracks within loop, Shift+drop merge clipboard to one track using clipboard mixer balance, Shift+split join adjacent takes. Grid from bar to eighth-note steps. Jump by grid markers, move takes, rename/clone/load/new/delete tape. Tape-button+left/right undo/redo since 1.7.0. Saving/restoring must include audio, takes, clipboard metadata, speed, loops and mix state; no real-time disk write on the audio callback.

Eight tape tricks map slots 1–8 to loop-in, loop-out, loop toggle, break, reverse, chop, memo1, memo2. Break/reverse/chop are performance controls; memo states recall parameters and must be testable. Do not implement destructive deletion when a momentary effect is expected.

## Mixer and final output

[P pp43–46,55]. Per-track level/pan/mute. Shift+track toggles mute; Shift+encoder edits pan; click reset/center where appropriate. EQ has low/mid/high plus amount with per-band/global toggles. Master effect is independent of the patch effect. Output has separate L/R gains, stereo link, drive and release. Real peak meters. Sound path: instrument -> patch effect -> recording/monitor; tape tracks -> per-track mix -> sum -> EQ -> master effect -> output dynamics/gains. Avoid feeding master processing back into tape unless selected resampling requires it.

Mixdown records live final output to either A or B, six minutes each; replay/stop/select-side/record on T1–4. Export individual tape stems separately from mastered stereo mixdown. Render should capture performed tape tricks and parameter changes.

## Sequencers

[P pp47–51]. Separate synth/drum sequencer memory, selected type and enabled state; only one plays at once. They store note events, not audio. Must support patch changes during the same sequence.

| Type | Controls / distinctive behaviour to implement |
|---|---|
| Arpeggio | Held note traversal; note division, trigger mode, trigger pattern, hold; Shift note length/type/pause-skip/swing |
| Endless | Enter note stream; division, swing, trigger pattern, hold; Shift manual mode/rotate pattern/direction; up to999 steps per firmware history |
| Finger | Cursor, swing, pattern length, hold, erase, play mode; drum grid variation |
| Hold | Break point, mono/poly, transpose, hold/clear; sustain and transform input |
| Pattern | Cursor, swing, pattern length, hold; Shift erase/offset/move section/play mode |
| Sketch | Draw X/Y note path; move X/Y; Shift erase/divider/grid/hold |
| Tombola | Notes as bouncing objects triggering events; rotation speed/heaviness/shape/bounce; manual mode |

Exact clock subdivisions, trigger patterns, storage dimensions and physics constants need additional reference. Generic 16-step playback for every type does not satisfy this inventory.

## Tempo, I/O and settings

[P pp52–62; G MIDI reference]. Internal/free, beat-match master clock, external MIDI clock, PO sync and sixteenth-note pulse. Tap tempo, tempo nudge, metronome sound/level and count-in. In plugin, DAW transport/BPM must have explicit sync mode and not silently fight local tape controls. Standalone uses selected audio device sample rate/buffer and MIDI ports.

Input: host mic/line/audio input, imported recording, output resample. FM RX/TX and OB-4 frequency sync require RF hardware; do not show a fake working radio. Host devices substitute USB audio host/device. Original 2/8/10-channel modes correspond to stereo, four stereo tape stems, or main+four stems; plugin bus layout can expose equivalent outputs. BLE MIDI availability is OS/adapter-dependent. COM MIDI controller mode should send MIDI, not just switch screens. System stores velocity/tuning, MIDI filters/channel/SPP origin, clock, monitor; hardware charge/power/radio settings are inapplicable unless replaced explicitly.

## MIDI compatibility map

[G, incoming MIDI table]. Note on/off, pitch bend, sustain, program change0–7 synth slots and8–15 drum slots; transport clock/start/continue/stop/SPP. MIDI CC:

| CC | Function |
|---|---|
|7 /9 /10|Track volume/mute/pan on channels1–4|
|46–49|Four engine macros, or active drum key tuning/in/out/play mode|
|50–53|Synth ADSR, or drum attack/gain/release/smooth|
|54–57 /58–61|Patch effect /LFO macros|
|62 /63|Randomize/reset patch, trigger>=64|
|64|Sustain|
|70–73 /74–77|Master effect /compressor macros|
|78 /79|Record level /octave down-up|
|80 /81|Tempo /metronome level|
|82–88|Previous bar,next bar,start,end,loop-in,loop-out,loop toggle|
|90–92 /93|EQ low/mid/high /synth-drum mode|
|102|Slot selection by channels1–8|
|104 /105 /123|Tape stop/play/all-notes-off|

CC80 mapping:0–5 ->40–50 BPM in steps of2;6–120 ->52–166 in steps of1;121–127 ->168–180 in steps of2. Controls marked trigger in [G] fire at>=64. Smooth continuous CC changes; handle all-notes-off and stopped transport without hanging voices.

## Visual evidence and review plan

`reference/product-front.png` is a 2048px official image whose source is portrait/rotated; `product-angle.png` is another product view. Compare proportions against these, not the original OP-1. Encoder colors differ from the original hardware. `reference/sources.json` records source links. Local PDF page renders: physical-overview(p4), sampler(p17), synth-sampler(p20), envelope(p22), drum-mode(p35), tape-controls(p39), tape-edit(p40), tape-styles(p42), mixer(p43), arpeggio-endless(p48), finger(p49), hold-pattern(p50), sketch-tombola(p51).

Draw original code-native controls and graphics. Review all 14 synth engine screens, both drum engines, six modulators, ten effects, seven sequencers, four tape styles, preset/input/output/COM/settings browsers. Require matching control grouping, readable labels, color correspondence, animation driven by parameters/audio, correct click/hold/Shift behaviour, and resizable layout. A screenshot of one tape screen does not establish global visual parity.

## Verification gates

1. Standalone and VST3 build from same audio/state engine on each supported platform; plugin scan and instantiation in real hosts.
2. Functional tests for note timing, sustain, velocity, pitch bend, automation, sequencer timing and MIDI mapping.
3. Tape tests: overdub, split/lift/drop/merge/join/undo, stereo, speed/reverse/loops, crossfades, independent stems, persistent reload.
4. Sample tests: import/capture/trim/sustain loop/reverse/stack/choke, boundary safety and session reload.
5. Persistence includes samples and tape; plugin state load must not depend on an absent development folder.
6. Stress audio callback: no allocation/locks/disk access; finite output, bounded feedback, sample-rate changes, variable host block sizes, all-notes-off, multiple instances.
7. Screenshot matrix at multiple UI scales and all modes. Until exact visual/algorithm comparisons are available, label the result a reconstruction with documented deviations.
