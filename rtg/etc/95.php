<?php
  include('./common.php');

  print "<HTML>\n<!-- RTG Version $VERSION -->\n<HEAD>\n";

  /* Connect to RTG MySQL Database */
  $dbc=@mysql_connect ($host, $user, $pass) or
  $dbc=@mysql_connect ("$host:/var/lib/mysql/mysql.sock", $user, $pass) or 
     die ("MySQL Connection Failed, Check Configuration.");
  mysql_select_db($db,$dbc);

  if ($PHP_SELF == "") {
    $PHP_SELF = "95.php";
    $customer = $_GET['customer'];
    $syear = $_GET['syear'];
    $eyear = $_GET['eyear'];
    $smonth = $_GET['smonth'];
    $emonth = $_GET['emonth'];
    $sday = $_GET['sday'];
    $eday = $_GET['eday'];
    $shour = $_GET['shour'];
    $ehour = $_GET['ehour'];
    $smin = $_GET['smin'];
    $emin = $_GET['emin'];
    $debug = $_GET['debug'];
  }

 print "<TITLE>RTG: ";
 if ($customer) print "$customer\n";
?>

</TITLE>
</HEAD>
<BODY BGCOLOR="ffffff">
<A HREF="http://rtg.sourceforge.net" ALT="[RTG Home Page]">
<IMG SRC="rtg.png" BORDER="0">
</A>
<P>

<?php
function cmp($c1, $c2) {
  return (int) ($c2 - $c1);
}

function int_stats($statement, $dbc) {
  $counter = $total = 0;
  $sample_secs = $last_sample_secs = 0;
  $num_rate_samples =0;
  $avgrate = $maxrate = $cur_rate = 0;
  $max = $avg = $nintyfifth = 0;
  $rate = array();

  $selectResult=mysql_query($statement, $dbc);
  while ($selectRow=mysql_fetch_object($selectResult)){
    $counter = $selectRow->counter;
    $sample_secs = $selectRow->unixtime;
    $total += $counter;
    if ($last_sample_secs > $sample_secs ||
        $sample_secs - $last_sample_secs <= 0 || $counter < 0) {
            print "*** Bad Samples\n";
       print "*** count: $counter ss: $sample_secs lss: $last_sample_secs\n";
       print "*** stmt: $statement\n";
    }
    if ($last_sample_secs != 0 && $sample_secs - $last_sample_secs > 0) {
       $num_rate_samples++;
       $cur_rate = $counter*8/($sample_secs - $last_sample_secs);
       array_push ($rate, $cur_rate);
       $avgrate += $cur_rate;
       if ($cur_rate > $maxrate) { $maxrate = $cur_rate; }
    }
    $last_sample_secs = $sample_secs;
  }
  $ignore = round($num_rate_samples * 0.05);
  usort($rate, 'cmp');
  for ($i=0;$i<=$ignore;$i++) {
    $nintyfifth = $rate[$i];
  }
  if ($num_rate_samples !=0) {
    $avg = $avgrate/$num_rate_samples;
  }

  return array ($total,$maxrate,$avg,$nintyfifth,$num_rate_samples,$ignore);
}
?>

<HR>
Report: 95th percentile<BR>

