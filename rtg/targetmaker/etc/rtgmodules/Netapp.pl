########################################################################
# $Id: Netapp.pl,v 1.1 2008/01/19 03:22:14 btoneill Exp $
########################################################################
# $Log: Netapp.pl,v $
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
# Revision 1.1  2004-09-30 12:26:16-05  btoneill
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
$MODULENAME = "NetApp";
$VERSION = '0.1';

#
# List of the types of stat classes that are gathered. This is used
# so that multiple modules don't collect the same classes of stats
# ie. the host-resources mib does cpu and disk, but we don't want those
# if we can get them from here
#
@CLASSES=qw(disk cpu nfs cifs);


# Module requires a subroutine called process_module_$MODULENAME to exist.
#
sub process_module_NetApp($$$);

#
# Local sub-routines
#
sub add_def_netapp($$);
sub disk_process_netapp();

# We check for enterprises.netapp.netapp1.product.productVersion.0
$MIB_TO_CHECK = ".1.3.6.1.4.1.789.1.1.2.0";


# Set this so it's in the main hash
$main::snmp_modules{$MODULENAME} = $MIB_TO_CHECK;

# add our classes to the main list of classes
push(@main::statclasses,@CLASSES);


#
# map tables to make sense
$main::table_map{'NetApp-cpu'} = [ qw(cpuBusyTimePerCent cpuIdleTimePerCent) ];
$main::table_options{'NetApp-cpu'} = [ qw(gauge=on units=CPU%) ];
$main::table_class{'NetApp-cpu'} = "cpu";

$main::table_map{'NetApp-nfsops'}	= [ qw(miscNfsOps) ];
$main::table_options{'NetApp-nfsops'} = [ qw(units=op/s) ];
$main::table_class{'NetApp-nfsops'} = "nfs";

$main::table_map{'NetApp-rpc'} = [ qw(rpcCalls rcpTcpCalls rpcUdpCalls) ];
$main::table_options{'NetApp-rpc'} = [ qw(units=Calls/s) ];
$main::table_class{'NetApp-rpc'} = "nfs";

$main::table_map{'NetApp-nfsv2'} = [ qw(v2cReads v2cWrites) ];
$main::table_options{'NetApp-nfsv2'} = [ qw(units=ops/s) ];
$main::table_class{'NetApp-nfsv2'} = "nfs";

$main::table_map{'NetApp-nfsv3'} = [ qw(v3cReads v3cWrites) ];
$main::table_options{'NetApp-nfsv3'} = [ qw(units=ops/s) ];
$main::table_class{'NetApp-nfsv3'} = "nfs";

$main::table_map{'NetApp-cifsusers'} = [ qw(cifsConnectedUsers) ];
$main::table_options{'NetApp-cifsusers'} = [ qw(gauge=on units=users) ];
$main::table_class{'NetApp-cifsusers'} = "cifs";

$main::table_map{'NetApp-cifses'} = [ qw(cifsNSessions cifsNOpenFiles cifsNOpenDirs) ];
$main::table_options{'NetApp-cifses'} = [ qw(gauge=on units=num) ];
$main::table_class{'NetApp-cifses'} = "cifs";

$main::table_map{'NetApp-cifsrw'} = [ qw(cifsReads cifsWrites) ];
$main::table_options{'NetApp-cifsrw'} = [ qw(units=ops/s) ];
$main::table_class{'NetApp-cifsrw'} = "cifs";

$main::table_map{'NetApp-cifs'} = [ qw(cifsTotalCalls) ];
$main::table_options{'NetApp-cifs'} = [ qw(units=ops/s) ];
$main::table_class{'NetApp-cifs'} = "cifs";

$main::table_map{'NetApp-disk'} = [ qw(dfKBytesTotal dfKBytesUsed dfKBytesAvail) ];
$main::table_options{'NetApp-disk'} = [ qw(gauge=on units=Bytes factor=1000) ];
$main::table_class{'NetApp-disk'} = "disk";

