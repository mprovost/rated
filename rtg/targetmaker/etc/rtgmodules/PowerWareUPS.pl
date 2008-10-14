########################################################################
# $Id: PowerWareUPS.pl,v 1.1 2008/01/19 03:22:14 btoneill Exp $
########################################################################
########################################################################
# Add in airport support
#
#
# Set name of Module, and the OID to check to run this module
#
$MODULENAME = "PowerWareUPS";
$VERSION = '0.1';

@CLASSES=qw(ups electrical);

# Module requires a subroutine called process_module_$MODULENAME to exist.
#
sub process_module_PowerWareUPS($$$);

#
# Local sub-routines
#
sub add_powerware($$);

# We check for xupsIdentManufacturer.0
$MIB_TO_CHECK = ".1.3.6.1.4.1.534.1.1.1.0";


push(@main::statclasses,@CLASSES);

# Set this so it's in the main hash
$main::snmp_modules{$MODULENAME} = $MIB_TO_CHECK;


$main::table_map{'PowerWare-Voltage'} = [ qw(xupsInputVoltage xupsOutputVoltage xupsByPassVoltage) ];
$main::table_options{'PowerWare-Voltage'} = [ qw(factor=1 gauge=on units=V) ];
$main::table_class{'PowerWare-Voltage'} = "electrical";

$main::table_map{'PowerWare-Current'} = [ qw(xupsInputCurrent xupsOutputCurrent) ];
$main::table_options{'PowerWare-Current'} = [ qw(factor=1 gauge=on units=A) ];
$main::table_class{'PowerWare-Current'} = "electrical";

$main::table_map{'PowerWare-Watts'} = [ qw(xupsInputWatts xupsOutputWatts) ];
$main::table_options{'PowerWare-Watts'} = [ qw(factor=1 gauge=on units=W) ];
$main::table_class{'PowerWare-Watts'} = "electrical";

$main::table_map{'PowerWare-Time'} = [ qw(xupsBatTimeRemaining) ];
$main::table_options{'PowerWare-Time'} = [ qw(factor=1 gauge=on units=secs) ];
$main::table_class{'PowerWare-Time'} = "ups";

$main::table_map{'PowerWare-BatVoltage'} = [ qw(xupsBatVoltage) ];
$main::table_options{'PowerWare-BatVoltage'} = [ qw(factor=1 gauge=on units=V) ];
$main::table_class{'PowerWare-BatVoltage'} = "ups";

$main::table_map{'PowerWare-BatCurrent'} = [ qw(xupsBatCurrent) ];
$main::table_options{'PowerWare-BatCurrent'} = [ qw(factor=1 gauge=on units=A) ];
$main::table_class{'PowerWare-BatCurrent'} = "ups";

$main::table_map{'PowerWare-BatCapacity'} = [ qw(xupsBatCapacity) ];
$main::table_options{'PowerWare-BatCapacity'} = [ qw(factor=1 gauge=on units=%) ];
$main::table_class{'PowerWare-BatCapacity'} = "ups";

$main::table_map{'PowerWare-OutputLoad'} = [ qw(xupsOutputLoad) ];
$main::table_options{'PowerWare-OutputLoad'} = [ qw(factor=1 gauge=on units=%) ];
$main::table_class{'PowerWare-OutputLoad'} = "ups";

#
# Local vars
#
%mibs_of_interest_powerwareups= (
        "xupsBatTimeRemaining" => ".1.3.6.1.4.1.534.1.2.1.0",
        "xupsBatVoltage" => ".1.3.6.1.4.1.534.1.2.2.0",
        "xupsBatCurrent" => ".1.3.6.1.4.1.534.1.2.3.0",
        "xupsBatCapacity" => ".1.3.6.1.4.1.534.1.2.4.0",
        "xupsOutputLoad" => ".1.3.6.1.4.1.534.1.4.1.0",
	"xupsInputVoltage" => ".1.3.6.1.4.1.534.1.3.4.1.2.",
	"xupsInputCurrent" => ".1.3.6.1.4.1.534.1.3.4.1.3.",
	"xupsInputWatts" => ".1.3.6.1.4.1.534.1.3.4.1.4.",
	"xupsOutputVoltage" => ".1.3.6.1.4.1.534.1.4.4.1.2.",
	"xupsOutputCurrent" => ".1.3.6.1.4.1.534.1.4.4.1.3.",
	"xupsOutputWatts" => ".1.3.6.1.4.1.534.1.4.4.1.4.",
        "xupsByPassVoltage" => ".1.3.6.1.4.1.534.1.5.3.1.2.",
);

