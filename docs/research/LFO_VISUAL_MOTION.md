# LFO LCD reconstruction and motion evidence

The first 0.4.0 implementation had six routing screens but visible layout/type-weight differences, a static Value waveform thumbnail and decorative motion not demonstrated by the references. This pass corrects those issues. It does not establish absolute hardware equivalence.

## References inspected

- The six guide screenshots supplied by the project owner establish the native 604×292 layouts, widget positions, colours and reference control states. Source-image hashes are recorded in [the evidence ledger](lfo-motion-evidence.json). Those manufacturer guide images are not bundled as app assets.
- [SON WU's LFO hardware tutorial](https://www.youtube.com/watch?v=myC27d3JhVE), uploaded 12 March 2023. The Random trace visibly changes across 1160.0–1161.0 seconds, while the module/envelope routing remains positioned consistently. The individual-versus-All selector is demonstrated around 1102–1120 seconds. Value, Element, Tremolo, Velocity and MIDI sections were also inspected. Video evidence uses 640×360 source frames with approximate perspective correction; firmware is not disclosed. It cannot establish exact present-firmware timing or every fine animation.
- The [official field guide](https://teenage.engineering/guides/op-1) establishes control roles and screen structure. The [current guide PDF](https://assets.teenage.engineering/_img/69f248938d433104c4dd1846_original.pdf) is static and does not define animation equations. The original OP-1 manual differs in its Random encoder order; the implementation follows field references.

## Reconstruction

Random's source clock and trace, the Element/Value horizontal chain, Tremolo's two branches, the four MIDI routes and Velocity's source split use independently measured guide geometry. The palette is sampled from the supplied references. Original vector glyphs give thin numerals/captions without depending on OS font availability; no manufacturer font file or screen bitmap is shipped.

Value draws a four-cycle waveform thumbnail that advances with published audio phase. Free-running clocks rotate their hands; tempo-locked clock digits remain fixed. Random draws eight retained values from the engine's actual random source and scrolls them with that source's phase. These are signal-bound animations rather than a GUI-only random generator or wall-clock wobble. Module icons remain category symbols. Routing wires and envelope curves stay stationary when their controls are held. Focus highlighting follows encoder interaction. Decorative particles, unrelated phase-driven movement and unreferenced input strips have been removed.

Random can select an individual parameter or All; All displays four parameter caps. Element's G drawing remains available with an explicitly documented manual source, not a physical accelerometer claim.

## Verification and remaining limits

Pixel-region tests check that the Value thumbnail and free clock move, tempo-locked numbers and routing blocks stay fixed, Random responds to retained source data, non-oscillator pages do not move with an unrelated phase, and bypass stays still. Native screenshots and phase sequences are generated from the application renderer. Core tests verify that the Random history contains actual generated samples and individual routing stays isolated.

Exact font outlines, antialiasing, all input-dependent micro-animations, raw hardware speed curves and current-firmware interpolation remain unmeasured. Static layout reconstruction and sampled motion are evidence of specific matches; they are not proof of absolute pixel/frame parity. Achieving that final claim requires matched firmware, known controls/input and a calibrated reference capture.
