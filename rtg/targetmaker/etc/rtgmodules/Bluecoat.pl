########################################################################
# $Id: Bluecoat.pl,v 1.1 2008/01/19 03:22:14 btoneill Exp $
########################################################################
# $Log: Bluecoat.pl,v $
# Revision 1.1  2008/01/19 03:22:14  btoneill
#
# Targetmaker 0.9 added to the default RTG distribution
#
# Revision 1.3  2005/10/17 15:15:09  btoneill
# Lots and Lots of changes
#
# Revision 1.2  2005/03/31 15:25:26  btoneill
#
# Lots and lots of bug fixes, and new features, and other stuff.
#
# Revision 1.1.1.1  2004/10/05 15:03:34  btoneill
# Initial import into CVS
#
# Revision 1.2  2004-09-30 12:28:20-05  btoneill
# Various bug fixes
#
# Revision 1.1  2004-09-27 11:51:48-05  btoneill
# Initial revision
#
# Revision 1.1  2004/06/24 15:42:51  btoneill
# Initial revision
#
#
########################################################################
# Add in stats for Bluecoat agents, namely cpu, memory
# disk usage, and disk i/o (if supported)
#
#
# Set name of Module, and the OID to check to run this module
#
$MODULENAME = "Bluecoat";
$VERSION = '0.1';

#
# List of the types of stat classes that are gathered. This is used
# so that multiple modules don't collect the same classes of stats
# ie. the host-resources mib does cpu and disk, but we don't want those
# if we can get them from here
#
@CLASSES=qw(cpu disk bcsensor);


# Module requires a subroutine called process_module_$MODULENAME to exist.
#
sub process_module_Bluecoat($$$);

#
# Local sub-routines
#
sub add_def_bluecoat($$);

# We check for BLUECOAT-HOST-RESOURCES-MIB::bchrSerial.0
$MIB_TO_CHECK = ".1.3.6.1.4.1.3417.2.9.1.1.0";


# Set this so it's in the main hash
$main::snmp_modules{$MODULENAME} = $MIB_TO_CHECK;

# add our classes to the main list of classes
push(@main::statclasses,@CLASSES);


#
# map tables to make sense
$main::table_map{'Bluecoat-cpu'}	= [ qw(deviceUsagePercentCpu) ];
$main::table_options{'Bluecoat-cpu'} = [ qw(scaley=on gauge=on units=%) ];
$main::table_class{'Bluecoat-cpu'} = "cpu";

$main::table_map{'Bluecoat-diskper'} = [ qw(deviceUsagePercentDisk) ];
$main::table_options{'Bluecoat-diskper'} = [ qw(scaley=on gauge=on units=%) ];
$main::table_class{'Bluecoat-diskper'} = "disk";

#
# Local vars
#
%mibs_of_interest_bluecoat = (
	"deviceUsagePercentCpu"	=> ".1.3.6.1.4.1.3417.2.4.1.1.1.4.1", 
	"deviceUsagePercentDisk" => ".1.3.6.1.4.1.3417.2.4.1.1.1.4.2",

        "deviceSensorTrapEnabled" => ".1.3.6.1.4.1.3417.2.1.1.1.1.1.2",
        "deviceSensorUnits" => ".1.3.6.1.4.1.3417.2.1.1.1.1.1.3",
        "deviceSensorValue" => ".1.3.6.1.4.1.3417.2.1.1.1.1.1.5",
        "deviceSensorName" => ".1.3.6.1.4.1.3417.2.1.1.1.1.1.9",
);

$bluecoat_sensor = [
        [ 1, 3, 6, 1, 4, 1, 3417, 2, 1, 1, 1, 1, 1, 2 ],      # deviceSensorTrapEnabled
        [ 1, 3, 6, 1, 4, 1, 3417, 2, 1, 1, 1, 1, 1, 3 ],      # deviceSensorUnits
        [ 1, 3, 6, 1, 4, 1, 3417, 2, 1, 1, 1, 1, 1, 5 ],      # deviceSensorValue
        [ 1, 3, 6, 1, 4, 1, 3417, 2, 1, 1, 1, 1, 1, 9 ],      # deviceSensorName
];

sub process_module_Bluecoat($$$) {
	my ($router,$community,$sess) = @_;

	debug("$router supports BlueCoat MIBs");

#	add_def_bluecoat($router,$community);
#	$sess->map_table( $ucd_snmpd_disk, \&process_bluecoat_sensor) unless(has_class('bcsensor'));
#	$sess->map_table( $ucd_snmpd_disk_io, \&disk_process_bluecoat_io) unless(has_class('diskio'));

	return 1;
}


