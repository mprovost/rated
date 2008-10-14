########################################################################
# $Id: HOST-RESOURCE-MIB.pl,v 1.1 2008/01/19 03:22:14 btoneill Exp $
########################################################################
# $Log: HOST-RESOURCE-MIB.pl,v $
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
# Revision 1.2  2004-09-30 12:28:12-05  btoneill
# Fixed various things
#
# Revision 1.1  2004-09-27 11:51:48-05  btoneill
# Initial revision
#
# Revision 1.2  2004/09/16 15:40:25  btoneill
# Added disk information
#
# Revision 1.1  2004/07/08 19:30:05  btoneill
# Initial revision
#
#
########################################################################
# Add stats for boxes with the standard host resources mib
# Namely we add in CPU, number of processes, and number of users 
#
#
# Set name of Module, and the OID to check to run this module
# 
#
$MODULENAME = "HOST";
$VERSION = '0.1';

#
# List of the types of stat classes that are gathered. This is used
# so that multiple modules don't collect the same classes of stats
# ie. the host-resources mib does cpu and disk, but we don't want those
# if we can get them from here
#
@CLASSES=qw(cpu numusers numprocesses disk);

# Module requires a subroutine called process_module_$MODULENAME to exist.
#
sub process_module_HOST($$$);

#
# Local sub-routines
#
sub add_def_hostsnmp($$);
sub cpu_process_hostsnmp();

# We check for host.hrSystem.hrSystemUptime.0
$MIB_TO_CHECK = ".1.3.6.1.2.1.25.1.1.0";


# Set this so it's in the main hash
$main::snmp_modules{$MODULENAME} = $MIB_TO_CHECK;

#
push(@main::statclasses,@CLASSES);

#
# map tables to make sense
$main::table_map{'HOSTRESOURCES-cpu'} = [ qw(hrProcessorLoad) ];
$main::table_options{'HOSTRESOURCES-cpu'} = [ qw(gauge=on units=% scaley=on ) ];
$main::table_class{'HOSTRESOURCES-cpu'} = "cpu";

$main::table_map{'HOSTRESOURCES-numusers'} = [ qw(hrSystemNumUsers) ];
$main::table_options{'HOSTRESOURCES-numusers'} = [ qw(gauge=on units=Usr ) ];
$main::table_class{'HOSTRESOURCES-numusers'} = "numusers";

$main::table_map{'HOSTRESOURCES-numproc'} = [ qw(hrSystemProcesses) ];
$main::table_options{'HOSTRESOURCES-numproc'} = [ qw(gauge=on units=Pcs ) ];
$main::table_class{'HOSTRESOURCES-numproc'} = "numprocesses";

$main::table_map{'HOSTRESOURCES-disk'} = [ qw(hrStorageSize hrStorageUsed) ];
$main::table_options{'HOSTRESOURCES-disk'} = [ qw(gauge=on units=Bytes factor=512) ];
$main::table_class{'HOSTRESOURCES-disk'} = "disk";

#
# Local vars
#
%mibs_of_interest_hostsnmp = (
	# cpu %
	"hrProcessorLoad"		=> ".1.3.6.1.2.1.25.3.3.1.2.",
	
	# number processes
	"hrSystemProcesses"	=> ".1.3.6.1.2.1.25.1.6.",
	
	# number users 
	"hrSystemNumUsers"	=> ".1.3.6.1.2.1.25.1.5.",

	# disk stuff
	"hrStorageSize"		=> ".1.3.6.1.2.1.25.2.3.1.5.",
	"hrStorageUsed"		=> ".1.3.6.1.2.1.25.2.3.1.6.",
);

$host_snmpd_cpu = [
	[ 1, 3, 6, 1, 2, 1, 25, 3, 2, 1, 2 ],	# hrDeviceType
	[ 1, 3, 6, 1, 2, 1, 25, 3, 4, 1, 2 ] 	# hrProcessorLoad 
];

$host_snmpd_disk = [
	[ 1, 3, 6, 1, 2, 1, 25, 2, 3, 1, 1 ],	# hrStorageIndex
	[ 1, 3, 6, 1, 2, 1, 25, 2, 3, 1, 2 ],	# hrStorageType
	[ 1, 3, 6, 1, 2, 1, 25, 2, 3, 1, 3 ],	# hrStorageDescr
	[ 1, 3, 6, 1, 2, 1, 25, 2, 3, 1, 4 ],	# hrStorageAllocationUnits
	[ 1, 3, 6, 1, 2, 1, 25, 2, 3, 1, 5 ],	# hrStorageSize
	[ 1, 3, 6, 1, 2, 1, 25, 2, 3, 1, 6 ],	# hrStorageUsed
];


