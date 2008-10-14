########################################################################
# $Id: Liebert.pl,v 1.1 2008/01/19 03:22:14 btoneill Exp $
########################################################################
########################################################################
# Add in airport support
#
#
# Set name of Module, and the OID to check to run this module
#
$MODULENAME = "LiebertSiteNet";
$VERSION = '0.1';

@CLASSES=qw(temp);

# Module requires a subroutine called process_module_$MODULENAME to exist.
#
sub process_module_LiebertSiteNet($$$);

#
# Local sub-routines
#
sub add_powerware($$);

# We check for xupsIdentManufacturer.0
$MIB_TO_CHECK = ".1.3.6.1.4.1.476.1.2.1.1.1.1.1.0";


push(@main::statclasses,@CLASSES);

# Set this so it's in the main hash
$main::snmp_modules{$MODULENAME} = $MIB_TO_CHECK;


$main::table_map{'SiteNet-Temp'} = [ qw(envTemperature1F) ];
$main::table_options{'SiteNet-Temp'} = [ qw(factor=1 gauge=on units=F) ];
$main::table_class{'SiteNet-Temp'} = "temp";

#
# Local vars
#
%mibs_of_interest_sitenet= (
	"envTemperature1F" => ".1.3.6.1.4.1.476.1.2.1.1.1.7.1.2.0",
);

#$normal_powerware = [
#	[ 1, 3, 6, 1, 4, 1, 534, 1, 3, 4, 1, 1 ],	# xupsInputPhase
#	[ 1, 3, 6, 1, 4, 1, 534, 1, 3, 4, 1, 2 ],	# xupsInputVoltage
#	[ 1, 3, 6, 1, 4, 1, 534, 1, 3, 4, 1, 3 ],	# xupsInputCurrent
#	[ 1, 3, 6, 1, 4, 1, 534, 1, 3, 4, 1, 4 ],	# xupsInputWatts
#];

sub process_module_LiebertSiteNet($$$) {
    my ($router,$community,$sess) = @_;


    debug("$router supports Liebert SiteNet");

    return 1 if(has_class('temp'));

    add_standard_sitenet();
    add_class('temp');
    
#    $sess->map_table( $normal_powerware, \&process_powerware);

#    add_class('electrical');

    return 1;
}

sub add_standard_sitenet() {

    $ifdescr = "SiteNet Temperature";
    $ifalias = $ifdescr;
    $ifspeed = "150";
    $bits = 0;

    if ( !$DBOFF ) {
        $iid = &find_interface_id( $rid, $ifdescr, $ifalias, $ifspeed );
    }

    foreach $mib_to_try (keys %mibs_of_interest_sitenet) {
        next unless($mibs_of_interest_sitenet{$mib_to_try} =~ /\.0$/);

        $SNMP_Session::suppress_warnings=2;
        @result = rtg_snmpget ("$communities{$router}\@$router", "$mibs_of_interest_sitenet{$mib_to_try}");
        $SNMP_Session::suppress_warnings=0;
        if(defined $result[0]) {
            if($ifalias eq "") {
                $ifalias = $ifdescr;
            }
            print_target($router,
                "$mibs_of_interest_sitenet{$mib_to_try}",
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


# have to end with a 1
1;

