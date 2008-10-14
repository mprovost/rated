########################################################################
# $Id: CPQSNMP.pl,v 1.1 2008/01/19 03:22:14 btoneill Exp $
########################################################################
# $Log: CPQSNMP.pl,v $
# Revision 1.1  2008/01/19 03:22:14  btoneill
#
# Targetmaker 0.9 added to the default RTG distribution
#
# Revision 1.6  2005/10/17 15:15:09  btoneill
# Lots and Lots of changes
#
# Revision 1.5  2005/03/31 15:25:25  btoneill
#
# Lots and lots of bug fixes, and new features, and other stuff.
#
# Revision 1.4  2004/11/02 17:12:58  btoneill
# Lots of fixes. this is what should be the 0.8 release
#
# Revision 1.2  2004/10/13 20:18:03  btoneill
# *** empty log message ***
#
# Revision 1.1.1.1  2004/10/05 15:03:34  btoneill
# Initial import into CVS
#
# Revision 1.1  2004-09-27 11:51:47-05  btoneill
# Initial revision
#
# Revision 1.1  2004/06/24 15:41:19  btoneill
# Initial revision
#
#
########################################################################
# Add stats for Compaq Insight Manger boxes
# Namely we add in CPU, Memory, Diskspace, and temperature
#
#
# Set name of Module, and the OID to check to run this module
#
$MODULENAME = "CPQSNMP";
$VERSION = '0.1';

#
# List of the types of stat classes that are gathered. This is used
# so that multiple modules don't collect the same classes of stats
# ie. the host-resources mib does cpu and disk, but we don't want those
# if we can get them from here
#
@CLASSES=qw(disk cpu memory temperature);

# Module requires a subroutine called process_module_$MODULENAME to exist.
#
sub process_module_CPQSNMP($$$);

#
# Local sub-routines
#
sub add_def_cpqsnmp($$);
sub disk_process_cpqsnmp();
sub cpu_process_cpqsnmp();
sub temp_process_cpqsnmp();

# We check for enterprises.compaq.cpqStdEquipment.cpqSeMibRev.cpqSeMibRevMajor.0
$MIB_TO_CHECK = ".1.3.6.1.4.1.232.1.1.1.0";


# Set this so it's in the main hash
$main::snmp_modules{$MODULENAME} = $MIB_TO_CHECK;

#
push(@main::statclasses,@CLASSES);

#
# map tables to make sense
$main::table_map{'CPQSNMP-cpu'} = [ qw(cpqHoCpuUtilUnitMin cpqHoCpuUtilUnitFiveMin cpqHoCpuUtilUnitThirtyMin cpqHoCpuUtilUnitHour) ];
$main::table_options{'CPQSNMP-cpu'} = [ qw(gauge=on units=% scaley=on borderb=90) ];
$main::table_class{'CPQSNMP-cpu'} = "cpu";

$main::table_map{'CPQSNMP-memphy'} = [ qw(cpqHoPhysicalMemorySize cpqHoPhysicalMemoryFree) ];
$main::table_options{'CPQSNMP-memphy'} = [ qw(gauge=on units=Bytes factor=1000000) ];
$main::table_class{'CPQSNMP-memphy'} = "memory";

$main::table_map{'CPQSNMP-mempag'} = [ qw(cpqHoPagingMemorySize cpqHoPagingMemoryFree) ];
$main::table_options{'CPQSNMP-mempag'} = [ qw(gauge=on units=Bytes factor=1000000) ];
$main::table_class{'CPQSNMP-mempag'} = "memory";

$main::table_map{'CPQSNMP-dskspc'} = [ qw(cpqHoFileSysSpaceTotal cpqHoFileSysSpaceUsed) ];
$main::table_options{'CPQSNMP-dskspc'} = [ qw(gauge=on units=Bytes factor=1000000) ];
$main::table_class{'CPQSNMP-dskspc'} = "disk";

$main::table_map{'CPQSNMP-dskunits'} = [ qw(cpqHoFileSysAllocUnitsTotal cpqHoFileSysAllocUnitsUsed) ];
$main::table_options{'CPQSNMP-dskunits'} = [ qw(gauge=on units=Files) ];
$main::table_class{'CPQSNMP-dskunits'} = "disk";

