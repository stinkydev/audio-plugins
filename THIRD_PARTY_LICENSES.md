# Third-Party Licenses

This project includes or depends on third-party software.  
This file documents the dependency name, source, version/tag, and license.

## Dependencies

### CLAP SDK
- **Source**: https://github.com/free-audio/clap
- **Version/Tag**: `1.2.7`
- **How used**: CLAP plugin SDK used by all plugins via CMake `FetchContent`
- **License**: MIT License

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
- If dependencies are updated, this file should be updated in the same change.
