########################################################################
# $Id: SNMPInformant.pl,v 1.1 2008/01/19 03:22:14 btoneill Exp $
########################################################################
# $Log: SNMPInformant.pl,v $
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
# Revision 1.2  2004-09-30 12:28:25-05  btoneill
# Various bug fixes
#
# Revision 1.1  2004-09-27 11:51:48-05  btoneill
# Initial revision
#
# Revision 1.5  2004/07/08 19:33:01  btoneill
# Removed some debugging information that was not needed
#
# Revision 1.4  2004/06/24 21:59:53  btoneill
# Changed memory pool/commit info to be in memory pool queue
#
# Revision 1.3  2004/06/24 20:36:32  btoneill
# SAdded threads support
#
# Revision 1.2  2004/06/24 20:18:20  btoneill
# Added more memory stats
#
# Revision 1.1  2004/06/24 19:59:21  btoneill
# Initial revision
#
#
########################################################################
# Add stats for SNMP-Informant MIBs
# Namely we add in CPU, Memory, diskio, etc.
#
#
# Set name of Module, and the OID to check to run this module
#
$MODULENAME = "SNMPINFORM";
$VERSION = '0.1';

#
# List of the types of stat classes that are gathered. This is used
# so that multiple modules don't collect the same classes of stats
# ie. the host-resources mib does cpu and disk, but we don't want those
# if we can get them from here
#
#@CLASSES=qw(disk diskio cpu memory memorypool threads);
@CLASSES=qw(disk diskio cpu memoryalt memorypool threads);

# Module requires a subroutine called process_module_$MODULENAME to exist.
#
sub process_module_SNMPINFORM($$$);

#
# Local sub-routines
#
sub add_def_snmpinform($$);
sub disk_process_snmpinform();
sub cpu_process_snmpinform();

# We check for enterprises.wtcs.informant.standard.memory.memoryAvailableBytes.0
$MIB_TO_CHECK = ".1.3.6.1.4.1.9600.1.1.2.1.0";


# Set this so it's in the main hash
$main::snmp_modules{$MODULENAME} = $MIB_TO_CHECK;

#
push(@main::statclasses,@CLASSES);

#
# map tables to make sense
$main::table_map{'SNMPInformant-cpu'} = [ qw(cpuPercentPrivilegedTime cpuPercentProcessorTime cpuPercentUserTime) ];
$main::table_options{'SNMPInformant-cpu'} = [ qw(gauge=on units=% scaley=on borderb=90) ];
$main::table_class{'SNMPInformant-cpu'} = "cpu";

$main::table_map{'SNMPInformant-memory'} = [ qw(hrMemorySize memoryAvailableKBytes ) ];
$main::table_options{'SNMPInformant-memory'} = [ qw(gauge=on units=Bytes factor=1000) ];
$main::table_class{'SNMPInformant-memory'} = "memoryalt";

$main::table_map{'SNMPInformant-memorypool'} = [ qw(memoryCommittedBytes memoryPoolNonpagedBytes memoryPoolPagedBytes) ];
$main::table_options{'SNMPInformant-memorypool'} = [ qw(gauge=on units=Bytes ) ];
$main::table_class{'SNMPInformant-memorypool'} = "memorypool";

$main::table_map{'SNMPInformant-memoryio'} = [ qw(fooPagingMemorySize fooPagingMemoryFree) ];
$main::table_options{'SNMPInformant-memoryio'} = [ qw(gauge=on units=Bytes factor=1000000) ];
$main::table_class{'SNMPInformant-memoryio'} = "memory";

$main::table_map{'SNMPInformant-dskiobytes'} = [ qw(lDiskDiskReadBytesPerSec lDiskDiskWriteBytesPerSec) ];
$main::table_options{'SNMPInformant-dskiobytes'} = [ qw(gauge=on units=Bytes) ];
$main::table_class{'SNMPInformant-dskiobytes'} = "diskio";

$main::table_map{'SNMPInformant-threads'} = [ qw(objectsThreads) ];
$main::table_options{'SNMPInformant-threads'} = [ qw(gauge=on units=trds) ];
$main::table_class{'SNMPInformant-threads'} = "threads";

