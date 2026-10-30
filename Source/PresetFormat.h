#pragma once
#include <juce_core/juce_core.h>

namespace field::preset {
// Read the JSON carried by an AIFF/AIFC APPL chunk with signature "op-1".
// Returns its original object, including unknown fields. No guessed conversion
// of hardware knob values or DSP parameters is performed. An ordinary file or
// AIFF without this metadata returns a void var and clears error. Malformed
// recognized AIFF/AIFC containers or op-1 metadata return void with an error.
// rawPayload, when provided, receives exact bytes after the four-byte signature
// including any NUL/space padding. It is cleared on missing metadata or failure.
// This validates container bounds, not COMM/audio codec contents; use an audio
// reader separately. Control-thread only. Metadata is capped at 1 MiB.
juce::var readMetadata(const juce::File&, juce::String& error,
                       juce::MemoryBlock* rawPayload = nullptr);

// A metadata object with a nonempty string "type". Unknown types are retained
// rather than reported as a known software engine.
bool isPreset(const juce::var&);

// Field Studio's declared engine order:14 melodic choices plus DBox at14.
// Returns -1 for unknown types or sample-kit drum metadata. The original
// metadata remains available to callers.
int engineIndex(const juce::var&);
}