$main::table_map{'CPQSNMP-dskper'} = [ qw(cpqHoFileSysPercentSpaceUsed) ];
$main::table_options{'CPQSNMP-dskper'} = [ qw(gauge=on units=% filled=on) ];
$main::table_class{'CPQSNMP-dskper'} = "disk";

$main::table_map{'CPQSNMP-temp'} = [ qw(cpqHeTemperatureCelsius) ];
$main::table_options{'CPQSNMP-temp'} = [ qw(gauge=on units=DegC filled=on) ];
$main::table_class{'CPQSNMP-temp'} = "temp";

#
# Local vars
#
%mibs_of_interest_cpqsnmp = (
	# cpu %
	"cpqHoCpuUtilUnitMin"		=> ".1.3.6.1.4.1.232.11.2.3.1.1.2.",
	"cpqHoCpuUtilUnitFiveMin"	=> ".1.3.6.1.4.1.232.11.2.3.1.1.3.",
	"cpqHoCpuUtilUnitThirtyMin"	=> ".1.3.6.1.4.1.232.11.2.3.1.1.4.",
	"cpqHoCpuUtilUnitHour"		=> ".1.3.6.1.4.1.232.11.2.3.1.1.5.",
	
	
	# memory usage
	"cpqHoPhysicalMemorySize"	=> ".1.3.6.1.4.1.232.11.2.13.1.",
	"cpqHoPhysicalMemoryFree"	=> ".1.3.6.1.4.1.232.11.2.13.2.",
	"cpqHoPagingMemorySize"		=> ".1.3.6.1.4.1.232.11.2.13.3.",
	"cpqHoPagingMemoryFree"		=> ".1.3.6.1.4.1.232.11.2.13.4.",
	
	# disk usage
	"cpqHoFileSysSpaceTotal"	=> ".1.3.6.1.4.1.232.11.2.4.1.1.3.",
	"cpqHoFileSysSpaceUsed"		=> ".1.3.6.1.4.1.232.11.2.4.1.1.4.",
	"cpqHoFileSysPercentSpaceUsed"	=> ".1.3.6.1.4.1.232.11.2.4.1.1.5.",
	"cpqHoFileSysAllocUnitsTotal"	=> ".1.3.6.1.4.1.232.11.2.4.1.1.6.",
	"cpqHoFileSysAllocUnitsUsed"	=> ".1.3.6.1.4.1.232.11.2.4.1.1.7.",

	# Temperature
	"cpqHeTemperatureCelsius"	=> ".1.3.6.1.4.1.232.6.2.6.8.1.4.",
);

@ignore_disks = qw(cdrom floppy usb proc pts automount /sys rpc_pipefs /dev/shm '/dev/vx on tmpfs');

$cpq_snmpd_cpu = [
	[ 1, 3, 6, 1, 4, 1, 232, 11, 2, 3, 1, 1, 1 ], 	# cpqHoCpuUtilUnitIndex
	[ 1, 3, 6, 1, 4, 1, 232, 11, 2, 3, 1, 1, 2 ], 	# cpqHoCpuUtilUnitMin
	[ 1, 3, 6, 1, 4, 1, 232, 11, 2, 3, 1, 1, 3 ], 	# cpqHoCpuUtilUnitFiveMin
	[ 1, 3, 6, 1, 4, 1, 232, 11, 2, 3, 1, 1, 4 ], 	# cpqHoCpuUtilUnitThirtyMin
	[ 1, 3, 6, 1, 4, 1, 232, 11, 2, 3, 1, 1, 5 ], 	# cpqHoCpuUtilUnitHour
	[ 1, 3, 6, 1, 4, 1, 232, 1, 2, 2, 1, 1, 3 ], 	# cpqSeCpuName
	[ 1, 3, 6, 1, 4, 1, 232, 1, 2, 2, 1, 1, 4 ] 	# cpqSeCpuSpeed
	
];

$cpq_snmpd_disk = [
	[ 1, 3, 6, 1, 4, 1, 232, 11, 2, 4, 1, 1, 1 ],	# cpqHoFileSysIndex
	[ 1, 3, 6, 1, 4, 1, 232, 11, 2, 4, 1, 1, 2 ],	# cpqHoFileSysDesc
	[ 1, 3, 6, 1, 4, 1, 232, 11, 2, 4, 1, 1, 3 ],	# cpqHoFileSysSpaceTotal
	[ 1, 3, 6, 1, 4, 1, 232, 11, 2, 4, 1, 1, 4 ],	# cpqHoFileSysSpaceUsed
	[ 1, 3, 6, 1, 4, 1, 232, 11, 2, 4, 1, 1, 5 ],	# cpqHoFileSysPercentSpaceUsed
	[ 1, 3, 6, 1, 4, 1, 232, 11, 2, 4, 1, 1, 6 ],	# cpqHoFileSysAllocUnitsTotal
	[ 1, 3, 6, 1, 4, 1, 232, 11, 2, 4, 1, 1, 7 ]	# cpqHoFileSysAllocUnitsUsed
];

