# Dependencies and research provenance

JUCE8.0.16-security is pinned to commit `ea77444750de45e036e0279bb0291144bf8e3688` from https://github.com/juce-framework/JUCE. Its licensing files remain in the dependency checkout; JUCE offers AGPLv3 and commercial licensing. Its bundled VST3 SDK retains the SDK's notices. No proprietary firmware or manufacturer sample library is linked into Field Studio.

Reference material lives in the sibling `op1-field-research` directory and is not application artwork. The instrument panel is drawn with code. DSP is independently written; preset-format facts were corroborated against independently parsed public research fixtures. This implementation is not affiliated with teenage engineering and does not establish identical OP-1 field sound or behaviour.

The original Field Studio code is MIT licensed. This beta uses JUCE under AGPLv3 and conveys combined standalone/plugin binaries under AGPLv3 with complete corresponding source. See ../LICENSING.md, ../LICENSE and ../LICENSES/AGPL-3.0.txt. Other dependency notices are included without relicensing their code.