<?php
  echo "<FORM ACTION=\"$PHP_SELF\" METHOD=\"GET\">\n";
  if (($bt || $smonth)) { 
    $dbc=@mysql_connect ($host, $user, $pass) or
    $dbc=@mysql_connect ("$host:/var/lib/mysql/mysql.sock", $user, $pass) or die ("MySQL Connection Failed, Check Configuration.");
    mysql_select_db($db,$dbc);
    /* Format into GNU date syntax */
    if ($bt == "") {
      $bt = strtotime("$syear-$smonth-$sday $shour:$smin:00");
      $et = strtotime("$eyear-$emonth-$eday $ehour:$emin:59");
    }
    echo "Customer: $customer <BR>\n";
    print strftime("Period: %m/%d/%Y %H:%M - ", $bt);
    print strftime("%m/%d/%Y %H:%M<P>\n", $et);

    $range="dtime>FROM_UNIXTIME($bt) AND dtime<=FROM_UNIXTIME($et)";

    $selectQuery="SELECT id, name, description, rid FROM interface WHERE description LIKE \"%$customer%\"";
    $selectResult=mysql_query($selectQuery, $dbc);
    if (mysql_num_rows($selectResult) <= 0) 
      print "<BR>No Such Customer Found.<BR>\n";
    else {
      while ($selectRow=mysql_fetch_object($selectResult)){
        $ids[$selectRow->id] = $selectRow->name; 
        $rids[$selectRow->id] = $selectRow->rid; 
	$desc[$selectRow->id] = $selectRow->description;
      }
      echo "<TABLE BORDER=\"1\">\n";
      echo "<TR BGCOLOR=\"#E0E0E0\">\n";
      echo "<TH COLSPAN=\"3\">Interface<TH COLSPAN=\"2\">Current Rate<TH COLSPAN=\"2\">Max Rate<TH COLSPAN=\"2\">95th %\n";
      if ($debug) 
        echo "<TH COLSPAN=\"2\">Samples<TH COLSPAN=\"2\">Ignore Top\n";
      echo "<TR BGCOLOR=\"#E0E0E0\">\n";
      echo "<TH>Name<TH>Description<TH>Router<TH>In (Mbps)<TH>Out (Mbps)<TH>In<TH>Out<TH>In<TH>Out\n";
      if ($debug) 
        echo "<TH>In<TH>Out<TH>In<TH>Out\n";
      echo "<TR BGCOLOR=\"#ffffcc\">\n";
      $yellow = 1;
      foreach($ids as $iid=>$name) {

        $selectQuery="SELECT name FROM router WHERE rid=$rids[$iid]";
        $selectResult=mysql_query($selectQuery, $dbc);
        $selectRow=mysql_fetch_object($selectResult);
	$router = $selectRow->name;

        if ($yellow) $yellow = 0;
	else $yellow = 1;

        echo "<TD>$name<TD>$desc[$iid]<TD>$router";

        $selectQuery="SELECT counter, UNIX_TIMESTAMP(dtime) as unixtime, dtime from ifInOctets_$rids[$iid] WHERE $range AND id=$iid ORDER BY dtime";
        list ($intbytes_in, $maxin, $avgin, $nfin,$insamples,$inignore) = int_stats($selectQuery, $dbc);
	$bytesin = round($intbytes_in/1000000);

        $selectQuery="SELECT counter, UNIX_TIMESTAMP(dtime) as unixtime, dtime from ifOutOctets_$rids[$iid] WHERE $range AND id=$iid ORDER BY dtime";
        list ($intbytes_out, $maxout, $avgout, $nfout,$outsamples,$outignore) = int_stats($selectQuery, $dbc);
	$bytesout = round($intbytes_in/1000000);

	printf("<TD ALIGN=\"right\">%2.2f", $avgin/1000000);
	printf("<TD ALIGN=\"right\">%2.2f", $avgout/1000000);
	printf("<TD ALIGN=\"right\">%2.2f", $maxin/1000000);
	printf("<TD ALIGN=\"right\">%2.2f", $maxout/1000000);
	printf("<TD ALIGN=\"right\">%2.2f", $nfin/1000000);
	printf("<TD ALIGN=\"right\">%2.2f", $nfout/1000000);
	if ($debug) {
          echo "<TD ALIGN=\"right\">$insamples<TD ALIGN=\"right\">$outsamples<TD ALIGN=\"right\">$inignore<TD ALIGN=\"right\">$outignore";
	}
	if ($yellow) echo "<TR BGCOLOR=\"#ffffcc\">\n";
	else echo "<TR BGCOLOR=\"#94D6E7\">\n";
      }
      echo "</TABLE>\n";
    }
  } else {
    echo "Customer Name: <INPUT TYPE=\"text\" NAME=\"customer\"><BR>\n";
    echo "<P><TABLE>\n";
    echo "<TD>From: \n";
    echo "<TD><INPUT TYPE=TEXT NAME=\"smonth\" SIZE=3 MAXLENGTH=2 VALUE=\"";
    print (date("m"));
    echo "\"> / \n";
    echo "<INPUT TYPE=TEXT NAME=\"sday\" SIZE=3 MAXLENGTH=2 VALUE=\"";
    printf("%02d", (date("d") - 1));
    echo "\"> / \n";
    echo "<INPUT TYPE=TEXT NAME=\"syear\" SIZE=5 MAXLENGTH=4 VALUE=\"";
    print (date("Y"));
    echo "\">\n";
    echo "<TD><INPUT TYPE=TEXT NAME=\"shour\" SIZE=3 MAXLENGTH=2 VALUE=\"";
    print (date("H"));
    echo "\">:\n";
    echo "<INPUT TYPE=TEXT NAME=\"smin\" SIZE=3 MAXLENGTH=2 VALUE=\"";
    print (date("i"));
    echo "\">\n";

    echo "<TR><TD>To: \n";
    echo "<TD><INPUT TYPE=TEXT NAME=\"emonth\" SIZE=3 MAXLENGTH=2 VALUE=\"";
    print (date("m"));
    echo "\"> / \n";
    echo "<INPUT TYPE=TEXT NAME=\"eday\" SIZE=3 MAXLENGTH=2 VALUE=\"";
    print (date("d"));
    echo "\"> / \n";
    echo "<INPUT TYPE=TEXT NAME=\"eyear\" SIZE=5 MAXLENGTH=4 VALUE=\"";
    print (date("Y"));
    echo "\">\n";
    echo "<TD><INPUT TYPE=TEXT NAME=\"ehour\" SIZE=3 MAXLENGTH=2 VALUE=\"";
    print (date("H"));
    echo "\">:\n";
    echo "<INPUT TYPE=TEXT NAME=\"emin\" SIZE=3 MAXLENGTH=2 VALUE=\"";
    print (date("i"));
    echo "\">\n";
    echo "<TR><TD>Debug: \n";
    echo "<TD><INPUT TYPE=CHECKBOX NAME=\"debug\">\n";
    echo "</TABLE>\n";

    echo "<P><INPUT TYPE=\"SUBMIT\" VALUE=\"Ok\">";
  }

  if ($dbc) mysql_close($dbc);
  echo "</FORM>\n";
?>
<BR>
<HR>
<FONT FACE="Arial" SIZE="2">
<?php
 print "<A HREF=\"http://rtg.sourceforge.net\">RTG</A> Version $VERSION</FONT>";
?>
</BODY>
</HTML>
