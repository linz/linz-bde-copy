rem @echo off
setlocal
set exe="../vc/Debug/bde_copy.exe"
if not "%1" == "r" echo Use "r" argument to test release version
if "%1" == "r" set exe="../vc/Release/bde_copy.exe"

del /q output\*.*

%exe% -c test1.cfg scg.crs output\scg.out output\scg.log
%exe% -c test1.cfg par1.crs output\par1.out output\par1.log
%exe% -c test2.cfg par1.crs output\par2.out output\par2.log
%exe% -c test3.cfg par1.crs output\par3.out output\par3.log
%exe% -c test1.cfg escape.crs output\escape.out output\escape.log
%exe% -c test4.cfg par1.crs output\par4.out output\par4.log
%exe% -c test1.cfg -p scg.crs.p0+scg.crs.p1.gz scg.crs output\scgp.out output\scgp.log
%exe% -c test1.cfg scg.crs+scg1.crs output\scg1.out output\scg1.log
%exe% -c test1.cfg -l 0 scg output\scg2.out output\scg2.log
%exe% -c test1.cfg -l 0 -d 20100401030201 scg output\scg3.out output\scg3.log
%exe% -c test1.cfg -x -l 0 scg output\scg4.out output\scg4.log
%exe% -c test1.cfg -x -l 0 -d 20100401030201 scg output\scg5.out output\scg5.log

%exe% -c test1.cfg par1.crs -z output\par6.out.gz output\par6.log


if "%1" == "r" (
cd output
for %%f in (*.log) do perl -pi.bak -e "s/Release/Debug/g" %%f
del /q *.bak
)
