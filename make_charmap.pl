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
