# LFO controls and compatibility

Version 0.4.0 adds six dedicated routing screens and reference-style controls for synth and drum modes. The supplied field guide images and the [official OP-1 field guide](https://teenage.engineering/guides/op-1) define the control roles. See also the [manufacturer's field guide PDF](https://assets.teenage.engineering/_img/69f248938d433104c4dd1846_original.pdf). Original OP-1 documentation has a different Random encoder order; this implementation follows the supplied **field** references.

## Opening and editing

Press **T4** to open modulation. Press T4 again to bypass/enable it without losing settings. **Shift+T4** opens the six-type LCD browser: turn blue or use Up/Down to select, then T4/Enter to load. Escape exits without changing the sound. COM also provides type, bypass, compatibility, sync, key retrigger, waveform and source settings.

| Type | Blue | Ochre | Grey | Orange | Additional controls |
|---|---|---|---|---|---|
| Random | Speed | Amount | Destination | Attack/decay envelope | Shift-orange selects a parameter; click its LCD knob to edit a supported base value. |
| Element | Source | Amount | Destination | Parameter | Shift-blue adjusts the manual G substitute. |
| MIDI | CC1 parameter | CC2 parameter | CC3 parameter | CC4 parameter | Hold Shift to choose each route's destination module. COM → MIDI LFO route amounts supplies signed route gain/off. |
| Tremolo | Speed | Signed pitch amount | Signed volume amount | Pitch envelope | Shift-blue gives fine speed adjustment; Shift-orange selects sine/saw/exp/square/blip. |
| Value | Speed | Amount | Destination | Parameter | Click the clock to cycle square/ramp/saw/sine. Shift-blue gives fine speed adjustment. |
| Velocity | Destination amount | Volume response | Destination | Parameter | Volume response zero removes ordinary velocity-to-volume scaling while keeping target modulation available. |

Speed uses Hz when free; with **COM → LFO tempo sync**, the speed knob selects beats per cycle. Destination choices pair each module with a free-running `F` variant; the ordinary choice resets phase on a note, while F keeps running. MIDI routes do not have an oscillator clock. Grey selects a module and orange selects a parameter within it. The flat target IDs remain available for automation, learned controllers and MCP.

Click a parameter knob in the LCD to switch its coloured encoder between selecting the parameter and editing its base value. Base editing is available for synth macros, ADSR, effect macros/mix, patch gain, tape speed and main volume. Software pitch/pan/filter modulation destinations without an exposed persistent base control report that limitation instead. Changing modules is an intentional sound edit.

## Signal behaviour

- **Random:** sample-and-hold values feed all available routes in the selected module. A signed note-triggered attack/decay envelope shapes their amount. Software destination groups include extra gain/pan/filter controls in addition to the four synth macros.
- **Element:** follows the currently selected host/external input envelope, the loudest active voice envelope, previous main-output level, or a manual value. `G*` is explicitly a manual substitute, adjustable by Shift-blue, automation, MIDI learn or MCP. No physical accelerometer or radio is emulated.
- **MIDI:** CC1–4 feed four independent signed routes. Unreceived controllers contribute nothing in reference mode; zero route gain disables that route. Routes targeting the same parameter add with bounded saturation.
- **Tremolo:** independent signed pitch and source-volume modulation. Its reference pitch range reaches ±24 semitones; the signed envelope shapes the pitch path. These are software scaling/envelope laws, not measured hardware calibration.
- **Value:** a running square/ramp/saw/sine oscillator, independent of the synth's orange macro. It can reset on notes or run continuously and can follow tempo.
- **Velocity:** target modulation uses each voice's note velocity. Its independent volume response interpolates between constant amplitude and ordinary note-velocity scaling, including DBox voices.

LCD clock motion uses the engine's published phase. Shape, amount, envelope, destinations and selected parameter colours reflect stored controls; the diagrams illustrate routing rather than an oscilloscope measurement.

## Existing sounds and automation

New projects default to reference behaviour. Projects and sound presets without the new `lfoModel` field load the previous modulation model. COM → Reference LFO behaviour explicitly upgrades them. Existing factory Slow orbit variations retain their earlier model to preserve their established modulation. Saving stores the choice, bypass, envelope and independent amounts.

Existing parameter IDs, ordering and waveform-choice normalization remain unchanged. The legacy triangle slot serves as ramp only for reference Value; it remains triangle in legacy mode. New parameters are appended: `lfoModel`, `lfoEnabled`, `lfoEnvelope`, `lfoPitchAmount`, `lfoVolumeAmount`, `lfoVelocityVolume`. All are available to host automation, MIDI learn and MCP parameter control.

Tests cover moving Value output, signed independent Tremolo routes, Random group isolation/envelope, Velocity volume response, Element input, legacy restoration, host parameter order, encoder roles, bypass and callback allocation/free checks. The drawings are original vector reconstructions. Exact pixels, random statistics, hardware curves/timing and all device gestures have not been measured against hardware.
