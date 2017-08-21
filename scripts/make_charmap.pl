################################################################################
#
#
#
# Copyright 2011 Crown copyright (c)
# Land Information New Zealand and the New Zealand Government.
# All rights reserved
#
# This program is released under the terms of the new BSD license. See the 
# LICENSE file for more information.
#
################################################################################

use Encode;

my $encoding = 'cp-1252';

for my $i (0x80 .. 0xFF )
{
    my $src = pack("C",$i);
    my $str = decode($encoding,$src);
    my $utf = encode_utf8($src);
    my @utf = unpack("C*",$utf);
    my $xsrc = sprintf("\\x%X",$i);
    my $xutf = join( ' ', map {sprintf("\\x%X",$_)} @utf);
    print "output_char $xsrc $xutf\n";
}
