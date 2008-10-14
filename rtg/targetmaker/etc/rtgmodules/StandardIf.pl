########################################################################
# $Id: StandardIf.pl,v 1.1 2008/01/19 03:22:14 btoneill Exp $
########################################################################
# $Log: StandardIf.pl,v $
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
# Revision 1.3  2005/04/07 21:30:34  btoneill
#
# Fixed stuff
#
# Revision 1.2  2005/03/31 15:25:26  btoneill
#
# Lots and lots of bug fixes, and new features, and other stuff.
#
# Revision 1.1.1.1  2004/10/05 15:03:34  btoneill
# Initial import into CVS
#
# Revision 1.2  2004-09-30 12:28:31-05  btoneill
# Alot of changes...
#
# Revision 1.1  2004-09-27 11:51:48-05  btoneill
# Initial revision
#
# Revision 1.1  2004/07/08 19:33:24  btoneill
# Initial revision
#
#
########################################################################
# Add in standard interface stats
#
#
# Set name of Module, and the OID to check to run this module
#
$MODULENAME = "StandardIf";
$VERSION = '0.1';

@CLASSES=qw(network);

# Module requires a subroutine called process_module_$MODULENAME to exist.
#
sub process_module_StandardIf($$$);

#
# Local sub-routines
#
sub add_standard($$);
sub process_std();

# We check for interfaces.ifNumber.0
# (we used to check for ifIndex.1, but Extreme switches like
# to start at 1001, so it would fail)
#$MIB_TO_CHECK = ".1.3.6.1.2.1.2.2.1.1.1";

$MIB_TO_CHECK = ".1.3.6.1.2.1.2.1.0";


push(@main::statclasses,@CLASSES);

# Set this so it's in the main hash
$main::snmp_modules{$MODULENAME} = $MIB_TO_CHECK;

#
#
# because standard intface stuff isn't in a module, we add it in here
$main::table_map{'MIBII-netio'} = [ qw(ifInOctets ifOutOctets) ];
$main::table_options{'MIBII-netio'} = [ qw(factor=8 units=bits) ];
$main::table_class{'MIBII-netio'} = "network";

$main::table_map{'MIBII-netpkt'} = [ qw(ifInUcastPkts ifOutUcastPkts) ];
$main::table_options{'MIBII-netpkt'} = [ qw(factor=1 units=pkts/s) ];
$main::table_class{'MIBII-netpkt'} = "network";

#
# Local vars
#
# Set of standard MIB-II objects of interest
%mibs_of_interest_32 = (
    "ifInOctets"     => ".1.3.6.1.2.1.2.2.1.10.",
    "ifOutOctets"    => ".1.3.6.1.2.1.2.2.1.16.",
    "ifInUcastPkts"  => ".1.3.6.1.2.1.2.2.1.11.",
    "ifOutUcastPkts" => ".1.3.6.1.2.1.2.2.1.17.",
);

# Set of 64 bit objects, preferred where possible
%mibs_of_interest_64 = (
    "ifInOctets"     => ".1.3.6.1.2.1.31.1.1.1.6.",
    "ifOutOctets"    => ".1.3.6.1.2.1.31.1.1.1.10.",
    "ifInUcastPkts"  => ".1.3.6.1.2.1.31.1.1.1.7.",
    "ifOutUcastPkts" => ".1.3.6.1.2.1.31.1.1.1.11.",
);


$normal = [
    [ 1, 3, 6, 1, 2, 1, 2,  2, 1, 1 ],        # ifIndex
    [ 1, 3, 6, 1, 2, 1, 2,  2, 1, 2 ],        # ifDescr
    [ 1, 3, 6, 1, 2, 1, 2,  2, 1, 5 ],        # ifSpeed
    [ 1, 3, 6, 1, 2, 1, 31, 1, 1, 1, 18 ],    # ifAlias
    [ 1, 3, 6, 1, 2, 1, 2,  2, 1, 7 ],        # ifAdminStatus
    [ 1, 3, 6, 1, 2, 1, 2,  2, 1, 8 ]         # ifOperStatus
];

$catalyst = [
    [ 1, 3, 6, 1, 2, 1, 2,  2, 1, 1 ],             # ifIndex
    [ 1, 3, 6, 1, 2, 1, 31, 1, 1, 1, 1 ],          # ifXEntry.ifName
    [ 1, 3, 6, 1, 2, 1, 2,  2, 1, 5 ],             # ifSpeed
    [ 1, 3, 6, 1, 2, 1, 31, 1, 1, 1, 18 ],    	   # ifAlias
    [ 1, 3, 6, 1, 2, 1, 2,  2, 1, 7 ],             # ifAdminStatus
    [ 1, 3, 6, 1, 2, 1, 2,  2, 1, 8 ]              # ifOperStatus
];

    #[ 1, 3, 6, 1, 4, 1, 9,  5, 1, 4, 1, 1, 4 ],    # CiscoCatalystPortName


