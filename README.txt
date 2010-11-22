This is the README for bde_copy which is a programme that extracts data from LINZ Bulk Data Extracts (BDE_ files and copies them to the uncompressed data files. If you would like to know more about the LINZ BDE format and the Landonline BDE see http://www.linz.govt.nz/survey-titles/landonline-data/landonline-bde/index.aspx

This program is released under the terms of the license contained in the file LICENSE.

HOW TO BUILD -- Win32 using Visual Studio 2008

1) Unzip zlib125.zip
2) Unzip contents of zlib124_masm_obj.zip to zlib-1.2.5\contrib\masmx86
3) Build the VS project file vc\bde_copy.vcproj

HOW TO BUILD -- UNIX

1) make sure zlib 1.25 is compiled or installed on system
2) cd into unix directory and run "make dist" to build and make distributable copy of programme in unix/release/bdecopy-release.tar.gz. This tarball include the programme and all required config files. These files can be installed and run from any directory. At some stage in the future a more *unix style build and install system will be created.

TESTING

cd into testdata directory and run tests using "runtests" script. Use a programme like kdiff3 or BeyondCompare and do a directory comparison between the output and validate directories.