$main::table_map{'NetApp-diskper'} = [ qw(dfPerCentKBytesCapacity) ];
$main::table_options{'NetApp-diskper'} = [ qw(scaley=on gauge=on units=%Used) ];
$main::table_class{'NetApp-diskper'} = "disk";


#
# Local vars
#
%mibs_of_interest_netapp = (
	# CPU
	"cpuBusyTimePerCent" => ".1.3.6.1.4.1.789.1.2.1.3.",
	"cpuIdleTimePerCent" => ".1.3.6.1.4.1.789.1.2.1.5.",

	# NFS ops
	"miscNfsOps"	=> ".1.3.6.1.4.1.789.1.2.2.1.",
	"rpcCalls"	=> ".1.3.6.1.4.1.789.1.3.1.1.1.",
	"rcpTcpCalls"	=> ".1.3.6.1.4.1.789.1.3.1.1.6.",	
	"rpcUdpCalls"	=> ".1.3.6.1.4.1.789.1.3.1.1.11.",	
	"v2cReads"	=> ".1.3.6.1.4.1.789.1.3.1.2.3.1.7.",
	"v2cWrites"	=> ".1.3.6.1.4.1.789.1.3.1.2.3.1.9.",
	"v3cReads"	=> ".1.3.6.1.4.1.789.1.3.1.2.4.1.7.",
	"v3cWrites"	=> ".1.3.6.1.4.1.789.1.3.1.2.4.1.8.",
	
	# CIFS ops
	"cifsConnectedUsers"	=> ".1.3.6.1.4.1.789.1.7.2.9.",
	"cifsNSessions"		=> ".1.3.6.1.4.1.789.1.7.2.12.",
	"cifsNOpenFiles"	=> ".1.3.6.1.4.1.789.1.7.2.13.",
	"cifsNOpenDirs"		=> ".1.3.6.1.4.1.789.1.7.2.14.",
	"cifsTotalCalls"	=> ".1.3.6.1.4.1.789.1.7.3.1.1.2.",
	"cifsReads"		=> ".1.3.6.1.4.1.789.1.7.3.1.1.5.",
	"cifsWrites"		=> ".1.3.6.1.4.1.789.1.7.3.1.1.6.",
	
	# disk usage
	"dfKBytesTotal" => ".1.3.6.1.4.1.789.1.5.4.1.3.",
	"dfKBytesUsed" => ".1.3.6.1.4.1.789.1.5.4.1.4.",
	"dfKBytesAvail" => ".1.3.6.1.4.1.789.1.5.4.1.5.",
	"dfPerCentKBytesCapacity" => ".1.3.6.1.4.1.789.1.5.4.1.6.",
	
	
);

$netapp_snmpd_disk = [
	[ 1, 3, 6, 1, 4, 1, 789, 1, 5, 4, 1, 1 ],	# dfIndex
	[ 1, 3, 6, 1, 4, 1, 789, 1, 5, 4, 1, 2 ],	# dfFileSys
	[ 1, 3, 6, 1, 4, 1, 789, 1, 5, 4, 1, 3 ],	# dfKBytesTotal
	[ 1, 3, 6, 1, 4, 1, 789, 1, 5, 4, 1, 4 ],	# dfKBytesUsed
	[ 1, 3, 6, 1, 4, 1, 789, 1, 5, 4, 1, 5 ],	# dfKBytesAvail
	[ 1, 3, 6, 1, 4, 1, 789, 1, 5, 4, 1, 6 ]	# dfPerCentKBytesCapacity
];


sub process_module_NetApp($$$) {
	my ($router,$community,$sess) = @_;

	debug("$router supports Network Appliance MIBs");

	add_def_netapp($router,$community);
	$sess->map_table( $netapp_snmpd_disk, \&disk_process_netapp ) unless(has_class('disk'));

	return 1;
}


