#!/kfp/util/bin/perl
$CONFIG_FILE = "/kfp/rtg/targetmaker/etc/targetmaker.cfg";
die "Cannot find file $CONFIG_FILE, exiting...\n" unless -f $CONFIG_FILE;
require $CONFIG_FILE;


# No edits needed beyond this point
#########################################################################

use DBI;
use Getopt::Long;

Getopt::Long::Configure("permute", "pass_through");
if (!GetOptions("help+" => \$help,
                "force" => \$force_delete
                )) {
        usage();
        die "Bad arguments";
}


@configs = ($RTG_CONF_FILE, "rtg.conf", "/usr/local/rtg/etc/rtg.conf", "/etc/rtg.conf","/opt/rtg/etc/rtg.conf");

foreach $conf (@configs) {
  if (open CONF, "<$conf") {
    print "Reading [$conf].\n" if $DEBUG;
    while ($line = <CONF>) {
      @cVals = split /\s+/, $line;
      if ($cVals[0] =~ /DB_Host/) {
        $db_host=$cVals[1];
      } elsif ($cVals[0] =~ /DB_User/) {
        $db_user=$cVals[1];
      } elsif ($cVals[0] =~ /DB_Pass/) {
        $db_pass=$cVals[1];
      } elsif ($cVals[0] =~ /DB_Database/) {
        $db_db=$cVals[1];
      } elsif ($cVals[0] =~ /Interval/) {
        $interval=$cVals[1];
      } elsif ($cVals[0] =~ /SNMP_Port/) {
        $snmp_port=$cVals[1];
      } elsif ($cVals[0] =~ /SNMP_Ver/) {
        $snmp_ver=$cVals[1];
      }
    }
    last;
  }
}



# SQL Database Handle
$dbh = DBI->connect("DBI:mysql:$db_db:host=$db_host",$db_user,$db_pass);
if (!$dbh) {
	print "Could not connect to database ($db_db) on $db_host.\n";
        print "Check configuration.\n";
        exit(-1);
}


$query = "SHOW TABLES LIKE 'ifInOctets_%'";
my $sth_foo = $dbh->prepare($query);

$sth_foo->execute;

while(@row = $sth_foo->fetchrow_array) {
	($rid) = $row[0] =~ /ifInOctets_(.+)/;
	$query = "SELECT name from router where rid = '$rid'";
	my $sth = $dbh->prepare($query);
	$sth->execute;
	@row = $sth->fetchrow_array;
	$remove_host = $row[0];

	$query = "SELECT dtime FROM ifInOctets_$rid WHERE dtime > DATE_SUB(NOW(),INTERVAL 180 day) ORDER BY dtime DESC LIMIT 1";
#	print "$query\n";
	my $sth2 = $dbh->prepare($query);
	$sth2->execute;
	@row = $sth2->fetchrow_array;
	if(defined($row[0])) {
#		print "Found dtime of $row[0]\n";
		next;	
	} 

	print "We need to remove $remove_host...\n";

	print "$remove_host has the following tables:\n";
	$query = "SHOW TABLES LIKE '%\_$rid'";
	my $sth = $dbh->prepare($query);
	$sth->execute;
	undef(@table_list);
	while(my @row = $sth->fetchrow_array) {
		$row[0] =~ /.+\_(.+)/;
		next unless($rid == $1);
		print "$row[0]\n";
		push(@table_list,$row[0]);
	}

	unless($force_delete) {
		print "Are you sure you want to remove the host? (y/n) ";
		$answer = <STDIN>;
		unless($answer =~ /^y/i) {
			print "aborting!\n";
			exit 1;
		}
	}


	foreach $drop_table (@table_list) {
		print "Dropping $drop_table...\n";
		$query = "DROP TABLE $drop_table";
		$sth = $dbh->prepare($query);
		$sth->execute;
	}

	print "Removing from router list...\n";
	$query = "DELETE from router where rid=$rid";
	$sth = $dbh->prepare($query);
	$sth->execute;

}
print "Done.\n";

exit(0);
