# Example 16-control layouts

These are Field Studio mapping profiles, not manufacturer hardware presets. Load one with **COM → MIDI learn / controller mappings → Load controller profile**. Configure the controller itself to send **CC20–35 on MIDI channel 16**, with either absolute values or two's-complement relative values matching the selected profile. Alternatively, learn your existing hardware setup directly; no profile is required.

CC20–23: synth macros 1–4. CC24–27: ADSR. CC28–31: patch FX macros 1–4. CC32–34: master EQ low/mid/high. CC35: master volume. All use 7-bit CC; reverse direction using MIDI learn if required by your encoder mode. These mappings do not configure a BeatStep/Faderfox device or its LEDs, and they do not follow the currently displayed module page.