$cpq_snmpd_temp = [
	[ 1, 3, 6, 1, 4, 1, 232, 6, 2, 6, 8, 1, 1 ],	# cpqHeTemperatureChassis
	[ 1, 3, 6, 1, 4, 1, 232, 6, 2, 6, 8, 1, 2 ],	# cpqHeTemperatureIndex
	[ 1, 3, 6, 1, 4, 1, 232, 6, 2, 6, 8, 1, 3 ],	# cpqHeTemperatureLocale
	[ 1, 3, 6, 1, 4, 1, 232, 6, 2, 6, 8, 1, 4 ],	# cpqHeTemperatureCelsius
];

# This comes from the CPQHLTH-MIB, the value returned from SNMP is an enum, so we need to 
# map it here
@tempLocaleArray=( "blank",	# isn't in the mib, but makes making the array easier
		"other",	# enum 1
		"unknown",	# enum 2
		"system",	# enum 3
		"systemBoard",	# enum 4
		"ioBoard",	# enum 5
		"cpu",		# enum 6
		"memory",	# enum 7
		"storage",	# enum 8
		"removableMedia",	# enum 9
		"powerSupply",	# enum 10
		"ambient",	# enum 11
		"chassis",	# enum 12
		"bridgeCard"	# enum 13
);

#
# For CPQ hypethread bug
#
$last_cpq_cpu_speed = "";



sub process_module_CPQSNMP($$$) {
	my ($router,$community,$sess) = @_;

	debug("$router supports Compaq Insight Manager MIBs");
        # For CPQ hypethread bug
        $last_cpq_cpu_speed = "";

	add_def_cpqsnmp($router,$community);
	$sess->map_table( $cpq_snmpd_cpu, \&cpu_process_cpqsnmp) unless(has_class('cpu'));
	$sess->map_table( $cpq_snmpd_disk, \&disk_process_cpqsnmp) unless(has_class('disk'));
	$sess->map_table( $cpq_snmpd_temp, \&temp_process_cpqsnmp) unless(has_class('temperature'));

	return 1;
}


sub disk_process_cpqsnmp() {
    my $reserved = 0;
    my ($rowindex, $index, $dskdescr, $dsksize, $dskused, $diskpercent, $unitsize, $unitused) = @_;

    $bits = 0;

    grep ( defined $_ && ( $_ = pretty_print $_),
	( $index, $dskdescr, $dsksize, $dskused, $diskpercent, $unitsize, $unitused) );

    $dskdescr =~ /^(.+:)/;
    $dskalias = $1;

    foreach $resv (@ignore_disks) {
        if ( $dskdescr =~ /$resv/ ) {
            $reserved = 1;
        }
    }

    if ($dskdescr && !$reserved) {

	    add_class('disk');
            if ( !$DBOFF ) {
                $iid = &find_interface_id( $rid, $dskdescr, $dskalias, $dsksize * 1000000);
            }
            foreach $mib ( keys %mibs_of_interest_cpqsnmp ) {
		next unless($mib =~ /FileSys/);
		
		if ( !$DBOFF ) {	
			create_table($mib,$rid);
		}


		$ifspeed = "nomaxspeed";
		print_target($router,
			"$mibs_of_interest_cpqsnmp{$mib}$index",
			$bits,
			$communities{$router},
			"$mib" . "_$rid",
			$iid,
			$ifspeed,
			"$dskalias ($dskdescr)");
            }
        }
    
}

