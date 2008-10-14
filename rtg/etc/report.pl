#!/usr/bin/perl
#
# Program: report.pl
# Author:  Rob Beverly <rbeverly@users.sourceforge.net>
#          Adam Rothschild <asr@latency.net>
# Date:    August 13, 2002
# Orig:    October 1, 2001
# Purpose: Generates RTG traffic report for a particular customer
#

use DBI;

$cust = $ARGV[0];
shift(@ARGV);
($startdate, $enddate) = getrange(@ARGV);

die <<USAGE unless $cust && $startdate && $enddate;

USAGE: $0 <customer> -[##d][##h][##m][##s]
   OR: $0 <customer> <mm/dd/yyyy[+hh:mm[:ss]]> <mm/dd/yyyy[+hh:mm[:ss]]>
USAGE

$DEBUG = 0;

# Begin with reasonable defaults
$db="rtg";
$host="localhost";
$user="snmp";
$pass="rtgdefault";
$onedaysec=60*60*24;

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

sub today {
  local($_) = shift;
  ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime($_);
  $mon+=1;
  $year+=1900;
  if ($mday < 10) { $mday = "0".$mday; }
  if ($mon < 10) { $mon = "0".$mon; }
  @_ = ($mon,$mday,$year);
  return(@_);
}

sub yesterday {
  local($_) = shift;
  $_-=86400;
  ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime($_);
  $mon+=1;
  $year+=1900;
  if ($mday < 10) { $mday = "0".$mday; }
  if ($mon < 10) { $mon = "0".$mon; }
  @_ = ($mon,$mday,$year);
  return(@_);
}

sub run_query {
	# print "Query:", $statement, "\n";
	my $sth = $dbh->prepare($statement)
		or die "Can't prepare $statement: $dbh->errstr\n";
	my $rv = $sth->execute
		or die "can't execute the query: $sth->errstr\n";
	@row = $sth->fetchrow_array ();
    $rc = $sth->finish;
	if ($row[0] == "") {
		$row[0] = 0;
	}
	return @row;
}

sub interface_stats{
  my $counter = $total = 0;
  my $sample_secs = $last_sample_secs = 0;
  my $num_rate_samples =0;
  my $rate = $avgrate = $maxrate = 0;
  my $max = $avg = 0;

  my $sth = $dbh2->prepare($statement)
    or die "Can't prepare $statement: $dbh2->errstr\n";
  my $rv = $sth->execute
    or die "can't execute the query: $sth2->errstr\n";
  while (my @row = $sth->fetchrow_array()) {
    ($counter, $sample_secs) = @row;
    if ($last_sample_secs > $sample_secs || 
	$sample_secs - $last_sample_secs <= 0 ||
	$counter < 0) {
      print "*** Bad Samples\n";
      print "*** count: $counter ss: $sample_secs lss: $last_sample_secs\n";
      print "*** stmt: $statement\n";
    } else { 
      $total += $counter;
      if ($last_sample_secs != 0) {
        $num_rate_samples++;
        $rate = $counter*8/($sample_secs - $last_sample_secs);
        $avgrate += $rate;
        if ($rate > $maxrate) { $maxrate = $rate; }
      }
      $last_sample_secs = $sample_secs;
    }
  }
  if ($num_rate_samples !=0) {
   $avg = $avgrate/$num_rate_samples;
  }
  @_ = ($total,$maxrate,$avg);
  return(@_);
}


($mon, $mday, $year) = &yesterday(time());

print "$cust Traffic\n";
print "Period: [", format_dt($startdate), " to ", format_dt($enddate), "]\n\n";
($router,$name,$bytesin, $bytesout, $ratein, $rateout, $utilin, $utilout, $maxratein, $maxrateout, $maxutilin, $maxutilout) = (" ", " ", "In", "Out", "Avg In", "Avg Out", "Util", "Util", "Max In", "Max Out", "Max Util", "Max Util");
write;
($router,$name,$bytesin, $bytesout, $ratein, $rateout, $utilin, $utilout, $maxratein, $maxrateout, $maxutilin, $maxutilout) = ("","Connection", "MBytes", "MBytes", "Mbps", "Mbps", "In %", "Out%",  "Mbps", "Mbps", "In%", "Out%");
write;
print "----------------------------------------------------------------------------------------------------------\n";

$dbh= DBI->connect("DBI:mysql:$db:host=$host", $user, $pass);
$dbh2= DBI->connect("DBI:mysql:$db:host=$host", $user, $pass);
$range="dtime>$startdate and dtime<=$enddate";

$statement="SELECT id FROM interface WHERE description LIKE \"%$cust%\"";
$sth = $dbh->prepare($statement)
        or die "Can't prepare $statement: $dbh->errstr\n";
$rv = $sth->execute
        or die "can't execute the query: $sth->errstr\n";
while (@row = $sth->fetchrow_array ())
{
 push(@interfaces, $row[0]);
}

foreach $interface (@interfaces) {
  $statement="SELECT rid, name, speed FROM interface WHERE id=$interface";
  &run_query($statement);
  ($rid, $name, $speed) = @row;

  $statement="SELECT rid, name FROM router WHERE rid=$rid";
  &run_query($statement);
  ($rid, $router) = @row;

  $statement="SELECT counter, UNIX_TIMESTAMP(dtime), dtime FROM ifInOctets_".$rid." WHERE $range AND id=$interface ORDER BY dtime"; 
  ($intbytes_in, $maxin, $avgin) = &interface_stats($statement);
  $bytesin = int($intbytes_in/1000000 + .5);

  $statement="SELECT counter, UNIX_TIMESTAMP(dtime), dtime FROM ifOutOctets_".$rid." WHERE $range AND id=$interface ORDER BY dtime"; 
  ($intbytes_out, $maxout, $avgout) = &interface_stats($statement);
  $bytesout = int($intbytes_out/1000000 + .5);

  $ratein = sprintf("%2.2f", $avgin/1000000);
  $rateout = sprintf("%2.2f", $avgout/1000000);
  $maxratein = sprintf("%2.2f", $maxin/1000000);
  $maxrateout = sprintf("%2.2f", $maxout/1000000);
  if ($speed != 0) {
    $utilin = sprintf("%2.2f", $ratein/($speed/1000000)*100);
    $utilout = sprintf("%2.2f", $rateout/($speed/1000000)*100);
    $maxutilin = sprintf("%2.2f", $maxratein/($speed/1000000)*100);
    $maxutilout = sprintf("%2.2f", $maxrateout/($speed/1000000)*100);
  } else {
    $utilin = $utilout = $maxutilin = $maxutilout = 0;
  }

  write;
  $totalbytesin += $bytesin;
  $totalbytesout += $bytesout;
  $totalratein += $ratein;
  $totalrateout += $rateout;
  $totalmaxratein += $maxratein;
  $totalmaxrateout += $maxrateout;
  $bytesin = $bytesout = 0;
}

print "\n";
$name = "Total:";
$router = "";
$bytesin = $totalbytesin;
$bytesout = $totalbytesout;
$ratein = $totalratein;
$rateout = $totalrateout;
$maxratein = $totalmaxratein;
$maxrateout = $totalmaxrateout;
$utilin = "";
$utilout = "";
$maxutilin = "";
$maxutilout = "";

write;
$rc = $dbh->disconnect;
exit;

format STDOUT = 
@<<<<<<<<<<< @<<<<<<<<<<< @>>>>>>> @>>>>>>> @>>>>>> @>>>>>> @>>>>> @>>>>> @>>>>>> @>>>>>> @>>>>> @>>>>>
$name,$router,&commify($bytesin), &commify($bytesout), $ratein, $rateout, $utilin, $utilout, $maxratein, $maxrateout, $maxutilin, $maxutilout  
.