sub process_module_HOST($$$) {
	my ($router,$community,$sess) = @_;

	debug("$router supports RFC 1514 Host Resource MIB");

	add_def_hostsnmp($router,$community);
	$sess->map_table( $host_snmpd_cpu, \&cpu_process_hostsnmp) unless(has_class('cpu'));
	$sess->map_table( $host_snmpd_disk, \&disk_process_hostsnmp);

	return 1;
}

sub disk_process_hostsnmp() {
    my $reserved = 0;
    my ($rowindex, $index, $type, $descr, $units, $size, $used) = @_;


    $bits = 0;
    grep ( defined $_ && ( $_ = pretty_print $_),
    	($rowindex, $index, $type, $descr, $units, $size, $used));

    $descr =~ s/\\/\//g;
    $dskdescr = "$descr";
    if($dskdescr =~ /(.+)Label/) {
    	$dskalias = "$1"; 
    } 
    else { $dskalias = "$index"; }

    %found_class = ();
    if ($type eq "1.3.6.1.2.1.25.2.1.4") {

	    if(has_class('disk') == 0) {
            	if ( !$DBOFF ) {
               	 $iid = &find_interface_id( $rid, $dskdescr, $dskalias, $dsksize * 1000000);
            	}

	    }

            foreach $mib ( keys %mibs_of_interest_hostsnmp) {
		next unless($mib =~ /hrStorage/);

		if(has_class('disk') && $mib =~ /hrStorageSize|hrStorageUsed/) { next; }
		
		if($mib =~ /hrStorageSize|hrStorageUsed/) { $found_class{'disk'}; }
		
		if ( !$DBOFF ) {
			create_table($mib,$rid);
		}


		$ifspeed = "nomaxspeed";
		print_target($router,
			"$mibs_of_interest_hostsnmp{$mib}$index",
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

sub cpu_process_hostsnmp() {
    my $reserved = 0;
    my ($rowindex, $device, $cpupercent ) = @_;

    # max cpu % is 100
    $cpusize=100;
    $bits=0;


   $processor_oid = "1.3.6.1.2.1.25.3.1.3";


    grep ( defined $_ && ( $_ = pretty_print $_),
	 ($device, $cpupercent ) );

   return unless("$processor_oid" eq "$device"); 

    $cpudescr="Unknown CPU #$rowindex";
    $cpualias="CPU #$rowindex";

    if ( !$DBOFF ) {
                $iid = find_interface_id( $rid, $cpudescr, $cpualias, $cpusize );
    }
    foreach $mib ( keys %mibs_of_interest_hostsnmp ) {
	next unless($mib =~ /hrProcessorLoad/);

	$SNMP_Session::suppress_warnings=2;
	@result = rtg_snmpget ("$communities{$router}\@$router", "$mibs_of_interest_hostsnmp{$mib}$rowindex");
	$SNMP_Session::suppress_warnings=0;

	next unless(defined $result[0]);

		
    	add_class('cpu');
	if ( !$DBOFF ) {	
		create_table($mib,$rid);
	}
#

	$ifspeed = 100; # CPU won't go over 100%
	print_target($router,
			"$mibs_of_interest_hostsnmp{$mib}$rowindex",
			$bits,
			$communities{$router},
			"$mib" . "_$rid",
			$iid,
			$ifspeed,
			"$cpualias ($cpudescr)");
    }
}

sub add_def_hostsnmp($$) {
	my ($router,$comm) = @_;

        my %foundclasses=();

	foreach $sstat (keys %mibs_of_interest_hostsnmp) {
		# only add by default ss mem stats
		# only add by default ss mem stats
                if($sstat =~ /(^hrSystemNumUsers)|(^hrSystemProcesses)/) {

                        # we assume all the starting rows are 0
                        $row = 0;

                        if($sstat =~ /^hrSystemNumUsers/) { 
                                my $thisclass = "numusers";
                                next if(has_class($thisclass));
                                next unless(hasoid($comm,$router,$mibs_of_interest_hostsnmp{$sstat}."$row"));
                                $ifdescr = "User Stats"; 
                                $ifalias = "Num Users for $router";
                                $ifspeed = "0";
                                $foundclasses{$thisclass} = 1;
                        }
                        elsif($sstat =~ /^hrSystemProcesses/) { 
                                my $thisclass = "numprocesses";
                                next if(has_class($thisclass));
                                next unless(hasoid($comm,$router,$mibs_of_interest_hostsnmp{$sstat}."$row"));
                                $ifdescr = "Process Stats"; 
                                $ifalias = "Number of Processes for $router";
                                $ifspeed = "0";
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
                                        "$mibs_of_interest_hostsnmp{$sstat}$row",
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

