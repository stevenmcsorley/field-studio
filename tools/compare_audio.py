#!/usr/bin/env python3
"""Compare controlled PCM WAV renders; a score is evidence, not a parity certificate.

Use the same note/velocity, preset, parameter state, source audio and sample rate.
No pitch shifting, loudness normalization or automatic alignment is performed:
those operations would hide real differences. Files must be PCM16/24/32 WAV.
"""
import argparse
import array
import json
import math
import struct
import wave
from pathlib import Path


def read_pcm(path):
    with wave.open(str(path), 'rb') as audio:
        if audio.getcomptype() != 'NONE':
            raise ValueError('Expected uncompressed PCM WAV')
        channels, rate, width, frames = (audio.getnchannels(), audio.getframerate(),
                                        audio.getsampwidth(), audio.getnframes())
        if frames > rate * 600 or channels not in (1, 2) or width not in (2, 3, 4):
            raise ValueError('Supported: up to ten minutes, mono/stereo, PCM16/24/32')
        raw = audio.readframes(frames)
    samples = array.array('f')
    for offset in range(0, len(raw), width):
        value = int.from_bytes(raw[offset:offset+width], 'little', signed=True)
        samples.append(value / float(1 << (width * 8 - 1)))
    return rate, channels, samples


def compare(reference, candidate):
    rr, rc, ref = read_pcm(reference)
    cr, cc, cand = read_pcm(candidate)
    if (rr, rc) != (cr, cc):
        raise ValueError('Rates/channels differ: convert explicitly and record why before comparing')
    length = min(len(ref), len(cand))
    if length == 0:
        raise ValueError('Empty recording')
    power = sum(float(x) * x for x in ref[:length]) / length
    error = sum((float(ref[i]) - cand[i])**2 for i in range(length)) / length
    peak = max(abs(float(ref[i]) - cand[i]) for i in range(length))
    window = max(1, round(rr * .01)) * rc
    envelopes = []
    for start in range(0, length, window):
        end = min(start + window, length)
        a = math.sqrt(sum(float(x)*x for x in ref[start:end]) / (end-start))
        b = math.sqrt(sum(float(x)*x for x in cand[start:end]) / (end-start))
        envelopes.append(abs(a-b))
    return {
        'sample_rate': rr, 'channels': rc,
        'reference_frames': len(ref)//rc, 'candidate_frames': len(cand)//rc,
        'same_length': len(ref) == len(cand),
        'exact_pcm_match': len(ref) == len(cand) and error == 0,
        'rms_error': math.sqrt(error), 'max_sample_error': peak,
        'signal_to_error_db': None if error == 0 or power == 0 else 10*math.log10(power/error),
        'mean_10ms_envelope_error': sum(envelopes)/len(envelopes),
        'limits': 'Same-rate, sample-aligned PCM comparison only; no hardware identity is inferred.'
    }


def self_test():
    import tempfile
    with tempfile.TemporaryDirectory() as directory:
        a, b = (Path(directory)/name for name in ('a.wav','b.wav'))
        def write(path, gain):
            with wave.open(str(path), 'wb') as out:
                out.setparams((1, 2, 8000, 0, 'NONE', 'not compressed'))
                out.writeframes(b''.join(struct.pack('<h', round(gain*12000*math.sin(i*.2)))
                                         for i in range(800)))
        write(a, 1); write(b, 1)
        assert compare(a,b)['exact_pcm_match']
        write(b,.5)
        assert compare(a,b)['rms_error'] > .05


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('reference', nargs='?', type=Path)
    parser.add_argument('candidate', nargs='?', type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--self-test', action='store_true')
    args = parser.parse_args()
    if args.self_test:
        self_test(); print('PASS: audio comparison detects matching and changed PCM')
    else:
        if not args.reference or not args.candidate:
            parser.error('provide reference and candidate WAVs')
        report = json.dumps(compare(args.reference,args.candidate),indent=2)
        if args.output: args.output.write_text(report+'\n')
        print(report)
