# LINZ bde_copy

Programme that extracts LINZ Bulk Data Extracts (BDE) files and optionally
creates normalised files that are more suitable for loading into external
systems. If you would like to know more about the LINZ BDE format and the
Landonline BDE see http://www.linz.govt.nz/survey-titles/landonline-data/landonline-bde/index.aspx

## Copyright

Copyright 2011 Crown copyright (c) Land Information New Zealand and the New
Zealand Government. All rights reserved.

## License

This program is released under the terms of the license contained in the file
LICENSE.

## Running bde_copy

See bde_copy.help or run bde_copy -? for more information

## Build Requirements

bde_copy requires the cross platform make system cmake. Version 2.8 or greater
must be downloaded and installed. See www.cmake.org for more information

bde_copy also requires zlib for gzip compression support. www.zlib.net

## How to Build on Windows (Visual Studio 2008)

- Install cmake 2.8 from www.cmake.org. Make sure you have cmake in your path
- Install the complete zlib development library from http://gnuwin32.sourceforge.net/packages/zlib.htm
- In the top level of bde_copy source directory create a "build" directory:
```
    mkdir build
    cd build
```
- Run cmake for a visual studio release project
``` 
    cmake -G "Visual Studio 9 2008" -DCMAKE_BUILD_TYPE=Release ..
```    
- Open up the Visual Studio solution file bde_copy.sln and then build the solution
- You should now have new build bde_copy in MSWin32/Release directory.

## How to Build on UNIX (GNU make)

- Make sure cmake is installed
- Make sure zlib runtime library and development headers are installed
- In the top level of bde_copy source directory create a "build" directory:
```
    mkdir build && cd build
```
- Run cmake (setting the install path to /usr:
```
    cmake -DCMAKE_INSTALL_PREFIX=/usr ..
```
- Make and (optionally install):
``` 
    make
    make install
```

To create a debug build use 
```
    cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug ..
```
## Testing

Make sure you have Perl installed on your system. Change into testdata directory
and run tests using "runtests" script. Use a programme like kdiff3 or
BeyondCompare and do a directory comparison between the output and validate
directories. At a future point this will be automated so file comparisons are
not required.
