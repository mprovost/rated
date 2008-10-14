########################################################################
# $Id: Proxy.pl,v 1.1 2008/01/19 03:22:14 btoneill Exp $
########################################################################
# $Log: Proxy.pl,v $
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
$MODULENAME = "Proxy";
$VERSION = '0.1';

#
# List of the types of stat classes that are gathered. This is used
# so that multiple modules don't collect the same classes of stats
# ie. the host-resources mib does cpu and disk, but we don't want those
# if we can get them from here
#
@CLASSES=qw(pxysys pxycache pxysvr pxyclnt);


# Module requires a subroutine called process_module_$MODULENAME to exist.
#
sub process_module_Proxy($$$);

#
# Local sub-routines
#
sub add_def_proxy($$);

# We check for proxyUpTime
$MIB_TO_CHECK = ".1.3.6.1.3.25.17.1.4.0";


# Set this so it's in the main hash
$main::snmp_modules{$MODULENAME} = $MIB_TO_CHECK;

# add our classes to the main list of classes
push(@main::statclasses,@CLASSES);


#
# map tables to make sense
$main::table_map{'Proxy-mem'} = [ qw(proxyMemUsage) ];
$main::table_options{'Proxy-mem'} = [ qw(units=B gauge=on) ];
$main::table_class{'Proxy-mem'} = "pxysys";

$main::table_map{'Proxy-storage'} = [ qw(proxyStorage) ];
$main::table_options{'Proxy-storage'} = [ qw(units=B gauge=on) ];
$main::table_class{'Proxy-storage'} = "pxysys";

$main::table_map{'Proxy-cpu'} = [ qw(proxyCpuLoad) ];
$main::table_options{'Proxy-cpu'} = [ qw(units=% scaley=on gauge=on) ];
$main::table_class{'Proxy-cpu'} = "pxysys";

$main::table_map{'Proxy-cache'} = [ qw(proxyNumObjects) ];
$main::table_options{'Proxy-cache'} = [ qw(units= gauge=on) ];
$main::table_class{'Proxy-cache'} = "pxycache";

$main::table_map{'Proxy-httpserver'} = [ qw(proxyServerHttpErrors proxyServerHttpRequests) ];
$main::table_options{'Proxy-httpserver'} = [ qw(units=) ];
$main::table_class{'Proxy-httpserver'} = "pxysvr";

$main::table_map{'Proxy-httpbandwidth'} = [ qw(proxyServerHttpInKbs proxyServerHttpOutKbs) ];
$main::table_options{'Proxy-httpbandwidth'} = [ qw(units=bps factor=1024) ];
$main::table_class{'Proxy-httpbandwidth'} = "pxysvr";

$main::table_map{'Proxy-httpclient'} = [ qw(proxyClientHttpErrors proxyClientHttpRequests proxyClientHttpHits) ];
$main::table_options{'Proxy-httpclient'} = [ qw(units=) ];
$main::table_class{'Proxy-httpclient'} = "pxyclnt";

$main::table_map{'Proxy-httpclientband'} = [ qw(proxyClientHttpInKbs proxyClientHttpOutKbs) ];
$main::table_options{'Proxy-httpclientband'} = [ qw(units=bps factor=1024) ];
$main::table_class{'Proxy-httpclientband'} = "pxyclnt";



#
# Local vars
#
%mibs_of_interest_proxy = (
        "proxyMemUsage" =>           ".1.3.6.1.3.25.17.1.1.0",
        "proxyStorage" =>            ".1.3.6.1.3.25.17.1.2.0",
        "proxyCpuLoad" =>            ".1.3.6.1.3.25.17.3.1.1.0",
        "proxyNumObjects" =>         ".1.3.6.1.3.25.17.3.1.2.0",
        "proxyServerHttpErrors" =>   ".1.3.6.1.3.25.17.3.2.2.2.0",
        "proxyServerHttpRequests" => ".1.3.6.1.3.25.17.3.2.2.1.0",
        "proxyServerHttpInKbs" =>    ".1.3.6.1.3.25.17.3.2.2.3.0",
        "proxyServerHttpOutKbs" =>   ".1.3.6.1.3.25.17.3.2.2.4.0",
        "proxyClientHttpErrors" =>   ".1.3.6.1.3.25.17.3.2.1.3.0",
        "proxyClientHttpRequests" => ".1.3.6.1.3.25.17.3.2.1.1.0",
        "proxyClientHttpHits" =>     ".1.3.6.1.3.25.17.3.2.1.2.0",
        "proxyClientHttpInKbs" =>    ".1.3.6.1.3.25.17.3.2.1.4.0",
        "proxyClientHttpOutKbs" =>   ".1.3.6.1.3.25.17.3.2.1.5.0"
);

sub process_module_Proxy($$$) {
	my ($router,$community,$sess) = @_;

	debug("$router supports Proxy MIB");

	add_def_proxy($router,$community);

        add_class('pxysys');
        add_class('pxycache');
        add_class('pxysvr');
        add_class('pxyclnt');

	return 1;
}


sub add_def_proxy($$) {
	my ($router,$comm) = @_;

	my %foundclasses=();

	foreach $sstat (keys %mibs_of_interest_proxy) {
		# only add by default proxy 
                $ifspeed = 99999999;
		if($sstat =~ /(^proxy)/) {

			if($sstat =~ /^proxyNum/) { 
				next unless(hasoid($comm,$router,$mibs_of_interest_proxy{$sstat}));
				$ifdescr = "Cache Objects"; 
				$ifalias = "Number of Objects in $router cache";
			} elsif($sstat =~ /^proxyMemUsage/) { 
				next unless(hasoid($comm,$router,$mibs_of_interest_proxy{$sstat}));
				$ifdescr = "System Usage"; 
				$ifalias = "System Usage for $router";
			} elsif($sstat =~ /^proxyStorage/) { 
				next unless(hasoid($comm,$router,$mibs_of_interest_proxy{$sstat}));
				$ifdescr = "System Usage"; 
				$ifalias = "System Usage for $router";
			} elsif($sstat =~ /^proxyCpuLoad/) { 
				next unless(hasoid($comm,$router,$mibs_of_interest_proxy{$sstat}));
				$ifdescr = "System Usage"; 
				$ifalias = "System Usage for $router";
				$ifspeed = 100;
			} elsif($sstat =~ /^proxyServer/) { 
				next unless(hasoid($comm,$router,$mibs_of_interest_proxy{$sstat}));
				$ifdescr = "Proxy Server Requests"; 
				$ifalias = "Proxy Server Requests for $router";
			} elsif($sstat =~ /^proxyClient/) { 
				next unless(hasoid($comm,$router,$mibs_of_interest_proxy{$sstat}));
				$ifdescr = "Proxy Client Requests"; 
				$ifalias = "Proxy Client Requests for $router";
			}

			if ( !$DBOFF ) {
				$rid = find_router_id($router);
				$iid = find_interface_id( $rid, $ifdescr, $ifalias, $ifspeed );
			}
			else { $iid = 999; $rid=999; }
          	
			if ( !$DBOFF ) {	
				create_table($sstat,$rid);
			}

                       
                        
			print_target($router,
                			"$mibs_of_interest_proxy{$sstat}",
	                		0,
					$comm,
					"$sstat" . "_$rid",
					$iid,
					$ifspeed,
					"$ifalias ($ifdescr)");
		}


	}
}

# have to end with a 1
1;

