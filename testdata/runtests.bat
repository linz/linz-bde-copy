rem @echo off
setlocal
set exe="../../MSWin32/Debug/bde_copy.exe"
if not "%1" == "r" echo Use "r" argument to test release version
if "%1" == "r" set exe="../../MSWin32/Release/bde_copy.exe"

mkdir output
cd output
del /q *.*

%exe% -c ../test1.cfg ../scg.crs scg.out scg.log
%exe% -c ../test1.cfg ../par1.crs par1.out par1.log
%exe% -c ../test2.cfg ../par1.crs par2.out par2.log
%exe% -c ../test3.cfg ../par1.crs par3.out par3.log
%exe% -c ../test1.cfg ../escape.crs escape.out escape.log
%exe% -c ../test4.cfg ../par1.crs par4.out par4.log
%exe% -c ../test5.cfg ../par1.crs par5a.out par5a.log
%exe% -c ../test5.cfg ../parh.crs par5b.out par5b.log
%exe% -c ../test5.cfg ../parhz.crs.gz par5c.out par5c.log
%exe% -c ../test6.cfg ../par1.crs par6a.out par6a.log
%exe% -c ../test6.cfg ../parh.crs par6b.out par6b.log
%exe% -c ../test1.cfg -p ../scg.crs.p0+../scg.crs.p1.gz ../scg.crs scgp.out scgp.log
%exe% -c ../test1.cfg ../scg.crs+../scg1.crs scg1.out scg1.log
%exe% -c ../test1.cfg -l 0 scg scg2.out scg2.log
%exe% -c ../test1.cfg -l 0 -d 20100401030201 scg scg3.out scg3.log
%exe% -c ../test1.cfg -x -l 0 scg scg4.out scg4.log
%exe% -c ../test1.cfg -x -l 0 -d 20100401030201 scg scg5.out scg5.log

%exe% -c ../test1.cfg ../par1.crs -z par6.out.gz par6.log

%exe% -c ../test1.cfg -w user_modify_flag=N ../scg.crs scgw1.out scgw1.log
%exe% -c ../test1.cfg -w user_modify_flag=N:data_type!=T ../scg.crs scgw2.out scgw2.log

%exe% -c ../testutf1.cfg ../mixutf.crs mixutf1.out testutf1.log
%exe% -c ../testutf2.cfg ../mixutf.crs mixutf2.out testutf2.log
%exe% -c ../testutf3.cfg ../mixutf.crs mixutf3.out testutf3.log
%exe% -c ../testutf4.cfg ../mixutf.crs mixutf4.out testutf4.log

for %%f in (*.log) do perl -pi.bak -e "s/^(ConfigFile\:\s)\w.*\\(.+\.cfg)$/$1$2/g" %%f
del /q *.bak

