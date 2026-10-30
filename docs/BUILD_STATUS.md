# Local build status

This workspace contains native Linux x86_64 standalone and VST3 builds. The project also has Windows/macOS build configuration and a three-platform CI workflow; those platforms have not been built here. No release has been pushed or published.

Use `scripts/run.sh` for the development standalone and `scripts/package.py` to produce a local bundle with standalone, VST3, documentation and SHA-256 file hashes. Packaging does not install plugins or change a DAW's configuration. The source project and pinned dependency definition are in this workspace; firmware/reference research is separate and not included in the application bundle.
