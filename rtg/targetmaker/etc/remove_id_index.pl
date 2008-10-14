#!/kfp/util/bin/perl
$CONFIG_FILE = "/kfp/rtg/targetmaker/etc/targetmaker.cfg";
die "Cannot find file $CONFIG_FILE, exiting...\n" unless -f $CONFIG_FILE;
require $CONFIG_FILE;


# No edits needed beyond this point
#########################################################################

use DBI;
use Getopt::Long;

$start_time = 18;
$stop_time = 6;
$sleep_seconds = 1800;
$sleep_seconds = 1800;

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


#$query = "SHOW TABLES";
$query = "SHOW TABLES LIKE '%\\_%'";
my $sth = $dbh->prepare($query);
$sth->execute;

while(@row = $sth->fetchrow_array()) {
	if($row[0] =~ /.+_\d+$/) {
		push(@table_list,$row[0]);
	} else {
		print "skipping table $row[0]...\n";
	}
}

foreach $table (sort @table_list) {
	my @cur_time = localtime();
	my $cur_hour = $cur_time[2];
	my $cur_day = $cur_time[6];
	my $sleep_on = 1;
	if($cur_hour >= $start_time || $cur_hour < $stop_time) {
		$sleep_on = 0;
	} elsif($cur_day == 0 || $cur_day == 6) {
		print "Yay! it's the weekend, we don't need to sleep!\n";
		$sleep_on = 0;
	}


	while($sleep_on == 1) {
		print "Ack! we need to sleep as it's $cur_day $cur_hour....\n";
		print "Sleeping $sleep_seconds seconds...\n";
		sleep($sleep_seconds);
		@cur_time = localtime();
		$cur_hour = $cur_time[2];
		$cur_day = $cur_time[6];
		if($cur_hour >= $start_time || $cur_hour < $stop_time) {
			$sleep_on = 0;
		} elsif($cur_day == 0 || $cur_day == 6) {
			print "Yay! it's the weekend, we don't need to sleep!\n";
			$sleep_on = 0;
		}
	}

	unless($dbh->ping) {
		print "Database connection dropped, restarting...\n";
		$dbh = DBI->connect("DBI:mysql:$db_db:host=$db_host",$db_user,$db_pass);
		if (!$dbh) {
			print "Could not connect to database ($db_db) on $db_host.\n";
        		print "Check configuration.\n";
        		exit(-1);
		}
	}

	$query = "SHOW INDEX FROM $table";
	my $sth2 = $dbh->prepare($query);
	$sth2->execute;
	while(@row2 = $sth2->fetchrow_array()) {
#		print join(":",@row2)."\n";
		$table_foo = $row2[0];
		$index = $row2[2];
		$col = $row2[4];
		if($col eq "id") {
			$query = "DROP INDEX $index ON $table_foo";
			print "$query\n";
			my $sth3 = $dbh->prepare($query);
			$sth3->execute;
		}

	}
}


print "Done.\n";
exit(0);
