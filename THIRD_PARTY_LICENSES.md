# Third-Party Licenses

This project includes or depends on third-party software.  
This file documents the dependency name, source, version/tag, and license.

## Dependencies

### CLAP SDK
- **Source**: https://github.com/free-audio/clap
- **Version/Tag**: `1.2.7`
- **How used**: CLAP plugin SDK compiled into all shipped plugins via CMake `FetchContent`
- **License**: MIT License — full text reproduced in [`licenses/CLAP-LICENSE.txt`](licenses/CLAP-LICENSE.txt)

### GoogleTest
- **Source**: https://github.com/google/googletest
- **Version/Tag**: `v1.14.0`
- **How used**: Unit test framework, fetched via CMake `FetchContent` when `BUILD_TESTS=ON`
- **License**: BSD 3-Clause License

### TypeScript
- **Source**: https://github.com/microsoft/TypeScript
- **Version**: `5.9.3` (from `eq/web-ui/package-lock.json`)
- **How used**: Development dependency for building the EQ web UI TypeScript sources
- **License**: Apache License 2.0

## Notes

- Project license is MIT; see [LICENSE](LICENSE).
- Third-party licenses remain the property of their respective owners.
- The full text of redistributed third-party licenses lives in the [`licenses/`](licenses/)
  directory. The build copies it to `build/licenses/` and `cmake --install` emits it
  to `<prefix>/licenses/`, so downstream packagers can bundle complete attribution.
- GoogleTest and TypeScript are test-/build-only and are **not** redistributed, so their
  full license texts are not bundled alongside the binaries.
- If dependencies are updated, this file and the bundled `licenses/` text should be
  updated in the same change to keep the notice matching the pinned versions.
