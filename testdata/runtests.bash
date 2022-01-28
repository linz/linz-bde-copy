#!/usr/bin/env bash

cd `dirname $0` # make sure we're in the directory containing this script
rm -rf output
mkdir output || exit 1
cd output || exit 1

EXEDIR=../../build
if [ "$1" ]
then
  EXEDIR=$1
fi
export BDECOPY_DATADIR=$PWD/../../conf

EXE=${EXEDIR}/bde_copy

if [ ! -x $EXE ]
then
  echo "$EXE not found or not executable, please pass the path containing $(bde_copy) as an argument" >&2
  exit 1
fi

$EXE -c ../test1.cfg ../scg.crs scg.out scg.log
$EXE -c ../test1.cfg ../par1.crs par1.out par1.log
$EXE -c ../test2.cfg ../par1.crs par2.out par2.log
$EXE -c ../test3.cfg ../par1.crs par3.out par3.log
$EXE -c ../test1.cfg ../escape.crs escape.out escape.log
$EXE -c ../test4.cfg ../par1.crs par4.out par4.log
$EXE -c ../test5.cfg ../par1.crs par5a.out par5a.log
$EXE -c ../test5.cfg ../parh.crs par5b.out par5b.log
$EXE -c ../test5.cfg ../parhz.crs.gz par5c.out par5c.log
$EXE -c ../test6.cfg ../par1.crs par6a.out par6a.log
$EXE -c ../test6.cfg ../parh.crs par6b.out par6b.log
$EXE -c ../test1.cfg -p ../scg.crs.p0+../scg.crs.p1.gz ../scg.crs scgp.out scgp.log
$EXE -c ../test1.cfg ../scg.crs+../scg1.crs scg1.out scg1.log
$EXE -c ../test1.cfg -l 0 scg scg2.out scg2.log
$EXE -c ../test1.cfg -l 0 -d 20100401030201 scg scg3.out scg3.log
$EXE -c ../test1.cfg -x -l 0 scg scg4.out scg4.log
$EXE -c ../test1.cfg -x -l 0 -d 20100401030201 scg scg5.out scg5.log

$EXE -c ../test1.cfg ../par1.crs -z par6.out.gz par6.log

$EXE -c ../test1.cfg -w user_modify_flag=N ../scg.crs scgw1.out scgw1.log
$EXE -c ../test1.cfg -w user_modify_flag=N:data_type!=T ../scg.crs scgw2.out scgw2.log

$EXE -c ../testutf1.cfg ../mixutf.crs mixutf1.out testutf1.log
$EXE -c ../testutf2.cfg ../mixutf.crs mixutf2.out testutf2.log
$EXE -c ../testutf3.cfg ../mixutf.crs mixutf3.out testutf3.log
$EXE -c ../testutf4.cfg ../mixutf.crs mixutf4.out testutf4.log
$EXE -c ../testutf5.cfg ../lolutf.crs lolutf.out lolutf.log

# OutputFile as '-' Checks
$EXE -c ../test1.cfg ../par1.crs - par7.log > par7.stdout
$EXE -c ../test2.cfg ../par1.crs - par8.log > par8.stdout
$EXE -c ../test3.cfg ../par1.crs - par9.log > par9.stdout
$EXE -c ../test1.cfg ../escape.crs - escape2.log > escape2.stdout
$EXE -c ../test4.cfg ../par1.crs - par10.log > par10.stdout
$EXE -c ../test5.cfg ../par1.crs - par5a.log
$EXE -c ../test5.cfg ../parh.crs - par11.log > par11.stdout
$EXE -c ../test5.cfg ../parhz.crs.gz - par12.log > par12.stdout
$EXE -c ../test6.cfg ../par1.crs - par13.log > par13.stdout
$EXE -c ../test6.cfg ../par1.crs - par14.log > par14.stdout
$EXE -c ../test1.cfg -p ../scg.crs.p0+../scg.crs.p1.gz ../scg.crs - scgp2.log > scgp2.stdout
$EXE -c ../test1.cfg ../scg.crs+../scg1.crs - scg6.log > scg6.stdout
$EXE -c ../test1.cfg -l 0 scg - scg7.log > scg7.stdout
$EXE -c ../test1.cfg -x -l 0 -d 20100401030201 scg - scg8.log > scg8.stdout

$EXE -c ../testutf1.cfg ../mixutf.crs - testutf5.log > mixutf5.stdout
$EXE -c ../testutf5.cfg ../lolutf.crs - lolutf1.log > lolutf1.stdout
$EXE -c ../test1.cfg -w user_modify_flag=N ../scg.crs - scgw3.log > scgw3.stdout

# Test diff paramater options.
$EXE -c ../test1.cfg -o id:area:shape ../par1.crs - par1o.log > par1o.stdout
$EXE -c ../test1.cfg -a ../par1.crs - par1a.log > par1a.stdout
$EXE -c ../test1.cfg -n ../par1.crs - par1n.log > par1n.stdout
$EXE -c ../test1.cfg -h ../par1.crs - par1h.log

# Redirects stdout to file, and redirects stderr through stdout to pipt the first line of stderr to file.
# Removes the stdout file if empty.
$EXE -c ../test1.cfg ../par1.crs -+ par1f1.out 2>&1 > par1f1.stdout | head -n1 > par1f1.stderr
if [ ! -s par1f1.stdout ]
then
  rm par1f1.stdout
fi
$EXE -c ../test1.cfg ../par1.crs par1f2.out - 2>&1 > par1f2.stdout | head -n1 > par1f2.stderr
if [ ! -s par1f2.stdout ]
then
  rm par1f2.stdout
fi
$EXE -c ../test1.cfg - ../par1.crs par1f3.out 2>&1 > par1f3.stdout | head -n1 > par1f3.stderr
if [ ! -s par1f3.stdout ]
then
  rm par1f3.stdout
fi
$EXE -c ../test1.cfg ../par1.crs - 2>&1 > par1f4.stdout | head -n1 > par1f4.stderr
if [ ! -s par1f4.stdout ]
then
  rm par1f4.stdout
fi

# Bogus calls
$EXE -c nonexistent.cfg ../lolutf.crs bogus1.out bogus1.log 2> bogus1.stderr > bogus1.stdout
perl -pi.bak -e 's/^(ConfigFile\:\s)[^\.].*[\\|\/](.+\.cfg)$/$1$2/g' *log || exit 1
rm -f *.bak

gunzip *.gz
diff -x .gitattributes . ../validate
