#!/perl -i.bak
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

# 4) Look where cost is in parcels.  Try taking out sprintf, etc.  See 

use strict;

while ( <> ) {
    print $_;
    if ( $_ =~ /^(\s+)\/\*\s+start_state_machine\s*$/i ) {
        my $p = $1;

        my $nclass  = 1;
        my $nstate  = 0;
        my $naction = 0;
        my $prefix  = 'sm';

        my $state_variable  = '';
        my $input_variable  = '';
        my $action_variable = '';
        my $default_state   = '';
        my $dfltid          = 0;

        my @cclass   = ( 0 ) x 256;
        my %clsid    = ();
        my @states   = ();
        my %stid     = ();
        my %actid    = ();
        my $printing = 1;
        my $error    = 0;

        while ( <> ) {
            if ( /^\s*start_state_machine_implementation\s+\*\/\s*$/i ) {
                $printing = 0;
                next;
            }
            if ( /^\s*\/\*\s+end_state_machine_implementation\s*$/i ) {
                $printing = 1;
                next;
            }

            last if /^\s*end_state_machine\s+\*\/\s*$/i;

            print $_ if $printing;

            if ( /^\s*state_variable\s+(\S+)\s*$/ ) {
                $state_variable = $1;
                next;
            }
            if ( /^\s*input_variable\s+(\S+)\s*$/ ) {
                $input_variable = $1;
                next;
            }
            if ( /^\s*action_variable\s+(\S+)\s*$/ ) {
                $action_variable = $1;
                next;
            }
            if ( /^\s*default_state\s+(\S+)\s*$/ ) {
                $default_state = lc( $1 );
                next;
            }
            if ( /^\s*prefix\s+(\S+)\s*$/ ) { $prefix = lc( $1 ); next; }

            $_ = lc( $_ );
            if ( /^\s*class\s+(\w+)\s+(\S)(.*)\2\s*$/i ) {
                my $name = $1;
                my @chars = map { ord( $_ ) } split( '', $3 );
                foreach my $c ( @chars ) { $cclass[$c] = $nclass; }
                $clsid{$name} = $nclass;
                $nclass++;
                next;
            }
            if ( /^\s*state\s+(\w+)\s+(.*?)\s*$/ ) {
                my ( $name, $proc ) = ( $1, $2 );
                push( @states,
                    { id => $nstate, name => $name, process => $proc } );
                $stid{$name} = $nstate;
                $nstate++;
                if ( $nstate > 255 ) {
                    print ">>> Error: More than 255 states\n";
                    $error = 1;
                }
            }
        }

        if ( $default_state ) {
            die "Invalid default state $default_state\n"
              if !exists $stid{$default_state};
            $dfltid = $stid{$default_state};
        }

        foreach my $s ( @states ) {
            my @trans = ( $dfltid ) x ( $nclass );
            $s->{trans} = \@trans;
            foreach my $p ( split( ' ', $s->{process} ) ) {
                if ( $p !~ /^(\w+)(?:\=\>(\w+)(?:\+(\w+))?)?$/ ) {
                    print ">>> ERROR Invalid state change definition $p\n";
                    $error = 1;
                    next;
                }
                my ( $cname, $state, $action ) = ( $1, $2, $3 );
                $state = $s->{name} if !$state;
                my $class = $clsid{$cname};
                if ( !$class ) {
                    print
                      ">>> ERROR Invalid class $cname in state $s->{name}\n";
                    $error = 1;
                }
                if ( !exists $stid{$state} ) {
                    print ">>> ERROR Invalid state $state used in $s->name\n";
                    $error = 1;
                }
                my $st  = $stid{$state};
                my $act = 0;
                if ( $action ) {
                    $actid{$action} = ( ++$naction ) if !exists $actid{$action};
                    $act = $actid{$action};
                }
                $trans[$class] = $st + $act * 256;
            }
        }

        if ( !$error ) {
            print $p, "start_state_machine_implementation */\n\n";
            print $p, "static unsigned char $prefix\_class[256] = {\n";
            for my $i ( 0 .. 15 ) {
                print $p, "   ",
                  join( ",", @cclass[ $i * 16 .. $i * 16 + 15 ] );
                print $i < 15 ? ",\n" : "};\n\n";
            }
            print $p, "static const unsigned short $prefix\_trans[$nstate][$nclass]={\n";
            for my $i ( 0 .. $nstate - 1 ) {
                print $p, "   {", join( ",", @{ $states[$i]->{trans} } ), "}";
                print $i == $nstate - 1 ? "};\n\n" : ",\n";
            }
            print $p,"unsigned short $prefix\_result = $prefix\_trans[$state_variable][$prefix\_class[(int)($input_variable)]];\n";
            print $p,
              "$state_variable = (unsigned char)($prefix\_result & 0xFF);\n";
            print $p,
              "$action_variable = (unsigned char)($prefix\_result >> 8);\n";
            print "\n";
            foreach my $i ( 0 .. $nstate - 1 ) {
                print $p, "#define $prefix\_state_$states[$i]->{name} $i\n";
            }
            print "\n";
            foreach my $k ( sort { $actid{$a} <=> $actid{$b} } keys %actid ) {
                print $p, "#define $prefix\_action_$k $actid{$k}\n";
            }
            print $p, "\n", $p, "/* end_state_machine_implementation\n";
            print $p, "   end_state_machine */\n";
        }
    }
    last if !$_;
}


__END__

/* start_state_machine

# Example of state machine code

state_variable state
input_variable *ch
action_variable action

class digit '123456789'
class point '.'
class sign '-'
class space ' '
class start '(,'

state none start=>ready
state ready space start sign=>sign+act_start digit=>number+start
state sign digit=>number
state number digit point=>number2 space=>none+end
state number2 digit=>number2+act_addndp space=>none+end

default_state none

end_state_machine */
