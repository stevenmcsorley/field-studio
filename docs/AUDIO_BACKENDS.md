# Audio devices: Windows ASIO and macOS CoreAudio

## Standalone

| Platform | Backend and setup |
|---|---|
| Windows x64 | ASIO is explicitly enabled at build time with `JUCE_ASIO=1`. Install the audio interface vendor's 64-bit ASIO driver, open **Options → Audio/MIDI Settings**, select ASIO and the interface, then set sample rate/buffer size. Existing WASAPI/DirectSound options remain available. |
| macOS Intel / Apple Silicon | Native CoreAudio is the equivalent device backend. Select the interface in **Options → Audio/MIDI Settings**. ASIO is a Windows backend, not a macOS setting. |
| Linux | Existing ALSA device support remains. |

Start with a buffer the interface handles reliably, then lower it while playing and recording. There is no universal buffer setting or measured latency guarantee. Driver/device availability, OS permissions and interface firmware can affect the available choices.

## VST3 / AU

The DAW owns the audio device, sample rate and buffer size. Select ASIO in the Windows DAW's audio settings, or CoreAudio in the macOS DAW. The plugin does not open a second audio device and has no independent ASIO/CoreAudio selector.

## Evidence and limits

The [JUCE backend documentation](https://docs.juce.com/master/juce__audio__devices_8h.html) identifies JUCE_ASIO as Windows-only. The pinned JUCE source provides the bundled ASIO headers and native macOS CoreAudio implementation. Native integration tests construct the ASIO backend type on Windows and the CoreAudio backend type on macOS. This establishes build/backend availability; it does not open a physical interface or measure playback, recording, round-trip latency, stability or clock drift.

Windows ASIO runtime testing with a physical interface remains open. The earlier user report confirms plugin/automation and Faderfox/BeatStep MIDI use, not an ASIO interface test. The remaining shared live-engine lock can still interrupt audio during contention; backend support does not complete the real-time ownership rewrite.

Bundled ASIO code uses its GPLv3 alternative with corresponding source and notices; see [licensing](../LICENSING.md). Downloads remain unsigned/not notarized.
