# Field Studio 0.2.2 beta 8

Reliability update to the portable multi-platform beta of the independent synthesizer, sampler and four-track stereo tape instrument. Exact OP-1 field sound, feature and visual parity is not established.

## Licensing

Original Field Studio code/graphics: **MIT**. Combined app/plugin binaries with JUCE: **AGPLv3**, with complete corresponding source provided at this release. Free to use, including commercially; redistribution of the combined build must comply with JUCE/AGPLv3 terms. See LICENSING.md in the downloads.

## Downloads

- **Windows x64:** standalone + VST3.
- **macOS universal (Apple Silicon and Intel, macOS 11+):** standalone + VST3 + AU. Both architectures are checked with lipo; tests execute on the CI runner's native architecture.
- **Linux x86_64:** standalone + VST3, built on Ubuntu 22.04 (glibc 2.35+).
- **Source:** project and pinned JUCE dependency together, including build scripts and dependency licence files.

Each download has a SHA-256 sidecar. Binary bundles include installation instructions, dependency notices and a per-file hash manifest. These are portable archives, not signed installers. Windows binaries are unsigned; macOS binaries are not Developer ID signed/notarized. The project owner reports working plugin operation, automation, Faderfox assignment and BeatStep use; DAW/OS/model details were not recorded. Broader device/host coverage remains open.

## Added in beta 8

- Grid LCD rebuilt around the supplied isometric floor/wall projection, fixed blue wire frame, ochre size box, white origin cube, directional arrows, Z marker and 12-segment mix strip. Thin stroke numerals replace filled-font readouts.
- Mother LCD rebuilt with finite rear-wall perspective grid, foreground/rear figures, gate bracket, colour rays and matching readout placement. Insert and master effects use the same renderer with independent values.
- Dimension's four-ellipse preview replaced by a paired 2:3 Lissajous knot. Corrected plot position, palette, waveform selection, axes and rounded resonance silhouette.
- Dedicated DNA LCD with linked hexadecimal tiles, CPU ID caption, filter label, terminal nodes and noise strip. Tile variations follow control settings; the caption is not a CPU serial readout.
- Updated native screenshots, LCD-only comparison views and rendering notes. Audio processing and existing parameter IDs are unchanged.

This improves fidelity to the supplied still references; exact font rasterization, hardware frequency laws, geometry at every control value and animation timing are not established. Dimension shows this engine's actual cutoff rather than copying the guide's 440–1440 example as a live measurement. See docs/LCD_FIDELITY.md.

## Retained from beta 7

- Original full-LCD Vocoder head/voice illustration with real input activity, analysis envelopes and source label. Waveform now selects eight carriers, Formant remaps envelopes, Bands selects 4–16 bands and Mix blends dry/wet independently. Existing Vocoder patches can sound different after these corrections.
- Sampling input page with source header, Standby/Recording states, real meter, threshold, input gain and 6/20/60/120-second maximums (20-second default). Record/Stop and held-key capture are supported; empty armed captures retain the previous sample.
- The orange microphone button now works as an input/sampling control. It opens the page, then the source picker on a second click. Shift-record samples the selected source; main-output resampling and audio-file import remain available.
- Optional external stereo capture using pinned miniaudio 0.11.23 under MIT: Windows WASAPI system loopback, Linux PulseAudio/PipeWire monitor inputs and macOS CoreAudio inputs including routed virtual devices. macOS automatic process/system capture is not implemented; virtual routing is required for other apps.
- New settings support saved state, automation, MIDI learn and MCP parameter editing. Device connections are transient and explicitly selected, with direct monitoring disabled on selection. Added native screenshots, routing instructions, license/source notices and package gates.
- Tests cover vocoder control independence, bounded audio, dry/wet gating, capture threshold/gain, held-key stop, empty capture, saved settings and bounded FIFO behaviour.

Physical input/loopback listening tests, exact hardware sound/gestures and seamless live capture remain open. Separate capture-device clocks and FIFO underflow/overflow can affect recordings; the shared engine lock remains a limitation. See docs/VOCODER_SAMPLING.md.

## Retained from beta 6

