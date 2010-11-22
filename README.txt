Build notes for bde_copy using visual studio on win32

1) Unzip zlib125.zip
2) Unzip contents of zlib124_masm_obj.zip to zlib-1.2.5\contrib\masmx86
3) Build the VS project file vc\bde_copy.vcproj
4) cd into testdata directory and run tests using runtests.bat. Use a programme like kdiff3 or BeyondCompare and do a directory comparison between the output and validate directories.

Build notes for bde_copy on linux

1) make sure zlib 1.25 is compiled or installed on system
2) cd into unix directory and run "make dist" to build and make distributable copy of programme in unix/release/bdecopy-release.tar.gz. This tarball include the programme and all required config files. These files can be installed and run from any directory. At some stage in the future a more *unix style build and install system will be created.
4) cd into testdata directory and run tests using runtests.bat. Use a programme like kdiff3 and do a directory comparison between the output and validate directories.