$main::table_map{'SNMPInformant-dskio'} = [ qw(lDiskDiskReadsPerSec lDiskDiskWritesPerSec) ];
$main::table_options{'SNMPInformant-dskio'} = [ qw(gauge=on units=ops) ];
$main::table_class{'SNMPInformant-dskio'} = "diskio";

$main::table_map{'SNMPInformant-dskioper'} = [ qw(lDiskPercentDiskReadTime lDiskPercentDiskWriteTime lDiskPercentIdleTime) ];
$main::table_options{'SNMPInformant-dskioper'} = [ qw(gauge=on units=% ) ];
$main::table_class{'SNMPInformant-dskioper'} = "diskio";

$main::table_map{'SNMPInformant-dskqueue'} = [ qw(lDiskAvgDiskReadQueueLength lDiskAvgDiskWriteQueueLength) ];
$main::table_options{'SNMPInformant-dskqueue'} = [ qw(gauge=on units=ops) ];
$main::table_class{'SNMPInformant-dskqueue'} = "diskio";

$main::table_map{'SNMPInformant-dskper'} = [ qw(lDiskPercentFreeSpace) ];
$main::table_options{'SNMPInformant-dskper'} = [ qw(gauge=on units=% ) ];
$main::table_class{'SNMPInformant-dskper'} = "disk";

$main::table_map{'SNMPInformant-diskusage'} = [ qw(lDiskFreeMegabytes) ];
$main::table_options{'SNMPInformant-diskusage'} = [ qw(gauge=on units=Bytes factor=1000) ];
$main::table_class{'SNMPInformant-diskusage'} = "disk";

#
# Local vars
#
%mibs_of_interest_snmpinform = (
	# cpu %
	"cpuPercentPrivilegedTime"	=> ".1.3.6.1.4.1.9600.1.1.5.1.4.",
	"cpuPercentProcessorTime"	=> ".1.3.6.1.4.1.9600.1.1.5.1.5.",
	"cpuPercentUserTime"		=> ".1.3.6.1.4.1.9600.1.1.5.1.6.",
	
	
	# memory usage
	"hrMemorySize"			=> ".1.3.6.1.2.1.25.2.2.",
	"memoryAvailableKBytes"		=> ".1.3.6.1.4.1.9600.1.1.2.2.",
	"memoryCommittedBytes"		=> ".1.3.6.1.4.1.9600.1.1.2.4.",
	"memoryPoolNonpagedBytes"	=> ".1.3.6.1.4.1.9600.1.1.2.11.",
	"memoryPoolPagedBytes"		=> ".1.3.6.1.4.1.9600.1.1.2.12.",
	
	# disk usage
	"lDiskPercentFreeSpace"	=> ".1.3.6.1.4.1.9600.1.1.1.1.5.",
	"lDiskFreeMegabytes"	=> ".1.3.6.1.4.1.9600.1.1.1.1.20.",

	
	# diskio usage
	"lDiskPercentDiskReadTime"	=> ".1.3.6.1.4.1.9600.1.1.1.1.2.",
	"lDiskPercentDiskWriteTime"	=> ".1.3.6.1.4.1.9600.1.1.1.1.4.",
	"lDiskPercentIdleTime"		=> ".1.3.6.1.4.1.9600.1.1.1.1.6.",
	"lDiskAvgDiskReadQueueLength"	=> ".1.3.6.1.4.1.9600.1.1.1.1.8.",
	"lDiskAvgDiskWriteQueueLength"	=> ".1.3.6.1.4.1.9600.1.1.1.1.9.",
	"lDiskDiskReadBytesPerSec" 	=> ".1.3.6.1.4.1.9600.1.1.1.1.15.",
	"lDiskDiskReadsPerSec"		=> ".1.3.6.1.4.1.9600.1.1.1.1.16.",
	"lDiskDiskWriteBytesPerSec"	=> ".1.3.6.1.4.1.9600.1.1.1.1.18.",
	"lDiskDiskWritesPerSec"		=> ".1.3.6.1.4.1.9600.1.1.1.1.19.",

	# num of threads running on the system
	"objectsThreads"		=> ".1.3.6.1.4.1.9600.1.1.4.2.",

);

