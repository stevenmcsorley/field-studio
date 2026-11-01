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
