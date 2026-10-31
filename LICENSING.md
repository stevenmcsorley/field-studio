# Free software and dependency licensing

## Original Field Studio code: MIT

Field Studio's original code, original graphics and project documentation are licensed under the [MIT License](LICENSE). You may use, modify, redistribute and sell that original work, including in commercial projects, provided you retain its copyright and permission notice. There is no usage fee or warranty.

This grant does not relicense third-party code, manufacturer reference material or user-imported samples. Respect their separate terms.

## Packaged application: AGPLv3 with corresponding source

The standalone and plugin downloads combine Field Studio with JUCE. JUCE is not MIT-licensed: the pinned framework offers AGPLv3 or a separate commercial licence. This project's free beta builds use the **AGPLv3 route**, not an assumed commercial JUCE licence. The combined application binaries are conveyed under **GNU AGPL version 3**; the original Field Studio files remain separately available under MIT. The full AGPLv3 text is in [LICENSES/AGPL-3.0.txt](LICENSES/AGPL-3.0.txt).

Everyone can use the app, including commercially, and make changes. When distributing the combined application, comply with AGPLv3, including providing its corresponding source and preserving the applicable notices/licences. AGPLv3 also has requirements for modified versions offered for remote network interaction. MIT permission for our original files does not remove JUCE's terms or make the complete JUCE-based application permissively licensed.

Each binary release is accompanied at the same release/download location by a complete source ZIP containing this project, its build scripts and the pinned JUCE dependency, together with their licence notices. Fetch that matching source ZIP, not only GitHub's automatic source archive (which omits the vendor checkout). The package BUILD_INFO.json records the source commit and JUCE revision. Other bundled dependencies retain their own notices in THIRD-PARTY-NOTICES/.

A distributor wanting a proprietary combined build would need an appropriate commercial JUCE licence (and compliance with other dependency terms), or a replacement for JUCE. No commercial JUCE entitlement is granted by this repository. No manufacturer code, factory sound library or extracted manufacturer artwork is bundled.

## Primary licensing references

- [JUCE licence at the exact pinned revision](https://github.com/juce-framework/JUCE/blob/ea77444750de45e036e0279bb0291144bf8e3688/LICENSE.md)
- [GNU AGPLv3](https://www.gnu.org/licenses/agpl-3.0.html)
- [MIT/Expat compatibility in the FSF licence list](https://www.gnu.org/licenses/license-list.html#Expat)