sub disk_process_netapp() {
    my $reserved = 0;
    my ($rowindex, $index, $dskdescr, $dsksize, $dskused, $dskfree, $diskpercent ) = @_;

    $bits = 0;
    grep ( defined $_ && ( $_ = pretty_print $_),
    	( $index, $dskdescr, $dsktotal, $dskused, $dskfree, $diskpercent ) );

    if ($dskdescr) {
	    add_class('disk');
            if ( !$DBOFF ) {
                $iid = &find_interface_id( $rid, $dskdescr, $dskdescr, $dsksize * 1000 );
            }
            foreach $mib ( keys %mibs_of_interest_netapp ) {
		next unless($mib =~ /^df/);
		
		if ( !$DBOFF ) {	
			create_table($mib,$rid);
		}

		$SNMP_Session::suppress_warnings=2;
		@result = rtg_snmpget ("$communities{$router}\@$router", "$mibs_of_interest_netapp{$mib}$index");
		$SNMP_Session::suppress_warnings=0;

		$ifspeed = "nomaxspeed";
		if(defined $result[0]) {
			print_target($router,
				"$mibs_of_interest_netapp{$mib}$index",
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

sub add_def_netapp($$) {
	my ($router,$comm) = @_;

	my %foundclasses=();

	foreach $sstat (keys %mibs_of_interest_netapp) {
		# only add by default ss mem stats
		if($sstat !~ /^df/) {

			# we assume all the starting rows are 0
			$row = 0;
			$bits = 0;



			if($sstat =~ /^cifs/) { 
				my $thisclass = "cifs";
				next if(has_class($thisclass));
				next unless(hasoid($comm,$router,$mibs_of_interest_netapp{$sstat}."$row"));
				$ifdescr = "CIFS Stats"; 
				$ifalias = "CIFS Stats for $router";
				$ifspeed = 0;
				$foundclasses{$thisclass} = 1;
				if($sstat =~ /^(cifsReads)|(cifsWrites)|(cifsTotalCalls)/) { $bits = 32; }
				else { $bits = 0; }
			}
			elsif($sstat =~ /^(misc)|(rpc)|(rcp)|(v2c)|(v3c)/) { 
				my $thisclass = "nfs";
				next if(has_class($thisclass));
				next unless(hasoid($comm,$router,$mibs_of_interest_netapp{$sstat}."$row"));
				$ifdescr = "NFS Stats"; 
				$ifalias = "NFS Stats for $router";
				$ifspeed = 0;
				$foundclasses{$thisclass} = 1;
				$bits = 32;
			}
			elsif($sstat =~ /^cpu/) {
				my $thisclass = "cpu";
				next if(has_class($thisclass));
				next unless(hasoid($comm,$router,$mibs_of_interest_netapp{$sstat}."$row"));
				$ifdescr = "CPU Usage";
				$ifalias = "CPU Usage % for $router";
				$ifspeed = 0;
				$foundclasses{$thisclass} = 1;
				$bits = 0;
			}

			if ( !$DBOFF ) {
				$rid = find_router_id($router);
				$iid = find_interface_id( $rid, $ifdescr, $ifalias, $ifspeed );
			}
			else { $iid = 999; $rid=999; }
          	
			if ( !$DBOFF ) {	
				$sql = "SHOW TABLE STATUS LIKE '$sstat"."_$rid'";
				my $sth = $dbh->prepare($sql)
					or die "Can't prepare $sql: $dbh->errstr\n";
				my $rv = $sth->execute
					or die "can't execute the query: $sth->errstr\n";

				if ( $sth->rows == 0 ) {
					create_table($sstat,$rid);
#					$sql = "CREATE TABLE $sstat"."_$rid (id INT NOT NULL, dtime DATETIME NOT NULL, counter BIGINT NOT NULL, KEY $sstat"."_$rid". "_idx (dtime))";
#					&sql_insert($sql);
				}
			}

			$ifspeed = "nomaxvalue"; #there is no max la
			print_target($router,
                			"$mibs_of_interest_netapp{$sstat}$row",
	                		$bits,
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

