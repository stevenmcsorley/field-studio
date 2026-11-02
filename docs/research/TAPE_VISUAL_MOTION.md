# Recording LCD and motion evidence

## Sources inspected

- [Official OP-1 field guide](https://teenage.engineering/guides/op-1), firmware 1.7.0: the tape-mode layout, recorder controls and tape-style overview. The tape-mode graphic is `694075983e7f1c62f9b140cb_opt.svg`; the overview distinguishes Studio, Vintage, Porta and Disc Mini. These files are research references, not bundled UI assets.
- [SON WU — How I make Ambient Music](https://www.youtube.com/watch?v=mB586MEe1ww): inspected excerpts 390–460 and 600–685 seconds. Around 630 seconds the central cassette and current lane are coloured for recording; around 645 seconds the cassette is pale after recording; the tape strip passes a stationary head. Around 660 seconds tape position changes independently of the drawn cassette's location. The source is 640×360, so its tiny LCD cannot establish exact typography, every widget or precise angular speed.
- [SON WU — the OP-1 field is so much BETTER!](https://www.youtube.com/watch?v=lF_oH4fB_3I): inspected 1184–1264 seconds. This includes the tape-style browser and a sound comparison in the sampler. It does **not** establish the animation of all four recorder mechanisms.

The owner's supplied tape guide image establishes the two-reel layout. Cuckoo's original OP-1 demonstrations are useful workflow references, but the original device is not the field's four-style LCD specification.

## Implementation

The renderer uses original vectors. Studio has the guide's two-reel transport; Vintage adds a different reel structure; Porta has a cassette shell and two turning hubs; Disc Mini has a cartridge/disc display. Studio and Porta have direct layout references. Vintage and Disc Mini motion/layout remain original approximations requiring clearer device footage.

A scalar travel counter follows the actual audio engine's signed tape increment, including tape-speed modulation and held reverse. Loop/chop playhead wraps do not reset that counter. Stop, brake, zero speed and count-in halt mechanism motion; scrubbing changes it. The counter is published with the existing audio-to-UI snapshot. No independent wall-clock animation runs while the engine is stopped. Relative angular speed and pack-radius interpolation are software approximations, not measured mechanical models.

The default sixteen-second strip scrolls beneath a centred head. Mouse-wheel zoom retains that relationship; explicit six-minute overview restores an absolute timeline. Paint and pointer seeking share the same time origin. Selected splice intervals under the head are blue, other material on that track ochre, other tracks grey. Red overlays only the actual current continuous recording stroke; discontinuous seeks/wraps start another stroke. Count-in has its own indicator. The right red line remains Level/Pan control position, not an invented audio meter.

## Validation and limits

A focused native test checks signed travel across loop wraps, brake/stop/scrub, scrolled pointer mapping, fixed-head scrolling, bounded red recording overlay and mechanism motion/static states for all styles. Preview PNGs/GIFs are deterministic renderer examples, not recordings of the hardware or proof of exact animation timing. Existing sound algorithms, tape write laws, MIDI IDs and recording button semantics are retained.

A full hardware take database, accurate silent gaps, every edit animation, record/key arming gestures, exact mechanism physics and frame/pixel parity remain unverified or absent. Take highlighting uses existing splice markers and audio extent; it cannot distinguish every historical recording from padded silence. No hardware recording/DAW/latency certification is implied by the focused check.
