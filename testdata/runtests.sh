#!/bin/sh


cd `dirname $0` # make sure we're in the directory containing this script
rm -rf output
mkdir output || exit 1
cd output || exit 1

EXEDIR=../../build
if [ "$1" ]; then
    EXEDIR=$1
fi
export BDECOPY_DATADIR=$PWD/../../conf

EXE=${EXEDIR}/bde_copy

if [ ! -x $EXE ]; then
    echo "$EXE not found or not executable, please pass the path containing `bde_copy` as an argument" >&2
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
$EXE -c ../test1.cfg ../par1.crs - par1stdout.log > par1.out
$EXE -c ../test2.cfg ../par1.crs - par2stdout.log > par2.out
$EXE -c ../test3.cfg ../par1.crs - par3stdout.log > par3.out
$EXE -c ../test1.cfg ../escape.crs - escapestdout.log > escape.out
$EXE -c ../test4.cfg ../par1.crs - par4stdout.log > par4.out
$EXE -c ../test5.cfg ../par1.crs - par5a.log 
$EXE -c ../test5.cfg ../parh.crs - par5bstdout.log > par5b.out
$EXE -c ../test5.cfg ../parhz.crs.gz - par5cstdout.log > par5c.out
$EXE -c ../test6.cfg ../par1.crs - par6astdout.log > par6a.out
$EXE -c ../test6.cfg ../par1.crs - par6bstdout.log > par6b.out
$EXE -c ../test1.cfg -p ../scg.crs.p0+../scg.crs.p1.gz ../scg.crs - scgpstdout.log > scgp.out
$EXE -c ../test1.cfg ../scg.crs+../scg1.crs - scg1stdout.log > scg1.out
$EXE -c ../test1.cfg -l 0 scg - scg2stdout.log > scg2.out
$EXE -c ../test1.cfg -x -l 0 -d 20100401030201 scg - scg5stdout.log > scg5.out

$EXE -c ../testutf1.cfg ../mixutf.crs - testutf1stdout.log > mixutf1.out
$EXE -c ../testutf5.cfg ../lolutf.crs - lolutfstdout.log > lolutf.out
$EXE -c ../test1.cfg -w user_modify_flag=N ../scg.crs - scgw1stdout.log > scgw1.out

# Test diff paramater options.
$EXE -c ../test1.cfg -o id:area:shape ../par1.crs - par1o.log > par1o.out
$EXE -c ../test1.cfg -a ../par1.crs - par1a.log > par1a.out
$EXE -c ../test1.cfg -n ../par1.crs - par1n.log > par1n.out
$EXE -c ../test1.cfg -h ../par1.crs - par1h.log

# Checks no output to stdout and gets first line of stderr.
$EXE -c ../test1.cfg ../par1.crs -+ par1f1.out 2>&1 > stdout | head -n1 > par1f1.log
if [ ! -s stdout ]; then rm stdout; fi
$EXE -c ../test1.cfg ../par1.crs par1f2.out - 2>&1 > stdout2 | head -n1 > par1f2.log
if [ ! -s stdout2 ]; then rm stdout2; fi
$EXE -c ../test1.cfg - ../par1.crs par1f3.out 2>&1 > stdout3 | head -n1 > par1f2.log
if [ ! -s stdout3 ]; then rm stdout3; fi
$EXE -c ../test1.cfg ../par1.crs - 2>&1 > stdout4 | head -n1 > par1f3.log
if [ ! -s stdout4 ]; then rm stdout4; fi

# Bogus calls
$EXE -c nonexistent.cfg ../lolutf.crs bogus1.out bogus1.log 2> bogus1.stderr > bogus1.stdout
perl -pi.bak -e 's/^(ConfigFile\:\s)[^\.].*[\\|\/](.+\.cfg)$/$1$2/g' *log || exit 1
rm -f *.bak

gunzip *.gz
diff -x .gitattributes . ../validate