$inform_snmpd_cpu = [
	[ 1, 3, 6, 1, 4, 1, 9600, 1, 1, 5, 1, 1 ], 	# cpuInstance
#	[ 1, 3, 6, 1, 4, 1, 9600, 1, 1, 5, 1, 2 ], 	# cpuPercentDPCTime
#	[ 1, 3, 6, 1, 4, 1, 9600, 1, 1, 5, 1, 3 ], 	# cpuPercentInteruptTime
	[ 1, 3, 6, 1, 4, 1, 9600, 1, 1, 5, 1, 4 ], 	# cpuPercentPrivilegedTime
	[ 1, 3, 6, 1, 4, 1, 9600, 1, 1, 5, 1, 5 ], 	# cpuPercentProcessorTime
	[ 1, 3, 6, 1, 4, 1, 9600, 1, 1, 5, 1, 6 ] 	# cpuPercentUserTime
#	[ 1, 3, 6, 1, 4, 1, 9600, 1, 1, 5, 1, 9 ], 	# cpuDPCRate
#	[ 1, 3, 6, 1, 4, 1, 9600, 1, 1, 5, 1, 10 ], 	# cpuDPCsQueuedPerSec
#	[ 1, 3, 6, 1, 4, 1, 9600, 1, 1, 5, 1, 11 ] 	# cpuInterruptsPerSec
	
];

$inform_snmpd_disk = [
	[ 1, 3, 6, 1, 4, 1, 9600, 1, 1, 1, 1, 1 ],	# lDiskInstance
	[ 1, 3, 6, 1, 4, 1, 9600, 1, 1, 1, 1, 2 ],	# lDiskPercentDiskReadTime
	[ 1, 3, 6, 1, 4, 1, 9600, 1, 1, 1, 1, 4 ],	# lDiskPercentDiskWriteTime
	[ 1, 3, 6, 1, 4, 1, 9600, 1, 1, 1, 1, 5 ],	# lDiskPercentFreeSpace 
	[ 1, 3, 6, 1, 4, 1, 9600, 1, 1, 1, 1, 6 ],	# lDiskPercentIdleTime
	[ 1, 3, 6, 1, 4, 1, 9600, 1, 1, 1, 1, 8 ],	# lDiskAvgDiskReadQueueLength
	[ 1, 3, 6, 1, 4, 1, 9600, 1, 1, 1, 1, 9 ],	# lDiskAvgDiskWriteQueueLength
	[ 1, 3, 6, 1, 4, 1, 9600, 1, 1, 1, 1, 15 ],	# lDiskDiskReadBytesPerSec
	[ 1, 3, 6, 1, 4, 1, 9600, 1, 1, 1, 1, 16 ],	# lDiskDiskReadsPerSec
	[ 1, 3, 6, 1, 4, 1, 9600, 1, 1, 1, 1, 18 ],	# lDiskDiskWriteBytesPerSec
	[ 1, 3, 6, 1, 4, 1, 9600, 1, 1, 1, 1, 19 ],	# lDiskDiskWritesPerSec
	[ 1, 3, 6, 1, 4, 1, 9600, 1, 1, 1, 1, 20 ]	# lDiskFreeMegabytes
];


sub process_module_SNMPINFORM($$$) {
	my ($router,$community,$sess) = @_;

	debug("$router supports SNMP-Informant MIBs");

	add_def_snmpinform($router,$community);
	$sess->map_table( $inform_snmpd_cpu, \&cpu_process_snmpinform) unless(has_class('cpu'));
	$sess->map_table( $inform_snmpd_disk, \&disk_process_snmpinform);

	return 1;
}


