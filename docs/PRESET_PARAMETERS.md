# Preset control-position translation

`PresetParameters` is a control-thread metadata translator. It uses independently encoded format facts from the local, pinned [Field raw-control research](https://github.com/jstnfst/te-op1/tree/24ac03e2de2f37c43272417d732ffd1f4e69636d), particularly `public/knob-model.js`, `public/mapping-data.js`, and hardware-exported metadata fixtures. It contains no copied community implementation or recovered manufacturer DSP.

**A matching normalized control position does not establish a matching sound, parameter-response curve, filter topology or effect algorithm.** The caller receives `exactSound=false`, an explicit source profile and separate mapped/unsupported reports. The profile is observed OP-1 field travel; it is not a universal original-OP-1 or future-firmware calibration.

## Applied values

- Supported melodic engines translate their first four raw knobs into `macro1..4`. Restricted/signed ranges are honored, including String's different travel for each control, Cluster count/spread/unitor, FM topology travel, Digital signed modulation, DNA signed filter, Pulse bipolar modulation and Dr Wave limits. Unspecified known controls use the profile's documented 0..32767 travel. Values outside observed travel are clamped and reported; source integers remain unchanged.
- Sampler markers are deliberately excluded from generic synth macros. Existing `PresetMapping` owns their time/root conversion. DBox per-pad arrays are also excluded rather than being misrepresented as a global four-knob patch.
- Source effect names select the software effect enum. `fx_active=false` bypasses the effect. Active, known effects map four raw controls to `fx1..4`, using effect-specific limits and bipolar ranges. Fazer has no corroborated raw-travel profile here, so only its type is selected and its raw parameters remain unapplied. Unknown active effects are bypassed with a report.
- For effects whose source fourth control is explicitly MIX (Grid, Mother, Spring, Terminal), its normalized position additionally initializes `effectMix`. This is a control-position adaptation, not a measured wet/dry gain law. Other effect blends remain at the software default with an explicit limitation.
- Known LFO names select the corresponding software type, but modulation remains neutral (`lfoDepth=0`) until rate/depth/routing conversion is calibrated. Source enable/type facts are separately available to later stages.
- ADSR sustain is imported as a dimensionless control position. Play mode and bend range use the nearest captured selector values; nonexact matches report that decision boundaries remain unverified. Hardware attack/decay/release times, portamento time and patch-volume gain law are not invented from raw percentages.

Source JSON is read-only. Invalid present control arrays or enable flags fail atomically, leaving the caller's previous translation untouched. Raw knob arrays must contain four or eight finite signed16-range integers; unsupported semantic fields remain attached to the original metadata instead of being discarded.

## API

`translateParameters(metadata, result, error)` returns a `ParameterTranslation`. `parameterValues` contains target IDs and physical software values (normalized floats for macro/FX controls, enum indices for type/mode, semitones for bend range). `controls` retains group/index/raw/range/position/target/method provenance. `mapped` and `unsupported` explain the adaptation. Source effect/LFO selections and active flags are separate facts, not proof that their complete hardware behaviour was reproduced.

## Validation

The new standalone test source is `tests/preset_parameter_tests.cpp`. It passes typed/signed/restricted ranges, bypass handling, unknown FX, neutral unsupported LFOs, sampler/DBox isolation, captured play/bend selectors, beyond-travel reporting, source preservation and atomic malformed-input rejection. The independently parsed inventory also passes: **167 records, 15 engine types, 8 represented source FX names, 6 LFO names and 1,576 translated control positions**. The numbers describe metadata coverage, not rendered audio equivalence. No reference sound is derived from those fixtures; most share a dummy preview payload.

A standalone local check was linked against the already-built JUCE core objects, without changing the app's CMake or processor sources. Root integration must add `Source/PresetParameters.cpp`, the test target and the application step, then rebuild the final instrument.