sub temp_process_cpqsnmp() {
    my $reserved = 0;
    my ($rowindex, $chassis, $index, $temploc, $tempcel ) = @_;

    $tempsize=0;
    $bits=0;

    grep ( defined $_ && ( $_ = pretty_print $_),
	($chassis, $index, $temploc, $tempcel ) );


    $tempdescr="Temperature Sensor #$chassis/$index ($tempLocaleArray[$temploc])";
    $tempalias="Temp #$chassis/$index ($tempLocaleArray[$temploc])";

    # isn't active if temp is -1
    if ($tempcel >= 0) {
	
	    add_class('temperature');
            if ( !$DBOFF ) {
                $iid = find_interface_id( $rid, $tempdescr, $tempalias, $tempsize );
            }
            foreach $mib ( keys %mibs_of_interest_cpqsnmp ) {
		next unless($mib =~ /HeTemperature/);
		
		if ( !$DBOFF ) {	
			create_table($mib,$rid);
		}
#
		
		$ifspeed = "nomaxspeed"; 
		print_target($router,
				"$mibs_of_interest_cpqsnmp{$mib}$chassis.$index",
				$bits,
				$communities{$router},
				"$mib" . "_$rid",
				$iid,
				$ifspeed,
				"$tempalias ($tempdescr)");
            }
        }

}

sub cpu_process_cpqsnmp() {
    my $reserved = 0;
    my ($rowindex, $index, $onemin, $fivemin, $thirtymin, $hour, $cpuname, $cpuspeed ) = @_;

    # max cpu % is 100
    $cpusize=100;
    $bits=0;

    grep ( defined $_ && ( $_ = pretty_print $_),
	($rowindex, $index, $onemin, $fivemin, $thirtymin, $hour, $cpuname, $cpuspeed ) );

    #
    # hyper threaded boxes don't give the speed and
    # name for their 2nd cpu they show themselves to
    # the OS as, so we have this hack
    #
    if($cpuname eq "") {
        $cpuname = "HyperThreaded";
        $cpudescr="$cpuname"." CPU #$index";
    } else {
        $cpudescr="$cpuname $cpuspeed"."Mhz CPU #$index";
    }
    $cpualias="CPU #$index";

    # if cpu isn't active, $onemin is -1
    if ($onemin >= 0) {

	    add_class('cpu');
            if ( !$DBOFF ) {
                $iid = find_interface_id( $rid, $cpudescr, $cpualias, $cpusize );
            }

            #
            # hyper threaded boxes don't give the speed and
            # name for their 2nd cpu they show themselves to
            # the OS as, so we have this hack
            #
            if($cpuspeed eq "") {
                $cpuspeed = $last_cpq_cpu_speed;
            } else {
                $last_cpq_cpu_speed = $cpuspeed;
            }
    
            if($cpuname eq "") {
                $cpuname = "HyperThreaded";
            }


            foreach $mib ( keys %mibs_of_interest_cpqsnmp ) {
		next unless($mib =~ /CpuUtil/);
		
		if ( !$DBOFF ) {	
			create_table($mib,$rid);
		}
#

		$ifspeed = 100; # CPU won't go over 100%
		print_target($router,
				"$mibs_of_interest_cpqsnmp{$mib}$index",
				$bits,
				$communities{$router},
				"$mib" . "_$rid",
				$iid,
				$ifspeed,
				"$cpualias ($cpudescr)");
            }
        }
}

sub add_def_cpqsnmp($$) {
	my ($router,$comm) = @_;

	my $found_memory = 0;

	foreach $sstat (keys %mibs_of_interest_cpqsnmp) {
		# only add by default ss mem stats
		if($sstat =~ /Memory/) {

			# do we already have a memory class found?
			next if(has_class('memory'));

			next unless(hasoid($comm,$router,$mibs_of_interest_cpqsnmp{$sstat}."0"));

			$found_memory = 1;

			$ifdescr = "Memory Stats"; 
			$ifalias = "Memory Statistics for $router";
			$ifspeed = 0;

			if ( !$DBOFF ) {
				$rid = find_router_id($router);
				$iid = find_interface_id( $rid, $ifdescr, $ifalias, $ifspeed );
			}
			else { $iid = 999; $rid=999; }
          	
			if ( !$DBOFF ) {	
				create_table($sstat,$rid);
			}

			$ifspeed = "nomaxspeed";
			print_target($router,
                			"$mibs_of_interest_cpqsnmp{$sstat}0",
	                		0,
					$comm,
					"$sstat" . "_$rid",
					$iid,
					$ifspeed,
					"$ifalias ($ifdescr)");

		}
	}

	if($found_memory > 0) { add_class('memory'); }
    
}

# have to end with a 1
1;