- Full-LCD tempo page with Song/Play BPM, tape ratio and nominal cm/s / IPS, animated metronome, level strip and clock/link indicators. Four new native screenshots and a dedicated setup guide.
- Blue/Shift-blue coarse/fine adjustment, averaged tap tempo, 1 / 0.01 BPM arrow nudges and three original metronome sounds. Playback metronome timing now stays independent of tape speed and reverse.
- Optional tape-tempo link: Beat Match can drive the engine/master MIDI clock from tape rate; MIDI receive/DAW follow can scale tape rate against the saved song BPM. Existing projects default unlinked.
- Generic MIDI receive/send/relay for compatible devices, replacing a PO-specific control concept. Relay preserves received clock/transport sample offsets and suppresses duplicates with MIDI thru. DAW follow takes priority over incoming realtime transport.
- New settings persist in projects/host state and support automation, MIDI learn and MCP parameter control. Existing MIDI clock choices/IDs are unchanged.
- Tests cover tap/fine/external ownership, linking/unlinking, relay clock and transport timestamps, duplicate suppression, sound choices, metronome speed independence, state and UI targets.

This is standard 24-PPQN MIDI sync, not PO/analogue 1/16 audio pulse output. Exact hardware tap curves, sound/animation/phase behaviour, physical-device clock accuracy and host-specific routing remain unverified. No pitch-preserving time stretch or hardware latency compensation is added. See docs/TEMPO_SYNC.md.

## Retained from beta 5

- Dedicated Arpeggio, Endless, Hold, Pattern and melodic/drum Finger LCDs and mode-specific normal/Shift controls. Six new native screenshots and a sequencer guide accompany this release.
- Up to eight pitches per Pattern/Endless or Finger step, with saved chord and velocity state; divisions, trigger masks, direction, rotation, manual stepping, play modes and Hold split/mono controls. Existing parameter IDs remain stable.
- Optional local MCP integration with two discoverable tools, bounded operation batches and Linux sandboxed JavaScript Code Mode. Portable plans support Windows/macOS/Linux. Enable explicitly from COM; client registration is documented, not automatic.
- Windows ASIO backend enabled using JUCE's bundled headers under their GPLv3 option. macOS retains CoreAudio; plugins use their host's driver. Physical ASIO hardware and latency remain untested.
- Automated native MCP tests, authorization checks, chord/state regression tests and backend factory checks. Complete source, integration, licensing notices and setup guides ship in the archives.

Hardware algorithms, exact sequencer gestures/timing, separate synth/drum sequence memories and full Endless keyboard entry remain open. See docs/SEQUENCERS.md, docs/MCP.md and docs/AUDIO_BACKENDS.md. The shared audio-engine lock remains a live-performance limitation.

## Retained from beta 4

- Full-LCD String, Voltage and Pulse screens with original parameter-driven vector illustrations matching the supplied reference layouts.
- Corrected their four-control roles: String uses ochre for decay, grey for stereo detune and orange for impulse type; Voltage has dedicated modulation/noise/phase-filter/detune behaviour; Pulse separates amplitude, second pulse and modulation from its blue filter.
- Full-LCD melodic Sampler with actual stereo waveform data, coloured markers, loop-fade shading and direction/tuning/fade/gain readouts. Drum sampler retains its existing pad view.
- Sampler Shift exposes direction, ±100-cent fine tune, loop crossfade and 0–2 gain. New parameters are host-automatable, MIDI-learnable and saved in projects, DAW state and sound presets. Existing projects/imports use neutral defaults.
- Boundary-focused zoom is available from marker-knob right-click menus or the four LCD header zones. Choose again for overview; Shift shows the entire stereo sample.
- Tests cover each synth control, zero Pulse amplitude, sampler tuning/gain, both crossfade directions, allocation-free processing, Shift control targets, zoom and host state restoration.

**Sound compatibility:** String, Voltage and Pulse can sound different from beta 3 following the control-role corrections. Sampler fine tune/fade/gain default to 0/0/1, preserving earlier sample playback. Nonzero loop crossfade shortens the repeated loop by its overlap. Hardware algorithms, curves, gestures, animation timing and exact sound remain unverified. See docs/INSTRUMENT_SCREENS.md.

## Retained from beta 3

