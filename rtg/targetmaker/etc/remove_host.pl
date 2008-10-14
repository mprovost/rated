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
                "hostname=s" => \$remove_host,
                "force" => \$force_delete,
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


print "Searchign for $remove_host...\n";
$query = "SELECT rid from router where name = '$remove_host'";
my $sth = $dbh->prepare($query);
$sth->execute;

my @row = $sth->fetchrow_array;
if($row[0] > 0) {
	$rid = $row[0];
	print "RID of $remove_host is $rid\n";
} else {
	print "host $remove_host is not in RTG\n";
	exit 1;
}

print "$remove_host has the following tables:\n";
$query = "SHOW TABLES LIKE '%_$rid'";
my $sth = $dbh->prepare($query);
$sth->execute;
while(my @row = $sth->fetchrow_array) {
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

print "Done.\n";
exit(0);
