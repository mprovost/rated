########################################################################
# $Id: AppleAirport.pl,v 1.1 2008/01/19 03:22:14 btoneill Exp $
########################################################################
########################################################################
# Add in airport support
#
#
# Set name of Module, and the OID to check to run this module
#
$MODULENAME = "AppleAirport";
$VERSION = '0.1';

@CLASSES=qw(network-802.11);

# Module requires a subroutine called process_module_$MODULENAME to exist.
#
sub process_module_AppleAirport($$$);

#
# Local sub-routines
#
sub add_airport($$);

# We check for interfaces.ifNumber.0
# (we used to check for ifIndex.1, but Extreme switches like
# to start at 1001, so it would fail)
#$MIB_TO_CHECK = ".1.3.6.1.2.1.2.2.1.1.1";

# sysConfFirmwareVersion.0
$MIB_TO_CHECK = ".1.3.6.1.4.1.63.501.3.1.5.0";


push(@main::statclasses,@CLASSES);

# Set this so it's in the main hash
$main::snmp_modules{$MODULENAME} = $MIB_TO_CHECK;


$main::table_map{'AirPort-txrx'} = [ qw(physicalInterfaceNumTX physicalInterfaceNumRX) ];
$main::table_options{'AirPort-txrx'} = [ qw(factor=1 units=pkts/s) ];
$main::table_class{'AirPort-txrx'} = "network-802.11";

$main::table_map{'AirPort-error'} = [ qw(physicalInterfaceNumTXError physicalInterfaceNumRXError) ];
$main::table_options{'AirPort-error'} = [ qw(factor=1 units=pkts/s) ];
$main::table_class{'AirPort-error'} = "network-802.11";

#
# Local vars
#
%mibs_of_interest_airport= (
	"physicalInterfaceNumTX" => ".1.3.6.1.4.1.63.501.3.4.2.1.7.",
	"physicalInterfaceNumRX" => ".1.3.6.1.4.1.63.501.3.4.2.1.8.",
	"physicalInterfaceNumTXError" => ".1.3.6.1.4.1.63.501.3.4.2.1.9.",
	"physicalInterfaceNumRXError" => ".1.3.6.1.4.1.63.501.3.4.2.1.10.",
);


$normal_airport = [
	[ 1, 3, 6, 1, 4, 1, 63, 501, 3, 4, 2, 1, 1],	# physicalInterfaceIndex
	[ 1, 3, 6, 1, 4, 1, 63, 501, 3, 4, 2, 1, 2],	# physicalInterfaceName
	[ 1, 3, 6, 1, 4, 1, 63, 501, 3, 4, 2, 1, 3],	# physicalInterfaceUnit
	[ 1, 3, 6, 1, 4, 1, 63, 501, 3, 4, 2, 1, 4],	# physicalInterfaceSpeed
	[ 1, 3, 6, 1, 4, 1, 63, 501, 3, 4, 2, 1, 5]	# physicalInterfaceState
];


sub process_module_AppleAirport($$$) {
	my ($router,$community,$sess) = @_;


	debug("$router supports Apple Airport");

	return 1 if(has_class('network-802.11'));

        $sess->map_table( $normal_airport, \&process_airport );

	add_class('network-802.11');


	return 1;
}

sub process_airport() {
    my $reserved = 0;
    my ($rowindex, $index, $ifalias, $ifunit, $ifspeed, $ifstate ) = @_;
    grep ( defined $_ && ( $_ = pretty_print $_),
      ($index, $ifalias, $ifunit, $ifspeed, $ifstate ));

    # Compaq likes to put a null at the end....
    $bits = 32;

    $ifdescr = "$ifalias$ifunit";
    $ifalias = $ifdescr;

    # Check for "reserved" interfaces, i.e. those we don't want to 
    # include in RTG
    foreach $resv (@reserved) {
        if ( $ifdescr =~ /$resv/ ) {
            $reserved = 1;
        }
    }
    if ($ifdescr) {
        if ( $ifstate == 1 && $reserved == 0 ) {
            if ( !$DBOFF ) {
                $iid = &find_interface_id( $rid, $ifdescr, $ifalias, $ifspeed );
            }
            foreach $mib ( keys %mibs_of_interest_airport ) {
		# we want to make sure the device supports the requested stat, not all do
		my %oid_to_try = ();

		# only add it if the value is actually an oid. This will get rid
		# of the no_32_bit ones that we run into multicast stuff
		if($mibs_of_interest_airport{$mib} =~ /^\.\d/) {
			$oid_to_try{"$mibs_of_interest_airport{$mib}$index"} = $bits;
		}

               foreach $mib_to_try (sort { $oid_to_try{$b} <=> $oid_to_try{$a} } keys %oid_to_try ) {
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
				}
	
				# we need to make the table if it 
				# doesn't already exist	
				if ( !$DBOFF ) {	
					create_table($mib,$rid);
				}

				last;
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
}


# have to end with a 1
1;