$normal_powerware = [
	[ 1, 3, 6, 1, 4, 1, 534, 1, 3, 4, 1, 1 ],	# xupsInputPhase
	[ 1, 3, 6, 1, 4, 1, 534, 1, 3, 4, 1, 2 ],	# xupsInputVoltage
	[ 1, 3, 6, 1, 4, 1, 534, 1, 3, 4, 1, 3 ],	# xupsInputCurrent
	[ 1, 3, 6, 1, 4, 1, 534, 1, 3, 4, 1, 4 ],	# xupsInputWatts
];

sub process_module_PowerWareUPS($$$) {
    my ($router,$community,$sess) = @_;


    debug("$router supports PowerWare UPS");

    return 1 if(has_class('ups'));

    add_standard_ups();
    add_class('ups');
    
    $sess->map_table( $normal_powerware, \&process_powerware);

    add_class('electrical');

    return 1;
}

sub add_standard_ups() {

    $ifdescr = "Standard UPS stats";
    $ifalias = $ifdescr;
    $ifspeed = "nomaxspeed";
    $bits = 0;

    if ( !$DBOFF ) {
        $iid = &find_interface_id( $rid, $ifdescr, $ifalias, $ifspeed );
    }

    foreach $mib_to_try (keys %mibs_of_interest_powerwareups) {
        next unless($mibs_of_interest_powerwareups{$mib_to_try} =~ /\.0$/);

        $SNMP_Session::suppress_warnings=2;
        @result = rtg_snmpget ("$communities{$router}\@$router", "$mibs_of_interest_powerwareups{$mib_to_try}");
        $SNMP_Session::suppress_warnings=0;
        if(defined $result[0]) {
            if($ifalias eq "") {
                $ifalias = $ifdescr;
            }
            print_target($router,
                "$mibs_of_interest_powerwareups{$mib_to_try}",
                $bits,
                $communities{$router},
                "$mib_to_try" . "_$rid",
                $iid,
                $ifspeed,
                "$ifalias ($ifdescr)");

            # we need to make the table if it 
            # doesn't already exist	
            if ( !$DBOFF ) {	
                create_table($mib_to_try,$rid);
            }

        }
    }

    return 1;
}

sub process_powerware() {
    my $reserved = 0;
    my ($rowindex, $index, $voltage, $current, $watts ) = @_;
    grep ( defined $_ && ( $_ = pretty_print $_),
      ($index, $voltage, $current, $watts ));

    $ifdescr = "Phase $index";
    $ifalias = $ifdescr;

    $ifspeed = "nomaxspeed";

    if ($ifdescr) {
        if ( !$DBOFF ) {
            $iid = &find_interface_id( $rid, $ifdescr, $ifalias, $ifspeed );
        }

        foreach $mib ( keys %mibs_of_interest_powerwareups) {
            # we want to make sure the device supports the requested stat, not all do
            my %oid_to_try = ();

            # only add this value if we're looking for a $index at the end...
            if($mibs_of_interest_powerwareups{$mib} =~ /\.$/) {
                $oid_to_try{"$mibs_of_interest_powerwareups{$mib}$index"} = 0;
            }

print "about to do foreach for $mib...\n";
            foreach $mib_to_try (sort { $oid_to_try{$b} <=> $oid_to_try{$a} } keys %oid_to_try ) {
print "going to try $mib_to_try for $mib...\n";
                $SNMP_Session::suppress_warnings=2;
                @result = rtg_snmpget ("$communities{$router}\@$router", "$mib_to_try");
                $SNMP_Session::suppress_warnings=0;
                if(defined $result[0]) {
                    if($ifalias eq "") {
                        $ifalias = $ifdescr;
                    }
                    print_target($router,
                        "$mib_to_try",
                        $oid_to_try{$mib_to_try},
                        $communities{$router},
                        "$mib" . "_$rid",
                        $iid,
                        $ifspeed,
                        "$ifalias ($ifdescr)");

                    # we need to make the table if it 
                    # doesn't already exist	
                    if ( !$DBOFF ) {	
                        create_table($mib,$rid);
                    } 

                    last;
                } 
            } 
        } 
    }
    else {
        if ($DEBUG) {
            $debug_line = "Ignoring $router $ifalias ($ifdescr) ";
            $debug_line .= "[oper = up] "      if $ifstate == 1;
            $debug_line .= "[oper = down] "    if $ifstate != 1;
            $debug_line .= "[reserved = yes] " if $reserved == 1;
            $debug_line .= "[reserved = no] "  if $reserved != 1;
            debug($debug_line);
        }
    }
}


# have to end with a 1
1;

