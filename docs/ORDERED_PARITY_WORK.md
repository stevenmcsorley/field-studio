# Ordered parity work

User order (2026-10-30):

1. Synth/effect sound and preset behaviour — evidence-supported control translation implemented and tested; exact sound remains unverified. Known control ranges/selectors can be translated from public preset research. Exact DSP cannot be certified: firmware inspection recovered no engine code, and available preset fixtures mostly contain identical preview audio rather than controlled rendered notes.
2. Envelope curves and modulation routing — implemented and tested: 20 destinations, four signed MIDI routes, six LFO shapes, tempo sync/retrigger, Element sources and optional exponential ADSR. Hardware curves and routing equivalence remain unverified. DBox retains independent transient envelopes.
3. Tape editing, count-in and performance controls — next, deferred at user request until the current build is complete.
4. MIDI/clock/controller support — pending.
5. Screen and animation matching — pending.
6. Continuous audio during heavy editing/loading/saving — pending.

Each phase must leave passing tests and a rebuilt implementation before the next implementation phase starts. Where reference evidence is missing, report the specific unverified behaviour instead of declaring parity. Existing capabilities and limitations remain in PARITY.md; this file tracks the order of the present request.

Current checkpoint: user requested building the current implementation before beginning tape editing. No phase 3–6 implementation has started.
