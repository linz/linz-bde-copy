# Change Log

All notable changes for the LINZ BDE Copy are documented in this file.

## [1.3.0] - 2018-02-05
### Changed
- Print startup errors to stderr (#33)
- Make base configuration file optional
### Added
- Add ability to output to stdout (#31)
- Version flag (-V)
- Support for compiling on MacOSX
### Fixed
- Win32 build
- File size error when reading more than one file
### Improved
- Documentation improvements

## [1.2.0] - 2016-04-13
### Added
- Added static casts to allow compilation of gzip code
- Added script to build `bde_copy.cfg.lolutf8`
- Added additional Landonline UTF8 chars

### Changed
- Updating Win32 binaries
- Updating config documentation

### Fixed
- Fixed for Landonline UTF8 character set

## [1.1.0] - 2013-06-03
### Changed
- Removing compiler warnings
- Linux test script
- UTF8 detection and handling working
- Added unicode escapes to string definitions, multi character replacement, test cases, and updated configuration file
- Added tests to msdos test script
- Change config to check to UTF-8 characters and pass them through as valid UTF-8
- More stuff for strange data in Landonline

### Fixed
- Fixing remaining compiler warnings and introduced bug

