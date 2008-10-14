########################################################################
# $Id: NetSNMP.pl,v 1.1 2008/01/19 03:22:14 btoneill Exp $
########################################################################
# $Log: NetSNMP.pl,v $
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
# Add in stats for NetSNMP agents, namely load, cpu, memory
# disk usage, and disk i/o (if supported)
#
#
# Set name of Module, and the OID to check to run this module
#
$MODULENAME = "NetSNMP";
$VERSION = '0.1';

#
# List of the types of stat classes that are gathered. This is used
# so that multiple modules don't collect the same classes of stats
# ie. the host-resources mib does cpu and disk, but we don't want those
# if we can get them from here
#
@CLASSES=qw(disk diskio cpu memory loadavg);


# Module requires a subroutine called process_module_$MODULENAME to exist.
#
sub process_module_NetSNMP($$$);

#
# Local sub-routines
#
sub add_def_netsnmp($$);
sub disk_process_netsnmp();
sub disk_process_netsnmp_io();

# We check for enterprises.ucdavis.version.versionIndex.0
$MIB_TO_CHECK = ".1.3.6.1.4.1.2021.100.1.0";


# Set this so it's in the main hash
$main::snmp_modules{$MODULENAME} = $MIB_TO_CHECK;

# add our classes to the main list of classes
push(@main::statclasses,@CLASSES);


#
# map tables to make sense
$main::table_map{'NetSNMP-loadavg'} = [ qw(laLoadInt1min laLoadInt5min laLoadInt10min) ];
$main::table_options{'NetSNMP-loadavg'} = [ qw(gauge=on units=Load) ];
$main::table_class{'NetSNMP-loadavg'} = "loadavg";

$main::table_map{'NetSNMP-cpu'}	= [ qw(ssCpuIdle ssCpuUser ssCpuSystem) ];
$main::table_options{'NetSNMP-cpu'} = [ qw(scaley=on gauge=on units=CPU%) ];
$main::table_class{'NetSNMP-cpu'} = "cpu";

$main::table_map{'NetSNMP-swap'} = [ qw(memTotalFree memTotalSwap memAvailSwap) ];
$main::table_options{'NetSNMP-swap'} = [ qw(gauge=on units=Bytes factor=1000) ];
$main::table_class{'NetSNMP-swap'} = "memory";

$main::table_map{'NetSNMP-real'} = [ qw(memTotalReal memAvailReal) ];
$main::table_options{'NetSNMP-real'} = [ qw(gauge=on units=Bytes factor=1000) ];
$main::table_class{'NetSNMP-real'} = "memory";

$main::table_map{'NetSNMP-disk'} = [ qw(dskAvail dskUsed) ];
$main::table_options{'NetSNMP-disk'} = [ qw(scaley=on gauge=on units=Bytes factor=1000) ];
$main::table_class{'NetSNMP-disk'} = "disk";

$main::table_map{'NetSNMP-diskper'} = [ qw(dskPercent dskPercentNode) ];
$main::table_options{'NetSNMP-diskper'} = [ qw(gauge=on units=%Used) ];
$main::table_class{'NetSNMP-diskper'} = "disk";

$main::table_map{'NetSNMP-diskio'} = [ qw(diskIONRead diskIONWritten) ];
$main::table_options{'NetSNMP-diskio'} = [ qw(units=B/s) ];
$main::table_class{'NetSNMP-diskio'} = "diskio";

$main::table_map{'NetSNMP-diskiotran'} = [ qw(diskIOReads diskIOWrites) ];
$main::table_options{'NetSNMP-diskiotran'} = [ qw(units=Trans/s) ];
$main::table_class{'NetSNMP-diskiotran'} = "diskio";



#
# Local vars
#
%mibs_of_interest_netsnmp = (
	# load average
	"laLoadInt1min"	=> ".1.3.6.1.4.1.2021.10.1.5.1",
	"laLoadInt5min"	=> ".1.3.6.1.4.1.2021.10.1.5.2",
	"laLoadInt10min"	=> ".1.3.6.1.4.1.2021.10.1.5.3",
	# cpu %
	"ssCpuUser"	=> ".1.3.6.1.4.1.2021.11.9.",
	"ssCpuSystem"	=> ".1.3.6.1.4.1.2021.11.10.",
	"ssCpuIdle"	=> ".1.3.6.1.4.1.2021.11.11.",
	# memory usage
	"memTotalSwap"	=> ".1.3.6.1.4.1.2021.4.3.",
	"memAvailSwap"	=> ".1.3.6.1.4.1.2021.4.4.",
	"memTotalReal"	=> ".1.3.6.1.4.1.2021.4.5.",
	"memAvailReal"	=> ".1.3.6.1.4.1.2021.4.6.",
	"memTotalFree"	=> ".1.3.6.1.4.1.2021.4.11.",
	# disk usage
#	"dskTotal"	=> ".1.3.6.1.4.1.2021.9.1.6.",
	"dskAvail"	=> ".1.3.6.1.4.1.2021.9.1.7.",
	"dskUsed"	=> ".1.3.6.1.4.1.2021.9.1.8.",
	"dskPercent"	=> ".1.3.6.1.4.1.2021.9.1.9.",
	"dskPercentNode"	=> ".1.3.6.1.4.1.2021.9.1.10.",
	# disk IO data
	"diskIONRead"	=> ".1.3.6.1.4.1.2021.13.15.1.1.3.",
	"diskIONWritten"	=> ".1.3.6.1.4.1.2021.13.15.1.1.4.",
	"diskIOReads"	=> ".1.3.6.1.4.1.2021.13.15.1.1.5.",
	"diskIOWrites"	=> ".1.3.6.1.4.1.2021.13.15.1.1.6.",
);

