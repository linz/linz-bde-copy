# Change Log

All notable changes for the LINZ BDE Copy are documented in this file.

## [X.Y.Z] - YYYY-MM-DD

### Removed

- Drop Ubuntu 18.04 support after
  [GitHub dropped their runner support](https://github.com/actions/runner-images/issues/6002).

## [1.5.4] - 2022-12-21

### Added

- Build on Ubuntu 22.04

## [1.5.3] - 2022-05-02

### Fixed

- Force pushing changes to origin remote

## [1.5.2] (broken) - 2022-05-02

### Improved

- Simplify verifying all jobs are passing

## [1.5.1] (broken) - 2022-05-02

### Improved

- Bump linz/linz-software-repository
- Use supported Debian compatibility version
- Untether Debian control versions

## [1.5.0] (broken) - 2022-02-24

### Added

- Support for Ubuntu 20.04/LTS

### Improved

- Quality improvements

## [1.4.0] - 2021-12-07

### Added

- Stalebot configuration
- Github action test workflow and badge
- Package building and publishing steps

### Fixed

- Fix Syntax string to include binary name

### Improved

- Documentation improvements

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

## [1.2.1] - 2016-05-01

### Changed

- Minor documentation changes

## [1.2.0] - 2015-04-09

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
- Added unicode escapes to string definitions, multi character replacement, test cases, and updated
  configuration file
- Added tests to msdos test script
- Change config to check to UTF-8 characters and pass them through as valid UTF-8
- More stuff for strange data in Landonline

### Fixed

- Fixing remaining compiler warnings and introduced bug