sub disk_process_snmpinform() {
    my $reserved = 0;
    my ($rowindex, $index, $readper, $writeper, $freeper, $idleper, $rqueue, $wqueue, 
    		$rbtyes, $rops, $wbytes, $wops, $freebytes) = @_;


    $bits = 0;
    grep ( defined $_ && ( $_ = pretty_print $_),
        ($index, $readper, $writeper, $freeper, $idleper, $rqueue, $wqueue, 
    			$rbtyes, $rops, $wbytes, $wops, $freebytes) );

    $dskdescr = "Drive $index";
    $dskalias = "$index";

#    print "$index, $readper, $writeper, $freeper, $idleper, $rqueue, $wqueue, 
#    			$rbtyes, $rops, $wbytes, $wops, $freebytes [$rowindex]\n";

    %found_class = ();
    if ($index ne "_Total") {

	    if(has_class('disk') == 0 || has_class('diskio') == 0) {
            	if ( !$DBOFF ) {
               	 $iid = &find_interface_id( $rid, $dskdescr, $dskalias, $dsksize * 1000000);
            	}

	    }

            foreach $mib ( keys %mibs_of_interest_snmpinform ) {
		next unless($mib =~ /lDisk/);

		if(has_class('disk') && $mib =~ /lDiskPercentFreeSpace|lDiskFreeMegabytes/) { next; }
		elsif(has_class('diskio') && $mib !~ /lDickPercentFreeSpace|lDiskFreeMegabytes/) { next; }
		
		if($mib =~ /lDiskPercentFreeSpace|lDiskFreeMegabytes/) { $found_class{'disk'}; }
		else { $found_class{'diskio'}; }
		
		if ( !$DBOFF ) {
			create_table($mib,$rid);
		}


		$ifspeed = "nomaxspeed";
		print_target($router,
			"$mibs_of_interest_snmpinform{$mib}$rowindex",
			$bits,
			$communities{$router},
			"$mib" . "_$rid",
			$iid,
			$ifspeed,
			"$dskalias ($dskdescr)");
           }
   }

   foreach(keys %found_class) { add_class($_); }
}

sub cpu_process_snmpinform() {
    my $reserved = 0;
    my ($rowindex, $index, $privtime, $proctime, $usertime) = @_;

    # max cpu % is 100
    $cpusize=100;
    $bits=0;

	

    grep ( defined $_ && ( $_ = pretty_print $_),
	($index, $privtime, $proctime, $usertime ) );

    $cpudescr="Unknown CPU #$index";
    $cpualias="CPU #$index";

    # if cpu isn't active, $privtime is -1
    if ($privtime >= 0) {

	    add_class('cpu');
            if ( !$DBOFF ) {
                $iid = find_interface_id( $rid, $cpudescr, $cpualias, $cpusize );
            }
            foreach $mib ( keys %mibs_of_interest_snmpinform ) {
		next unless($mib =~ /^cpu/);
		
		if ( !$DBOFF ) {	
			create_table($mib,$rid);
		}
#

		$ifspeed = 100; # CPU won't go over 100%
		print_target($router,
				"$mibs_of_interest_snmpinform{$mib}$rowindex",
				$bits,
				$communities{$router},
				"$mib" . "_$rid",
				$iid,
				$ifspeed,
				"$cpualias ($cpudescr)");
            }
        }
}

sub add_def_snmpinform($$) {
	my ($router,$comm) = @_;

	my $found_memory = 0;

	my %found_class = ();
	foreach $sstat (keys %mibs_of_interest_snmpinform) {
		# only add by default memory
		if($sstat =~ /emory|objectsThreads/) {
		

			# do we already have a memory class found?
			if(has_class('memorypool') && $sstat =~ /emoryCommit|emoryPool/) { next; }
			elsif(has_class('theads') && $sstat =~ /objectsThreads/) { next; }
			elsif(has_class('memoryalt') && $sstat =~ /emory/ && $sstat !~ /emoryCommit|emoryPool/) { next; }


			next unless(hasoid($comm,$router,$mibs_of_interest_snmpinform{$sstat}."0"));

			if($sstat =~ /emoryCommit|emoryPool/) {
				$found_class{'memorypool'} = 1;
				$ifdescr = "Memory Pool Stats"; 
				$ifalias = "Memory Pool Statistics for $router";
				$ifspeed = 0;
			}
			elsif($sstat =~ /emory/) {
				$found_class{'memoryalt'} = 1;
				$ifdescr = "Memory Stats"; 
				$ifalias = "Memory Statistics for $router";
				$ifspeed = 0;
			}
			elsif($sstat =~ /objectsThreads/) {
				$found_class{'threads'} = 1;
				$ifdescr = "System Threads";
				$ifalias = "System Threads for $router";
				$ifspeed = 0;
			}

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
                			"$mibs_of_interest_snmpinform{$sstat}0",
	                		0,
					$comm,
					"$sstat" . "_$rid",
					$iid,
					$ifspeed,
					"$ifalias ($ifdescr)");

		}
	}

	foreach(keys %found_class) { add_class($_); }

}

# have to end with a 1
1;

