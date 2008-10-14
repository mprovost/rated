########################################################################
# $Id: ExtremeSNMP.pl,v 1.1 2008/01/19 03:22:14 btoneill Exp $
########################################################################
# $Log: ExtremeSNMP.pl,v $
# Revision 1.1  2008/01/19 03:22:14  btoneill
#
# Targetmaker 0.9 added to the default RTG distribution
#
# Revision 1.3  2005/10/17 15:15:09  btoneill
# Lots and Lots of changes
#
# Revision 1.2  2005/03/31 15:25:25  btoneill
#
# Lots and lots of bug fixes, and new features, and other stuff.
#
# Revision 1.1.1.1  2004/10/05 15:03:34  btoneill
# Initial import into CVS
#
# Revision 1.2  2004-09-30 12:28:04-05  btoneill
# Fixed various things...
#
# Revision 1.1  2004-09-27 11:51:48-05  btoneill
# Initial revision
#
# Revision 1.2  2004/07/08 19:31:44  btoneill
# First version that works correctly
#
# Revision 1.1  2004/07/07 15:27:25  btoneill
# Initial revision
#
#
########################################################################
#
$MODULENAME = "ExtremeSNMP";
$VERSION = '0.1';

#
# List of the types of stat classes that are gathered. This is used
# so that multiple modules don't collect the same classes of stats
# ie. the host-resources mib does cpu and disk, but we don't want those
# if we can get them from here
#
@CLASSES=qw(cpu temperature);


# Module requires a subroutine called process_module_$MODULENAME to exist.
#
sub process_module_ExtremeSNMP($$$);

#
# Local sub-routines
#
sub add_def_extremesnmp($$);

# We check for enterprises.extremenetworks.extremeAgent.extremeSystem.extremeSystemCommon.extremeSystemID.0
$MIB_TO_CHECK = ".1.3.6.1.4.1.1916.1.1.1.16.0";


# Set this so it's in the main hash
$main::snmp_modules{$MODULENAME} = $MIB_TO_CHECK;

# add our classes to the main list of classes
push(@main::statclasses,@CLASSES);


#
# map tables to make sense
$main::table_map{'ExtremeSNMP-cpu'}	= [ qw(extremeCpuAggregateUtilization) ];
$main::table_options{'ExtremeSNMP-cpu'} = [ qw(scaley=on gauge=on units=CPU%) ];
$main::table_class{'ExtremeSNMP-cpu'} = "cpu";

$main::table_map{'ExtremeSNMP-temp'} = [ qw(extremeCurrentTemperature) ];
$main::table_options{'ExtremeSNMP-temp'} = [ qw(gauge=on units=Deg) ];
$main::table_class{'ExtremeSNMP-temp'} = "temp";

#
# Local vars
#
%mibs_of_interest_extremesnmp = (
	# load average
	"extremeCpuAggregateUtilization"	=> ".1.3.6.1.4.1.1916.1.1.1.28.",
	"extremeCurrentTemperature"		=> ".1.3.6.1.4.1.1916.1.1.1.8.",
);


sub process_module_ExtremeSNMP($$$) {
	my ($router,$community,$sess) = @_;

	debug("$router supports ExtremeSNMP MIBs");

	add_def_extremesnmp($router,$community);

	return 1;
}


sub add_def_extremesnmp($$) {
	my ($router,$comm) = @_;

	my %foundclasses=();

	foreach $sstat (keys %mibs_of_interest_extremesnmp) {
		# only add by default ss mem stats
		if($sstat =~ /(Temperature)|(CpuAgg)/) {

			# we assume all the starting rows are 0
			$row = 0;



			if($sstat =~ /CpuAgg/) { 
				my $thisclass = "cpu";
				next if(has_class($thisclass));
				next unless(hasoid($comm,$router,$mibs_of_interest_extremesnmp{$sstat}."$row"));
				$ifdescr = "CPU Stats"; 
				$ifalias = "CPU Usage % for $router";
				$ifspeed = 100;
				$foundclasses{$thisclass} = 1;
			}
			elsif($sstat =~ /Temperature/) { 
				my $thisclass = "temperature";
				next if(has_class($thisclass));
				next unless(hasoid($comm,$router,$mibs_of_interest_extremesnmp{$sstat}."$row"));
				$ifdescr = "Temp Stats"; 
				$ifalias = "Temperature for $router";
				$ifspeed = "0";
				$foundclasses{$thisclass} = 1;
			}

			if ( !$DBOFF ) {
				$rid = find_router_id($router);
				$iid = find_interface_id( $rid, $ifdescr, $ifalias, $ifspeed );
			}
			else { $iid = 999; $rid=999; }
          	
			if ( !$DBOFF ) {	
				create_table($mib,$rid);
			}

			if($ifspeed == 0) { $ifspeed = "nomaxspeed"; }

			print_target($router,
                			"$mibs_of_interest_extremesnmp{$sstat}$row",
	                		0,
					$comm,
					"$sstat" . "_$rid",
					$iid,
					$ifspeed,
					"$ifalias ($ifdescr)");
		}


	}
	#
	# add any of the classes that we found
	foreach(keys %foundclasses) { add_class($_); }
}

# have to end with a 1
1;

