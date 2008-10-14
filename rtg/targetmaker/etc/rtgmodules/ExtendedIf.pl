########################################################################
# $Id: ExtendedIf.pl,v 1.1 2008/01/19 03:22:14 btoneill Exp $
########################################################################
# $Log: ExtendedIf.pl,v $
# Revision 1.1  2008/01/19 03:22:14  btoneill
#
# Targetmaker 0.9 added to the default RTG distribution
#
# Revision 1.5  2005/10/17 15:15:09  btoneill
# Lots and Lots of changes
#
# Revision 1.4  2005/05/11 21:52:48  btoneill
# Lots and lots of changes
#
# Revision 1.3  2005/03/31 15:25:25  btoneill
#
# Lots and lots of bug fixes, and new features, and other stuff.
#
# Revision 1.2  2004/10/15 16:35:32  btoneill
# bug fixes and stuff
#
# Revision 1.1.1.1  2004/10/05 15:03:34  btoneill
# Initial import into CVS
#
# Revision 1.2  2004-09-30 12:27:51-05  btoneill
# Fixed various things
#
# Revision 1.1  2004-09-27 11:51:48-05  btoneill
# Initial revision
#
# Revision 1.4  2004/07/08 19:30:49  btoneill
# Modified to handle function to create config file
# Made changes to work with StandardIf
# class changed to network-extended
#
# Revision 1.3  2004/07/08 16:42:21  btoneill
# Changed MIB to check for so that Extreme switches would be happy
#
# Revision 1.2  2004/06/24 20:07:44  btoneill
# Added proper RCS› tags
#
#
########################################################################
# Add in extended interface stats
#
#
# Set name of Module, and the OID to check to run this module
#
$MODULENAME = "ExtendedIf";
$VERSION = '0.1';

@CLASSES=qw(network-extended);

# Module requires a subroutine called process_module_$MODULENAME to exist.
#
sub process_module_ExtendedIf($$$);

#
# Local sub-routines
#
sub add_extended($$);

# We check for interfaces.ifNumber.0
# (we used to check for ifIndex.1, but Extreme switches like
# to start at 1001, so it would fail)
#$MIB_TO_CHECK = ".1.3.6.1.2.1.2.2.1.1.1";

$MIB_TO_CHECK = ".1.3.6.1.2.1.2.1.0";


push(@main::statclasses,@CLASSES);

# Set this so it's in the main hash
$main::snmp_modules{$MODULENAME} = $MIB_TO_CHECK;


$main::table_map{'MIBII-netpktNU'} = [ qw(ifInNUcastPkts ifOutNUcastPkts) ];
$main::table_options{'MIBII-netpktNU'} = [ qw(factor=1 units=pkts/s) ];
$main::table_class{'MIBII-netpktNU'} = "network-extended";

$main::table_map{'MIBII-discards'} = [ qw(ifInDiscards ifOutDiscards) ];
$main::table_options{'MIBII-discards'} = [ qw(factor=1 units=pkts impulses=on) ];
$main::table_class{'MIBII-discards'} = "network-extended";

$main::table_map{'MIBII-errors'} = [ qw(ifInErrors ifOutErrors) ];
$main::table_options{'MIBII-errors'} = [ qw(factor=1 units=pkts impulses=on) ];
$main::table_class{'MIBII-errors'} = "network-extended";

$main::table_map{'MIBII-multicast'} = [ qw(ifHCInMulticastPkts ifHCOutMulticastPkts) ];
#$main::table_options{'MIBII-multicast'} = [ qw(factor=1 units=pkts impulses=on) ];
$main::table_options{'MIBII-multicast'} = [ qw(factor=1 units=pkts) ];
$main::table_class{'MIBII-multicast'} = "network-extended";

#
# Local vars
#
%mibs_of_interest_extif= (
	"ifInNUcastPkts" => ".1.3.6.1.2.1.2.2.1.12.",
	"ifOutNUcastPkts" => ".1.3.6.1.2.1.2.2.1.18.",
	"ifInDiscards"   => ".1.3.6.1.2.1.2.2.1.13.",
	"ifOutDiscards"  => ".1.3.6.1.2.1.2.2.1.19.",
	"ifInErrors"     => ".1.3.6.1.2.1.2.2.1.14.",
	"ifOutErrors"    => ".1.3.6.1.2.1.2.2.1.20.",
	"ifHCInMulticastPkts" => "no_32_bit_equiv",
	"ifHCOutMulticastPkts" => "no_32_bit_equiv"
);

%mibs_of_interest_extif_64 = (
	"ifInNUcastPkts" => ".1.3.6.1.2.1.31.1.1.1.9.",
	"ifOutNUcastPkts" => ".1.3.6.1.2.1.31.1.1.1.13.",
	"ifHCInMulticastPkts" => ".1.3.6.1.2.1.31.1.1.1.8.",
	"ifHCOutMulticastPkts" => ".1.3.6.1.2.1.31.1.1.1.12."
);

$normal_extif = [
	[ 1, 3, 6, 1, 2, 1, 2,  2, 1, 1 ],        # ifIndex
	[ 1, 3, 6, 1, 2, 1, 2,  2, 1, 2 ],        # ifDescr
	[ 1, 3, 6, 1, 2, 1, 2,  2, 1, 5 ],        # ifSpeed
	[ 1, 3, 6, 1, 2, 1, 31, 1, 1, 1, 18 ],    # ifAlias
	[ 1, 3, 6, 1, 2, 1, 2,  2, 1, 7 ],        # ifAdminStatus
	[ 1, 3, 6, 1, 2, 1, 2,  2, 1, 8 ]         # ifOperStatus
];