- Full-LCD Master Out with separate audio-driven left/right meters, peak indicators, level faders and Drive/Release readouts. Shift-blue/ochre sets both output levels together. Existing host parameter IDs are preserved.
- Full-LCD Grid with parameter-driven 3D grid, dimensions, feedback and Mix meter, for insert and master FX.
- Full-LCD Mother with perspective room, original body outlines, distance/gate/colour cues and Mix meter, for insert and master FX.
- Grid now has independent X/Y delay dimensions, Z feedback and orange dry/wet Mix. Mother has a dedicated distance/gated-room/colour path and orange dry/wet Mix. Native imports apply Mix once; UI selection uses full overall blend. Existing saved overall blend values are retained.
- Fixed a shared fractional-delay wrap boundary that could read past the buffer; caught by ASan before release.
- Master Drive now adds makeup gain and saturation with release-dependent compression. Grid host tail reporting now accounts for high-feedback decay.
- README feature gallery, VST3 installation/automation guide and user-confirmed Faderfox/BeatStep results are included.

**Sound compatibility:** Grid, Mother and nonzero Master Drive can sound different from beta 2. Back up projects. Existing saved Grid/Mother overall blends are retained as a software override; choose COM → Effect wet/dry override → 100% for full native Mix travel. DSP equations, parameter laws, meter calibration and precise hardware parity remain unverified. See docs/MASTER_SPATIAL.md.

## Retained from beta 2

- Recreated the full-LCD mixer: four audio-driven track meters, pan indicators, coloured level readouts and mute/solo indication. Shift + T1–T4 toggles track mutes on the mixer page. Click a channel to mute; Shift-click to solo.
- Recreated the EQ LCD with three band faders, a control-response illustration and red Amount fader. Orange now controls EQ Amount; per-band/overall toggles affect audio and persist in projects/DAW state. Shift + orange retains the existing drive extension. Click band columns to toggle, Amount column to bypass, MAX EQ for full amount and CLEAN for zero amount.
- Includes the new tape reel LCD, position-driven motion, loop/time/speed controls and panel keys 4–6 for momentary brake/reverse/chop.

The EQ response illustration and meter scales are original approximations, not measured hardware responses or calibrated VU meters. Beta 2 retained the existing filter crossovers, drive and dynamics algorithms; beta 3 changes Master Drive as described above. See docs/MIXER_EQ.md for controls and limits.

## Retained from beta 1

- Recreated CWO cow LCD, Dimension and DSynth screens, Delay circles, and permanent panel button legends, including secondary icons on sound keys 1–8. The README now shows CWO.
- CWO now blends lower/upper frequency-shifted sidebands, includes both at midpoint, and introduces dry sound as Feedback rises. Its internal mechanisms respond to controls with continuous motion.
- Delay Range moves the selected ring and centre together. Input Level controls new material entering the loop while preserving dry audio and circulating echoes. Slow settings add buffer colour. Held controls no longer produce invented circle wobble.
- CWO/Delay Shift fine-adjusts all four native controls. The optional overall wet/dry override is in COM; new selections and active native imports use full module output. Existing saved blend settings are preserved.
- Added engine/preset browsing, persistent dated snapshots, nested user folders, rename/copy/move/delete and 45 original starter presets. Fixed shifted number-key preset shortcuts.
- Project saving and WAV export run on a bounded cancellable worker. Earlier prepared-media, tape reservation and stale-operation reliability improvements are retained.

CWO/Delay sound can differ from earlier beta versions following the signal-path corrections. Back up projects before upgrading. Public hardware demonstrations informed these changes; exact hardware transfer curves, animation timing and present-firmware parity remain unverified. See docs/research/EFFECT_VISUAL_MOTION.md and docs/PARITY.md.

## Known limitations

The short commit/UI lock still exists: a contended callback can still produce silence. The final command/telemetry migration, asynchronous UI job coordination, total working-memory admission and real device/DAW deadline checks remain unfinished. This is not a seamless live-performance release. Synchronous preparation can still stall the UI thread if initiated there.

The pool cap covers paged tape storage, not all process RAM; separate formats/processes may have separate caps. Retained history/saves count against their pool reservations. Clear unused media and discard history to release them. Back up projects before testing. See docs/KNOWN_ISSUES.md for the precise scope.

Downloads remain unsigned/not notarized: no signing certificates are available. Broader audio-device/DAW/controller coverage and exact hardware equivalence remain unverified beyond the user report above. MIDI learn, reusable profiles and guide-based screen improvements from 0.2.0 are retained.

## Verification

Release publication is gated on Linux, Windows and macOS build/tests, core ASan/UBSan, all package manifests/checksums and complete target downloads. The Linux suite loads the actual VST3 module. Automated success does not substitute for audible device or commercial-DAW testing.
