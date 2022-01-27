# LINZ bde_copy

[![Build Status](https://secure.travis-ci.org/linz/linz-bde-copy.svg)](http://travis-ci.org/linz/linz-bde-copy)
[![Actions Status](https://github.com/linz/linz-bde-copy/workflows/test/badge.svg?branch=master)](https://github.com/linz/linz-bde-copy/actions)

Programme that extracts LINZ Bulk Data Extracts (BDE) files and optionally creates normalised files
that are more suitable for loading into external systems. BDE files were once available as a public
format for accessing Landonline data, but has now been discontinued. However the BDE file format is
still used internally for transfering data from the Landonline system to the
[LDS](https://data.linz.govt.nz). For more information see
[here](http://www.linz.govt.nz/data/linz-data/property-ownership-and-boundary-data/historic-property-databases)

## Copyright

Copyright 2011 Crown copyright (c) Land Information New Zealand and the New Zealand Government. All
rights reserved.

## License

This program is released under the terms of the license contained in the file LICENSE.

## Running `bde_copy`

See `bde_copy.help` or run `bde_copy -?` for more information

## Build Requirements

`bde_copy` requires the cross platform make system `cmake`. Version 2.8 or greater must be
downloaded and installed. See www.cmake.org for more information

`bde_copy` also requires `zlib` for `gzip` compression support. www.zlib.net

## How to Build on Windows (Visual Studio 2008)

- Install cmake 2.8 from www.cmake.org. Make sure you have cmake in your path
- Install the complete zlib development library from
  http://gnuwin32.sourceforge.net/packages/zlib.htm
- In the top level of `bde_copy` source directory create a "build" directory:

```
    mkdir build
    cd build
```

- Run cmake for a visual studio release project

```
    cmake -G "Visual Studio 9 2008" -DCMAKE_BUILD_TYPE=Release ..
```

- Open up the Visual Studio solution file `bde_copy.sln` and then build the solution
- You should now have new build `bde_copy` in MSWin32/Release directory.

## How to Build on UNIX (GNU make)

- Make sure cmake is installed
- Make sure zlib runtime library and development headers are installed
- In the top level of `bde_copy` source directory create a "build" directory:

```
    mkdir build && cd build
```

- Run cmake (setting the install path to /usr):

```
    cmake -DCMAKE_INSTALL_PREFIX=/usr ..
```

- Make and (optionally) run tests and install:

```
    make
    make check
    make install
```

To create a debug build use

```
    cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug ..
```

## Testing

If you have Nix installed you can simply run
`nix-shell --pure --run 'mkdir -p build && cd build && cmake -DCMAKE_INSTALL_PREFIX=$(mktemp --directory) .. && make && make check && make install'`

Otherwise, make sure you have Perl installed on your system.

On GNU systems you can run tests using `make check`, or run
`testdata/runtests.sh <abs_path_to_build_dir>`.

For windows, change into testdata directory and run `runtests.bat`, then use a programme like kdiff3
or BeyondCompare and do a directory comparison between the output and validate directories. At a
future point this will be automated so file comparisons are not required.