$catalyst_extif = [
    [ 1, 3, 6, 1, 2, 1, 2,  2, 1, 1 ],             # ifIndex
    [ 1, 3, 6, 1, 2, 1, 31, 1, 1, 1, 1 ],          # ifXEntry.ifName
    [ 1, 3, 6, 1, 2, 1, 2,  2, 1, 5 ],             # ifSpeed
    [ 1, 3, 6, 1, 2, 1, 31, 1, 1, 1, 18 ],    	   # ifAlias
    [ 1, 3, 6, 1, 2, 1, 2,  2, 1, 7 ],             # ifAdminStatus
    [ 1, 3, 6, 1, 2, 1, 2,  2, 1, 8 ]              # ifOperStatus
];



sub process_module_ExtendedIf($$$) {
	my ($router,$community,$sess) = @_;


	debug("$router supports Extended Interfaces");

	return 1 if(has_class('network-extended'));

	if ( $system eq "Catalyst" ) {
            $sess->map_table( $catalyst_extif, \&process_ext );
        }
        else {
            $sess->map_table( $normal_extif, \&process_ext );
        }

	add_class('network-extended');


	return 1;
}

sub process_ext() {
    my $reserved = 0;
    my ($rowindex, $index, $ifdescr, $ifspeed, $ifalias,
        $ifadminstatus, $ifoperstatus ) = @_;
    grep ( defined $_ && ( $_ = pretty_print $_),
      ( $index, $ifdescr, $ifspeed, $ifalias, $ifadminstatus, $ifoperstatus ) );

    # Compaq likes to put a null at the end....
    $ifdescr =~ s/\x00//g;
    $bits = 32;

    # Check for "reserved" interfaces, i.e. those we don't want to 
    # include in RTG
    foreach $resv (@reserved) {
        if ( $ifdescr =~ /$resv/ ) {
            $reserved = 1;
        }
    }
    if ($ifdescr) {
        if ( $system eq "Catalyst" ) {
            if ( $ifdescr =~ /(\d+)\/(\d+)/ ) {
                $catalystoid = ".1.3.6.1.4.1.9.5.1.4.1.1.4.".$1.".". $2;
		$SNMP_Session::suppress_warnings=2;
                @result = rtg_snmpget( "$communities{$router}\@$router", "$catalystoid" );
		$SNMP_Session::suppress_warnings=0;
                $ifalias = join ( ' ', @result );
            }
        }
        if($ifspeed == 0) {
                # if the speed is == 0 we're assuming somethign is wrong with the
                # agent as any network device should have a speed. we then assume
                # that it's a 100M interface as most things are these days.
                $ifspeed = 100000000;
        }
        if($ifspeed == 100) {
            # for some reason net-snmp on solaris 7 reports the ifspeed to be
            # 100 instead of 10000000. We assume that no real ethernet device
            # will have a speed of 100bps, so we change any 100 to 100M
            $ifspeed = 100000000;
        }

        if($system =~ /^Linux/ && $CPQ_LINUX_GIGABIT_BUG && $ifspeed == 10000000) {
            # for some reason netsnmp on compaq boxes with gigabit nics
            # when running in gigabit show up as 10M. This is bad.
            $ifspeed = 1000000000;
        }


        if ( $ifadminstatus == 1 && $ifoperstatus == 1 && $reserved == 0 ) {
            if ( !$DBOFF ) {
                $iid = &find_interface_id( $rid, $ifdescr, $ifalias, $ifspeed );
            }


            foreach $mib ( keys %mibs_of_interest_extif ) {
		# we want to make sure the device supports the requested stat, not all do
		my %oid_to_try = ();

		# only add it if the value is actually an oid. This will get rid
		# of the no_32_bit ones that we run into multicast stuff
		if($mibs_of_interest_extif{$mib} =~ /^\.\d/) {
			$oid_to_try{"$mibs_of_interest_extif{$mib}$index"} = $bits;
		} 

		if($SNMP_USE_64) {
			if(exists $mibs_of_interest_extif_64{$mib}) {
				$oid_to_try{"$mibs_of_interest_extif_64{$mib}$index"} = 64;
			}
		}

               foreach $mib_to_try (sort { $oid_to_try{$b} <=> $oid_to_try{$a} } keys %oid_to_try ) {
			$SNMP_Session::suppress_warnings=2;
			@result = rtg_snmpget ("$communities{$router}\@$router", "$mib_to_try");
			$SNMP_Session::suppress_warnings=0;
			if(defined $result[0] && $result[0] ne "") {
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
					create_table($mib,$rid)
				}

				last;
			}	
		}	
	}	
  }      
        else {
            if ($DEBUG) {
		$debug_line = "Ignoring $router $ifalias ($ifdescr) ";
                $debug_line .= "[admin = up] "     if $ifadminstatus == 1;
                $debug_line .= "[admin = down] "   if $ifadminstatus != 1;
                $debug_line .= "[oper = up] "      if $ifoperstatus == 1;
                $debug_line .= "[oper = down] "    if $ifoperstatus != 1;
                $debug_line .= "[reserved = yes] " if $reserved == 1;
                $debug_line .= "[reserved = no] "  if $reserved != 1;
		debug($debug_line);
            }
        }
    }
}


# have to end with a 1
1;

