#!/usr/bin/perl
# 
# RTG MAIN Generator 
#

sub main {
  $cfgfile = $ARGV[0];
  die <<USAGE  unless $cfgfile;

  USAGE: $0 <configfile>
USAGE
  
  open CFG, "<$cfgfile" or die "Could not open file: $!";
  open MAIN, ">index.html" or die "Could not open file: $!";

  &html_header(MAIN);
  print MAIN "<FONT SIZE=\"+1\">RTG Router List</FONT><P>\n";

  $endtime = time() + (60*60);
  $starttime = $endtime - (60*60*23);
  $lastrouter = $lastid = "ZZ";
  $et = localtime($endtime);
  $st = localtime($starttime);
  
  while ($line = <CFG>) {
     chomp $line;
     if (!($line =~ /^#/)) {
        ($router, $oid, $bits, $community, $table, $id, $ifdescr, $ifalias) = split /\s/, $line;
		($tmp, $rid) = split /_/, $table;
        if ($router ne $lastrouter) {
           &html_footer(HTML_BPS);
           close(HTML_BPS);
           print MAIN "$router: ";
           print MAIN "<A HREF=$router"."_bps.html>[bitrate]</A> ";
           print MAIN "<A HREF=$router"."_pps.html>[packetrate]</A> ";
           print MAIN "<A HREF=$router"."_err.html>[errorrate]</A><BR>\n";
           open HTML_BPS, ">$router"."_bps.html" or die "Could not open file: $!";
           open HTML_PPS, ">$router"."_pps.html" or die "Could not open file: $!";
           open HTML_ERR, ">$router"."_err.html" or die "Could not open file: $!";
           &html_header(HTML_BPS);
           &html_header(HTML_PPS);
           &html_header(HTML_ERR);
           print HTML_BPS "<FONT SIZE=\"+1\">$router</FONT><BR>\n";
           print HTML_BPS "Period: $st - $et<P>\n";
           print HTML_PPS "<FONT SIZE=\"+1\">$router</FONT><BR>\n";
           print HTML_PPS "Period: $st - $et<P>\n";
           print HTML_ERR "<FONT SIZE=\"+1\">$router</FONT><BR>\n";
           print HTML_ERR "Period: $st - $et<P>\n";
       }
        $lastrouter = $router;
        if ($id ne $lastid) {
           print HTML_BPS "<TABLE BORDER=0 CELLSPACING=0>\n";
           print HTML_BPS "<TD bgcolor=\"cccccc\"><STRONG>$router ($ifdescr/$ifalias):</STRONG></TD><TR>\n";
           print HTML_BPS "</TABLE>\n";
           print HTML_BPS "<IMG SRC=rtgplot.cgi?t1=ifInOctets_$rid&t2=ifOutOctets_$rid&iid=$id&begin=$starttime&end=$endtime&units=bits/s&factor=8&scalex=yes><BR>\n";
           print HTML_PPS "<TABLE BORDER=0 CELLSPACING=0>\n";
           print HTML_PPS "<TD bgcolor=\"cccccc\"><STRONG>$router ($ifdescr/$ifalias):</STRONG></TD><TR>\n";
           print HTML_PPS "</TABLE>\n";
           print HTML_PPS "<IMG SRC=rtgplot.cgi?t1=ifInUcastPkts_$rid&t2=ifOutUcastPkts_$rid&iid=$id&begin=$starttime&end=$endtime&units=packets/s&scalex=yes><BR>\n";
           print HTML_ERR "<TABLE BORDER=0 CELLSPACING=0>\n";
           print HTML_ERR "<TD bgcolor=\"cccccc\"><STRONG>$router ($ifdescr/$ifalias):</STRONG></TD><TR>\n";
           print HTML_ERR "</TABLE>\n";
           print HTML_ERR "<IMG SRC=rtgplot.cgi?t1=ifInErrors_$rid&iid=$id&begin=$starttime&end=$endtime&units=errors/s&scalex=yes><BR>\n";
        }
        $lastid = $id;
     }
  }

  &html_footer(MAIN);

  close HTML_BPS; 
  close MAIN;
  close CFG;
}

sub html_header() {
  local($_) = shift;
  print $_ "<MAIN>\n";
  print $_ "<HEAD>\n";
  print $_ "<TITLE>RTG $router</TITLE>\n";
  print $_ "</HEAD>\n";
  print $_ "<BODY BGCOLOR=\"ffffff\">\n";
}

sub html_footer() {
  local($_) = shift;
  print $_ "</BODY>\n";
  print $_ "</MAIN>\n";
}

main;
exit(0);
