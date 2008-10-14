#!/usr/bin/perl
#
# Program: populate.pl
# Author:  Rob Beverly <rbeverly@users.sourceforge.net>
# Date:    May 21, 2004
# Orig:    
# Purpose: Artificially Populate an RTG Database for the Purposes of
#          Simulation and Testing.  See USAGE.
# More:    See http://rtg.sf.net

# MySQL Database Format:
# CREATE TABLE ifInOctets (
#   id int(11) NOT NULL default '0',
#   dtime datetime NOT NULL default '0000-00-00 00:00:00',
#   counter bigint(20) NOT NULL default '0',
#   KEY ifInOctets_idx (dtime)
# );

use DBI;
use Time::Local;

$DEBUG = 1;
$GAUGE = 0;

$table = $ARGV[0];
$interval = $ARGV[1];
$interfaces = $ARGV[2];
($startdate, $enddate) = getrange($ARGV[3], $ARGV[4]);

die <<USAGE unless $table && $interval && $interfaces && $startdate && $enddate;

Artificially Populate an RTG Database
USAGE: $0 <tbl> <per> <intf> -[##d][##h][##m][##s]
   OR: $0 <tbl> <per> <intf> <mm/dd/yyyy[+hh:mm[:ss]]> <mm/dd/yyyy[+hh:mm[:ss]]>
   where <tbl>  = Database Table
         <per>  = Simulated Sampling Period (secs)
         <intf> = Number of Interfaces to Simulate
USAGE

# We'll draw from a random distribution [0,maxbytes) where 
# maxbytes = maxrate*interval
$maxrate = 100;  #Mbps

# Begin with reasonable defaults
$db="rtg";
$host="localhost";
$user="snmp";
$pass="rtgdefault";

# Default locations to find RTG configuration file
@configs = ("rtg.conf", "/usr/local/rtg/etc/rtg.conf", "/etc/rtg.conf");
foreach $conf (@configs) {
  if (open CONF, "<$conf") {
    print "Reading [$conf].\n" if $DEBUG;
    while ($line = <CONF>) {
      @cVals = split /\s+/, $line;
      if ($cVals[0] =~ /DB_Host/) { 
        $host=$cVals[1];
      } elsif ($cVals[0] =~ /DB_User/) { 
        $user=$cVals[1];
      } elsif ($cVals[0] =~ /DB_Pass/) { 
        $pass=$cVals[1];
      } elsif ($cVals[0] =~ /DB_Database/) { 
        $db=$cVals[1];
      }
    }
    last;
  }
}


sub getrange {
  my($param1, $param2) = @_;

  if(!$param2) {		# First format:  -[[[##d]##h]##m]##s
    return unless $param1 =~ /^-/;
    $datediff = substr($param1, 1);
    return unless $datediff =~ /^(([0-9]+)d)?(([0-9]+)h)?(([0-9]+)m)?(([0-9]+)s)?$/;
    ($days,$hours,$minutes,$secs) = ($2,$4,$6,$8);
    $endtime = time();
    $starttime = $endtime - (((((($days * 24) + $hours) * 60) + $minutes) * 60) + $secs);
    $startdate = timestamp($starttime);
    $enddate = timestamp($endtime);
  } else {	# Second format: <mm/dd/yyyy[+hh:mm[:ss]]> <mm/dd/yyyy[+hh:mm[:ss]]> 
    ($startdate = parsedate($param1)) || return;
    ($enddate = parsedate($param2)) || return;
    $startdate < $enddate || return;
  }

  return ($startdate, $enddate);
}

sub sqltounixtime {
  local($sqltime) = @_;

  $sqltime =~ /^(\d{4})(\d{2})(\d{2})(\d{2})(\d{2})(\d{2})/;
  my ($year, $month, $day, $hour, $min, $sec) = ($1, $2, $3, $4, $5, $6);
  my $t = timelocal($sec,$min,$hour,$day,$month-1,$year);
  #print scalar localtime($t);
  return ($t);
}

sub parsedate {
  local($str) = @_;

  $str =~ /^([0-9]{2})\/([0-9]{2})\/([0-9]{4})(\+([0-9]{2}):([0-9]{2})(:([0-9]{2}))?)?$/ || return;
  ($month, $dd, $yyyy, $hh, $mm, $ss) = ($1, $2, $3, $5, $6, $8);
  if(!$hh) { $hh = "00"; }
  if(!$mm) { $mm = "00"; }
  if(!$ss) { $ss = "00"; }

  return "$yyyy$month$dd$hh$mm$ss";
}

sub timestamp {
  local ($time) = @_;
  ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime($time);
  $mon += 1;
  $year += 1900;
  if($mday < 10) { $mday = "0".$mday; }
  if($mon < 10) { $mon = "0".$mon; }
  if($hour < 10) { $hour = "0".$hour; }
  if($min < 10) { $min = "0".$min; }
  if($sec < 10) { $sec = "0".$sec; }
  return "$year$mon$mday$hour$min$sec";
}

sub format_dt {
  my($str) = @_;
  $str =~ /^([0-9]{4})([0-9]{2})([0-9]{2})([0-9]{2})([0-9]{2})([0-9]{2})$/;
  return "$2/$3/$1 $4:$5:$6";
}

sub commify {
    local($_) = shift;
    1 while s/^(-?\d+)(\d{3})/$1,$2/;
    return $_;
}

sub exe_query {
    my($dbh, $statement) = @_;
	print "Query:", $statement, "\n" if $DEBUG;
	my $sth = $dbh->prepare($statement)
		or die "Can't prepare $statement: $dbh->errstr\n";
	my $rv = $sth->execute
		or die "can't execute the query: $sth->errstr\n";
}

sub main{
  print "RTG Database Populator.\n";
  srand(time());
  $ustart = &sqltounixtime($startdate);
  $uend = &sqltounixtime($enddate);
  $period = $uend - $ustart;
  $samples = int($period/$interval);
  $maxbytes = int( ($maxrate * 1000000 * $interval) / 8);

  print "Simulating Data for: [";
  print scalar localtime($ustart), " - ";
  print scalar localtime($uend), "]\n";
  print "Period: [$period sec]\n";
  print "Data Points to Insert: [$samples]\n";
  print "Table: [$table]\n"; 
  print "Populating from Random Distribution: [0,$maxbytes)\n";

  if ($samples <= 0) {
    print "Input Invalid.\n";
    exit(-1);
  }

  $dbh= DBI->connect("DBI:mysql:$db:host=$host", $user, $pass);
  $inserttime = $ustart;
  for ($i=0;$i<$samples;$i++) {
     for ($j=1;$j<=$interfaces;$j++) {
       if ($GAUGE) {
         $bytes = int(rand() * $maxrate + 0.5); 
       } else {
         $bytes = int(rand() * $maxbytes + 0.5); 
       }
       $statement="INSERT INTO $table VALUES($j, FROM_UNIXTIME($inserttime), $bytes)";
       &exe_query($dbh, $statement);
     }
	 $inserttime+=$interval;
  }
  $rc = $dbh->disconnect;
}

&main();
exit;
