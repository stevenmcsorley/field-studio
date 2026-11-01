# miniaudio capture dependency

Pinned version: 0.11.23. Upstream: https://github.com/mackron/miniaudio

Commit: f40cf03f80cdb7e741d43e53b7e706e8c1394bcf

miniaudio.h SHA-256: 7e4f3f13c8fe66df2080ac3dd12a89193e3c2463cb7f067c798abd7331cd8ee6

Used under the MIT option in LICENSES/miniaudio.txt. The capture translation unit disables decoding, encoding, resource manager, node graph, generation and high-level engine features. This dependency opens explicitly selected external inputs/loopback devices; JUCE remains the instrument's audio engine. Header is unmodified.
