# Field Studio 0.4.2 — Recording LCD and tape motion

- Recorder graphics now follow the selected style: Studio/Vintage reels, Porta cassette and Disc Mini cartridge. Studio/Porta have direct public layout references; Vintage/Disc Mini remain original approximations.
- Winding follows signed audio-engine travel, retaining continuity across loop/chop wraps and reflecting MIDI/remote reverse. Stop, brake, zero speed and count-in freeze the mechanism; scrub/rewind update it.
- Detail view scrolls beneath a fixed centre head. Wheel over the LCD to zoom; double-click switches between detail and six-minute overview. Pointer seeking/selection share the painted time origin.
- Splice intervals under the head highlight blue; red is confined to the actual current recording stroke. The ruler follows effective tempo, and timecode uses original monoline vectors.
- README and tape documentation include native animated previews, screenshots and primary-source evidence with explicit remaining parity gaps. The focused tape-motion check passed locally; broad local suites were not rerun. Release publication remains gated on the existing multiplatform CI checks.

Recording button semantics, MIDI IDs and sound algorithms are retained. Full hardware takes/silent gaps, arming gestures, exact reel physics and pixel/frame parity remain open. Portable unsigned Linux x86_64, Windows x64 and macOS universal standalone/VST3 downloads, plus macOS AU, publish after CI passes.

---

# Field Studio 0.4.1 — MIDI recording and timeline controls

Adds MIDI learn for tape recording/finish, track selection, sample capture, tape edits and undo/redo, one-second/bar navigation, playhead loop markers, momentary brake/reverse/roll and metronome enable. Fine jog supports relative encoders at approximately 15.75 ms per tick. Existing parameter IDs and controller assignments remain compatible.

Right-click panel transport/loop buttons to learn; all destinations are listed under COM → MIDI learn / controller mappings. Configure action buttons for absolute CC 127 on press and 0 on release. New action commands trigger once per press and bypass pickup. Recording/edit preparation stays off the audio callback, uses a bounded command queue and respects Stop/Panic/project-change fences. Busy actions report a retry message. Capture finish remains separate from tape Stop. See docs/CONTROLLERS.md for detailed setup and limits.

Includes the 0.4.0 LFO work below plus a production callback-to-LCD animation regression test and clearer idle-animation documentation. Portable unsigned Linux x86_64, Windows x64 and macOS universal standalone/VST3 packages, with AU on macOS, are published only after the release workflow passes.

---

# Field Studio 0.4.0 — LFO reference update

Six dedicated routing LCDs and corrected LFO controls for synth and drum modes. Includes standalone/VST3 for Linux x86_64, Windows x64 and macOS universal, plus macOS AU, complete corresponding source and checksums. Downloads remain unsigned.

- T4 bypass/enable, Shift+T4 type selection, grouped destinations and parameter selection, supported base-value editing from LCD knobs.
- Value oscillates with square/ramp/saw/sine shapes instead of depending on the synth orange macro.
- Random routes to an individual parameter or the whole selected module and gains a signed attack/decay envelope.
- Tremolo has independent signed pitch/volume amounts, pitch envelope and shifted shape selection.
- Velocity has independent target amount and volume response, including DBox.
- MIDI has four parameter/destination selectors, signed route gain/off and no modulation from unreceived controllers.
- Element supports selected audio, voice envelope, main output and an explicitly labelled manual G substitute.
- Older projects/presets retain their previous LFO model. Existing host parameter IDs/order and normalized waveform travel are retained; new controls are appended and support automation, MIDI learn and MCP.

The callback ownership changes and empty-audio decode fix from 0.3.1 are retained. Synth/effect algorithms outside the modulation changes are unchanged. See [LFO controls and compatibility](LFO.md) for screenshots, usage and limits.

Release publication requires all three platform builds/tests, MCP checks, core/live ownership ASan/UBSan and verification of all OS/source packages. The maintainer requested no CI monitoring; publication runs automatically after the gates succeed.

Exact OP-1 hardware sound, pixels, modulation curves and timing remain unverified. No physical G-force sensor is emulated. Real-device long-session listening, broader DAW coverage, signing and installers remain open; see [known issues](KNOWN_ISSUES.md).