sub process_module_StandardIf($$$) {
	my ($router,$community,$sess) = @_;


	debug("$router supports Standard Interfaces");

	return 1 if(has_class('network'));
	
	# Sanity check bits
	$bits = getrouterbits($router);
       
	$bits = $defbits if ( ( $bits != 32 ) && ( $bits != 64 ) );
        if ( $bits == 64 ) { %mibs_of_interest = %mibs_of_interest_64 }
        else { %mibs_of_interest = %mibs_of_interest_32 }

	if ( !$DBOFF ) {
            $rid = &find_router_id($router);
        }

	if ( $system eq "Catalyst" ) {
            $sess->map_table( $catalyst, \&process_std );
        }
        else {
            $sess->map_table( $normal, \&process_std );
        }

	add_class('network');


	return 1;
}

sub process_std() {
    my $reserved = 0;
    my ($rowindex, $index, $ifdescr, $ifspeed, $ifalias,
        $ifadminstatus, $ifoperstatus ) = @_;


    grep ( defined $_ && ( $_ = pretty_print $_), 
        ( $index, $ifdescr, $ifspeed, $ifalias, $ifadminstatus, $ifoperstatus ) );

    # Compaq likes to stick a null at the end... who knows why.....
    $ifdescr =~ s/\x00//g;

    # Check for "reserved" interfaces, i.e. those we don't want to 
    # include in RTG
    foreach $resv (@reserved) {
        if ( $ifdescr =~ /$resv/ ) {
            $reserved = 1;
        }
    }

    #
    # This is a hack, for some reason iprb operstatus keeps
    # going up and down on solaris x86 boxes with net-snmp
    # this hack will go away once the net-snmp problem is fixed
    #
    if($ifdescr =~ /^iprb/) {
        if($ifadminstatus == 1) {
            $ifoperstatus = 1;
        }
    }


    if ($ifdescr) {
        if ( $system eq "Catalyst" ) {
            if ( $ifdescr =~ /(\d+)\/(\d+)/ ) {
                $catalystoid = ".1.3.6.1.4.1.9.5.1.4.1.1.4.".$1.".". $2;
                @result = rtg_snmpget( "$communities{$router}\@$router", "$catalystoid" );
                $ifalias = join ( ' ', @result );
            }
        }

        if($ifspeed == 0) {
            # if the speed is == 0 we're assuming somethign is wrong with the
            # agent as any network device should have a speed. we then assume
            # that it's a 100M interface as most things are these days.
            # ....
            # Unless, it's a vlan, so we check the description for the vlan
            # entry, then we set it really really high (100G for now)....
            #
            if($ifdescr =~ /vlan/i) {
                $ifspeed = 100000000000;
            } else {
                $ifspeed = 100000000;
            }
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
                       
            if ($ifspeed ne "") {
                $ifspeed *= $interval;
                int $ifspeed;
            }

            $ifspeed2 = $ifspeed;

            foreach $mib ( keys %mibs_of_interest ) {

                # we want to make sure the device supports the requested stat, not all do
                # more ugliness. This time to check to see if there is a 64 bit
                # counter available for this stat, if there is, we use it
                my %oid_to_try = ();
                $oid_to_try{"$mibs_of_interest{$mib}$index"} = $bits;

                if($SNMP_USE_64) {
                    $oid_to_try{"$mibs_of_interest_64{$mib}$index"} = 64;
                }

                foreach $mib_to_try (sort { $oid_to_try{$b} <=> $oid_to_try{$a} } keys %oid_to_try ) {

                    $SNMP_Session::suppress_warnings=2;
                    @result = rtg_snmpget ("$communities{$router}\@$router", "$mib_to_try");
                    $SNMP_Session::suppress_warnings=0;

                    if(defined $result[0]) {

                        #
                        # if we get a value back for the 64 bit counter, and it's
                        # 0, we ignore it. This is to fix some bug we have with
                        # some of our cisco routers that return values in 32 bit
                        # but not 64 bit.
                        #
                        if($CISCO_64_BUG == 1 && $result[0] == 0 && $oid_to_try{$mib_to_try} == 64) {
                            next;
                        }

                        if($mib =~ /Octet/) { $ifspeed = $ifspeed2 / 8; }
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

                        # we create the table if it doesn't exist
                        if ( !$DBOFF ) {	
                                create_table($mib,$rid);
                        }

                        # we found one, so we skip the rest
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
    } else {
	print "Eeeek!!!!! ( $rowindex, $index, $ifdescr, $ifspeed, $ifalias, $ifadminstatus, $ifoperstatus ) )\n";
    }


}


# have to end with a 1
1;