sub disk_process_bluecoat() {
    my $reserved = 0;
    my ($rowindex, $index, $dskdescr, $dskalias, $dsksize, $dskfree, $diskused, $diskpercent, $diskipercent ) = @_;

    $bits = 0;
    grep ( defined $_ && ( $_ = pretty_print $_),
	( $index, $dskdescr, $dskalias, $dsksize, $dskfree, $diskused, $diskpercent, $diskipercent ) );

    if ($dskdescr) {
	    add_class('disk');
            if ( !$DBOFF ) {
                $iid = &find_interface_id( $rid, $dskdescr, $dskalias, $dsksize * 1000 );
            }
            foreach $mib ( keys %mibs_of_interest_bluecoat) {
		next unless($mib =~ /^dsk/);
		
		if ( !$DBOFF ) {	
			create_table($mib,$rid);
		}

		$SNMP_Session::suppress_warnings=2;
		@result = rtg_snmpget ("$communities{$router}\@$router", "$mibs_of_interest_bluecoat{$mib}$index");
		$SNMP_Session::suppress_warnings=0;

		$ifspeed = "nomaxspeed";
		if(defined $result[0]) {
			print_target($router,
				"$mibs_of_interest_bluecoat{$mib}$index",
				$bits,
				$communities{$router},
				"$mib" . "_$rid",
				$iid,
				$ifspeed,
				"$dskalias ($dskdescr)");
		}
            }
        }
}

sub disk_process_bluecoat_io() {
    my $reserved = 0;
    my ($rowindex, $index, $iodescr, $iobytesread, $iobyteswritten, $ioreads, $iowrites ) = @_;

    $iosize=0;
    $bits=32;

	

    grep ( defined $_ && ( $_ = pretty_print $_),
	($index, $iodescr, $iobytesread, $iobyteswritten, $ioreads, $iowrites ) );

    $ioalias="Raw disk device $iodescr";

    if ($iodescr) {
	    add_class('diskio');
            if ( !$DBOFF ) {
                $iid = find_interface_id( $rid, $iodescr, $ioalias, $iosize );
            }
            foreach $mib ( keys %mibs_of_interest_bluecoat) {
		next unless($mib =~ /^diskIO/);
		
		if ( !$DBOFF ) {	
			create_table($mib,$rid);
		}
#
		# we want to make sure the device supports the requested stat, not all do
		$SNMP_Session::suppress_warnings=2;
		@result = rtg_snmpget ("$communities{$router}\@$router", "$mibs_of_interest_bluecoat{$mib}$index");
		$SNMP_Session::suppress_warnings=0;

		$ifspeed = "nomaxspeed"; # no max disk io values
		if(defined $result[0]) {
			print_target($router,
					"$mibs_of_interest_bluecoat{$mib}$index",
					$bits,
					$communities{$router},
					"$mib" . "_$rid",
					$iid,
					$ifspeed,
					"$ioalias ($iodescr)");
		}
            }
        }
}

sub add_def_bluecoat($$) {
	my ($router,$comm) = @_;

	my %foundclasses=();

	foreach $sstat (keys %mibs_of_interest_bluecoat) {
		# only add by default ss mem stats
		if($sstat =~ /(^ss)|(^mem)|(^la)/) {

			# we assume all the starting rows are 0
			$row = 0;



			if($sstat =~ /^ss/) { 
				my $thisclass = "cpu";
				next if(has_class($thisclass));
				next unless(hasoid($comm,$router,$mibs_of_interest_bluecoat{$sstat}."$row"));
				$ifdescr = "CPU Stats"; 
				$ifalias = "CPU Usage % for $router";
				$ifspeed = 100;
				$foundclasses{$thisclass} = 1;
			}
			elsif($sstat =~ /^mem/) { 
				my $thisclass = "memory";
				next if(has_class($thisclass));
				next unless(hasoid($comm,$router,$mibs_of_interest_bluecoat{$sstat}."$row"));
				$ifdescr = "Memory Stats"; 
				$ifalias = "Memory Statistics for $router";
				$ifspeed = 0;
				$foundclasses{$thisclass} = 1;
			}
			elsif($sstat =~ /^laLoad/) {
				my $thisclass = "loadavg";
				$row="";
				next if(has_class($thisclass));
				next unless(hasoid($comm,$router,$mibs_of_interest_bluecoat{$sstat}."$row"));
				$ifdescr = "Load Average";
				$ifalias = "Load Average x100";
				$ifspeed = 0;
				$foundclasses{$thisclass} = 1;
			}

			if ( !$DBOFF ) {
				$rid = find_router_id($router);
				$iid = find_interface_id( $rid, $ifdescr, $ifalias, $ifspeed );
			}
			else { $iid = 999; $rid=999; }
          	
			if ( !$DBOFF ) {	
				create_table($sstat,$rid);
			}

			$ifspeed = "nomaxvalue"; #there is no max la
			print_target($router,
                			"$mibs_of_interest_bluecoat{$sstat}$row",
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