$ucd_snmpd_disk = [
	[ 1, 3, 6, 1, 4, 1, 2021, 9, 1, 1 ],		# dskIndex
	[ 1, 3, 6, 1, 4, 1, 2021, 9, 1, 2 ],		# dskPath
	[ 1, 3, 6, 1, 4, 1, 2021, 9, 1, 3 ],		# dskDevice
	[ 1, 3, 6, 1, 4, 1, 2021, 9, 1, 6 ],		# dskTotal
	[ 1, 3, 6, 1, 4, 1, 2021, 9, 1, 7 ],		# dskAvail
	[ 1, 3, 6, 1, 4, 1, 2021, 9, 1, 8 ],		# dskUsed
	[ 1, 3, 6, 1, 4, 1, 2021, 9, 1, 9 ],		# dskPercent
	[ 1, 3, 6, 1, 4, 1, 2021, 9, 1, 10 ]		# dskPercentNode
];

$ucd_snmpd_disk_io = [
	[ 1, 3, 6, 1, 4, 1, 2021, 13, 15, 1, 1, 1 ],	# diskIOIndex
	[ 1, 3, 6, 1, 4, 1, 2021, 13, 15, 1, 1, 2],	# diskIODevice
	[ 1, 3, 6, 1, 4, 1, 2021, 13, 15, 1, 1, 3 ],	# diskIONRead
	[ 1, 3, 6, 1, 4, 1, 2021, 13, 15, 1, 1, 4 ],	# diskIONWritten
	[ 1, 3, 6, 1, 4, 1, 2021, 13, 15, 1, 1, 5 ],	# diskIOReads
	[ 1, 3, 6, 1, 4, 1, 2021, 13, 15, 1, 1, 6 ]	# diskIOWrites
];


sub process_module_NetSNMP($$$) {
	my ($router,$community,$sess) = @_;

	debug("$router supports UCD/Net-SNMP MIBs");

	add_def_netsnmp($router,$community);
	$sess->map_table( $ucd_snmpd_disk, \&disk_process_netsnmp ) unless(has_class('disk'));
	$sess->map_table( $ucd_snmpd_disk_io, \&disk_process_netsnmp_io ) unless(has_class('diskio'));

	return 1;
}


sub disk_process_netsnmp() {
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
            foreach $mib ( keys %mibs_of_interest_netsnmp ) {
		next unless($mib =~ /^dsk/);
		
		if ( !$DBOFF ) {	
			create_table($mib,$rid);
		}

		$SNMP_Session::suppress_warnings=2;
		@result = rtg_snmpget ("$communities{$router}\@$router", "$mibs_of_interest_netsnmp{$mib}$index");
		$SNMP_Session::suppress_warnings=0;

		$ifspeed = "nomaxspeed";
		if(defined $result[0]) {
			print_target($router,
				"$mibs_of_interest_netsnmp{$mib}$index",
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

sub disk_process_netsnmp_io() {
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
            foreach $mib ( keys %mibs_of_interest_netsnmp ) {
		next unless($mib =~ /^diskIO/);
		
		if ( !$DBOFF ) {	
			create_table($mib,$rid);
		}
#
		# we want to make sure the device supports the requested stat, not all do
		$SNMP_Session::suppress_warnings=2;
		@result = rtg_snmpget ("$communities{$router}\@$router", "$mibs_of_interest_netsnmp{$mib}$index");
		$SNMP_Session::suppress_warnings=0;

		$ifspeed = "nomaxspeed"; # no max disk io values
		if(defined $result[0]) {
			print_target($router,
					"$mibs_of_interest_netsnmp{$mib}$index",
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

sub add_def_netsnmp($$) {
	my ($router,$comm) = @_;

	my %foundclasses=();

	foreach $sstat (keys %mibs_of_interest_netsnmp) {
		# only add by default ss mem stats
		if($sstat =~ /(^ss)|(^mem)|(^la)/) {

			# we assume all the starting rows are 0
			$row = 0;



			if($sstat =~ /^ss/) { 
				my $thisclass = "cpu";
				next if(has_class($thisclass));
				next unless(hasoid($comm,$router,$mibs_of_interest_netsnmp{$sstat}."$row"));
				$ifdescr = "CPU Stats"; 
				$ifalias = "CPU Usage % for $router";
				$ifspeed = 100;
				$foundclasses{$thisclass} = 1;
			}
			elsif($sstat =~ /^mem/) { 
				my $thisclass = "memory";
				next if(has_class($thisclass));
				next unless(hasoid($comm,$router,$mibs_of_interest_netsnmp{$sstat}."$row"));
				$ifdescr = "Memory Stats"; 
				$ifalias = "Memory Statistics for $router";
				$ifspeed = 0;
				$foundclasses{$thisclass} = 1;
			}
			elsif($sstat =~ /^laLoad/) {
				my $thisclass = "loadavg";
				$row="";
				next if(has_class($thisclass));
				next unless(hasoid($comm,$router,$mibs_of_interest_netsnmp{$sstat}."$row"));
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
                			"$mibs_of_interest_netsnmp{$sstat}$row",
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

