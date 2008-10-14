<?php
  include('./common.php');

  print "<HTML>\n<!-- RTG Version $VERSION -->\n<HEAD>\n";

  /* Connect to RTG Database */
 if ($pgsql) {
    $dbc=pg_connect("host=$host user=$user password=$pass dbname=$db") or
       die ("PGSQL Connection Failed, Check Configuration.");
 } else {
    $dbc=@mysql_connect ($host, $user, $pass) or
    $dbc=@mysql_connect ("$host:/var/lib/mysql/mysql.sock", $user, $pass) or 
       die ("MySQL Connection Failed, Check Configuration.");
    mysql_select_db($db,$dbc);
 }

 if ($PHP_SELF == "") {
   $PHP_SELF = "rtg.php";
   $errors = $_GET['errors'];
   $scalex = $_GET['scalex'];
   $scaley = $_GET['scaley'];
   $aggr = $_GET['aggr'];
   $percentile = $_GET['percentile'];
   $nth = $_GET['nth'];
   $xplot = $_GET['xplot'];
   $yplot = $_GET['yplot'];
   $borderb = $_GET['borderb'];
   $iid = $_GET['iid'];
   $rid = $_GET['rid'];
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
 }

 # Query wrapper
 function query($dbc, $q) {
   if ($pgsql) 
     return (pg_query($dbc, $q));
   else 
     return (mysql_query($q, $dbc));
 }

 # Fetch wrapper
 function fetch_object($r) {
   if ($pgsql)
     return (pg_fetch_object($r));
   else
     return (mysql_fetch_object($r));
 }

 # Num rows wrapper
 function num_rows($r) {
   if ($pgsql)
     return (pg_num_rows($r));
   else
     return (mysql_num_rows($r));
 }

  # Determine router, interface names as necessary
  if ($rid && $iid) {
    $selectQuery="SELECT a.name, a.description, a.speed, b.name AS router FROM interface a, router b WHERE a.rid=b.rid AND a.rid=$rid AND a.id=$iid[0]";
    $selectResult=query($dbc, $selectQuery);
    $selectRow=fetch_object($selectResult);
    $interfaces = num_rows($selectResult);
    $name = $selectRow->name;
    $description = $selectRow->description;
    $speed = ($selectRow->speed)/1000000;
    $router = $selectRow->router;
  } else if ($rid && !$iid) {
    $selectQuery="SELECT name AS router from router where rid=$rid";
    $selectResult=query($dbc, $selectQuery);
    $selectRow=fetch_object($selectResult);
    $router = $selectRow->router;
  }

  # Generate Title 
  echo "<TITLE>RTG: ";
  if ($rid && $iid) echo "$router: $name";
  else if ($rid && !$iid) echo "$router";
  echo "</TITLE>\n";
?>

</HEAD>
<BODY BACKGROUND="rtgback.png" BGCOLOR="ffffff">
<A HREF="http://rtg.sourceforge.net" ALT="[RTG Home Page]">
<IMG SRC="rtg.png" BORDER="0">
</A>
<P>

<?php
echo "<FORM ACTION=\"$PHP_SELF\" METHOD=\"get\">\n";

if (!$rid && !$iid) {
 echo "<SELECT NAME=\"rid\" SIZE=10>\n";
 $selectQuery="SELECT DISTINCT name, rid FROM router ORDER BY name";
 $selectResult=query($dbc, $selectQuery);
 while ($selectRow=fetch_object($selectResult)){
    echo "<OPTION VALUE=\"$selectRow->rid\">$selectRow->name\n";
 }
 echo "</SELECT>\n";
}

if ($smonth != "" && $iid == "") {
 echo "<BLOCKQUOTE><FONT SIZE=+1><STRONG>\n";
 echo "Please select an interface!\n";
 echo "</FONT></STRONG></BLOCKQUOTE>\n";
}

if ($rid && !$iid) {
  echo "<SELECT NAME=\"rid\" SIZE=10>\n"; 
  echo "<OPTION SELECTED VALUE=\"$rid\">$router\n";
  echo "</SELECT>\n";

  echo "<SELECT MULTIPLE NAME=\"iid[]\" SIZE=10>\n"; 
  $selectQuery="SELECT id, name, description FROM interface WHERE rid=$rid ORDER BY name";
  $selectResult=query($dbc, $selectQuery);
  while ($selectRow=fetch_object($selectResult)){
     echo "<OPTION VALUE=\"$selectRow->id\">$selectRow->name ($selectRow->description)\n";
  }
  echo "</SELECT>\n";

  echo "<P><TABLE>\n";
  echo "<TD>From: \n";
  echo "<TD><INPUT TYPE=TEXT NAME=\"smonth\" SIZE=3 MAXLENGTH=2 VALUE=\"";
  print (date("m"));
  echo "\"> / \n";
  echo "<INPUT TYPE=TEXT NAME=\"sday\" SIZE=3 MAXLENGTH=2 VALUE=\"";
  print (date("d"));
  echo "\"> / \n";
  echo "<INPUT TYPE=TEXT NAME=\"syear\" SIZE=5 MAXLENGTH=4 VALUE=\"";
  print (date("Y"));
  echo "\">\n";
  echo "<TD><INPUT TYPE=TEXT NAME=\"shour\" SIZE=3 MAXLENGTH=2 VALUE=\"00\">:\n";
  echo "<INPUT TYPE=TEXT NAME=\"smin\" SIZE=3 MAXLENGTH=2 VALUE=\"00\">\n";

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
  echo "<TD><INPUT TYPE=TEXT NAME=\"ehour\" SIZE=3 MAXLENGTH=2 VALUE=\"23\">:\n";
  echo "<INPUT TYPE=TEXT NAME=\"emin\" SIZE=3 MAXLENGTH=2 VALUE=\"59\">\n";
  echo "<TR><TD>Aggregate Interfaces: \n";
  echo "<TD><INPUT TYPE=CHECKBOX NAME=\"aggr\">\n";
  echo "<TR><TD><INPUT TYPE=TEXT NAME=\"nth\" SIZE=2 MAXLENGTH=2 VALUE=\"95\">th Percentile: \n";
  echo "<TD><INPUT TYPE=CHECKBOX NAME=\"percentile\">\n";
  echo "<TR><TD>Fit to Data: \n";
  echo "<TD><INPUT TYPE=CHECKBOX NAME=\"scalex\">\n";
  echo "<TR><TD>Fit to Speed: \n";
  echo "<TD><INPUT TYPE=CHECKBOX NAME=\"scaley\">\n";
  echo "<TR><TD>X Size: \n";
  echo "<TD><INPUT TYPE=TEXT NAME=\"xplot\" SIZE=3 VALUE=\"500\">\n";
  echo "<TR><TD>Y Size: \n";
  echo "<TD><INPUT TYPE=TEXT NAME=\"yplot\" SIZE=3 VALUE=\"150\">\n";
  echo "<TR><TD>BorderB: \n";
  echo "<TD><INPUT TYPE=TEXT NAME=\"borderb\" SIZE=3 VALUE=\"70\">\n";
  echo "</TABLE>\n";
}

if (($bt || $smonth) && $iid) { 

  /* Format into GNU date syntax */
  if ($bt == "") {
    $bt = strtotime("$syear-$smonth-$sday $shour:$smin:00");
    $et = strtotime("$eyear-$emonth-$eday $ehour:$emin:59");
  }

  $range="dtime>FROM_UNIXTIME($bt) AND dtime<=FROM_UNIXTIME($et)";
  $range="$range AND id=$iid[0]";

  $selectQuery="SELECT description, name, speed FROM interface WHERE rid=$rid AND id=$iid[0]";
  $selectResult=query($dbc, $selectQuery);
  $selectRow=fetch_object($selectResult);
  echo "<TABLE BORDER=0>\n";
  echo "<TD><I>Device</I>:</TD><TD>$router ($rid)</TD><TR>\n";
  echo "<TD><I>Interface</I>:</TD><TD>$selectRow->name ($iid[0])</TD><TR>\n";
  printf ("<TD><I>Speed</I>:</TD><TD>%2.3f Mbps</TD><TR>\n", $selectRow->speed / 1000000);
  echo "<TD><I>Description</I>:</TD><TD>$selectRow->description</TD><TR>\n";
  print strftime("<TD><I>Period</I>:</TD><TD>%m/%d/%Y %H:%M - ", $bt);
  print strftime("%m/%d/%Y %H:%M</TD>\n", $et);
  echo "</TABLE>\n";
  echo "<P>\n";
 
  # XXX - REB - Why did this get nuked? 
  #$selectQuery="SELECT DISTINCT id FROM ifInOctets_$rid WHERE $range";
  #$selectResult=mysql_query($selectQuery, $dbc);
  #if (mysql_num_rows($selectResult) <= 0) {
  #   print "<BR>No Data Found on Interface for Given Range.<BR>\n";
  #}
  #else {
    $args = "t1=ifInOctets_$rid&t2=ifOutOctets_$rid&begin=$bt&end=$et&units=bits/s&factor=8";
    foreach ($iid as $value) {
      $args="$args&iid=$value";
    }
    if ($scalex) $args = "$args&scalex=yes";
    if ($scaley) $args = "$args&scaley=yes";
    if ($xplot) $args = "$args&xplot=$xplot";
    if ($yplot) $args = "$args&yplot=$yplot";
    if ($borderb) $args = "$args&borderb=$borderb";
    if ($aggr) $args = "$args&aggr=yes";
    if ($percentile) $args = "$args&percentile=$nth";
    print "<IMG SRC=rtgplot.cgi?$args><BR>\n";
    $args = "t1=ifInUcastPkts_$rid&t2=ifOutUcastPkts_$rid&begin=$bt&end=$et&units=pkts/s";
    foreach ($iid as $value) {
      $args="$args&iid=$value";
    }
    if ($scalex) $args = "$args&scalex=yes";
    if ($xplot) $args = "$args&xplot=$xplot";
    if ($yplot) $args = "$args&yplot=$yplot";
    if ($borderb) $args = "$args&borderb=$borderb";
    if ($aggr) $args = "$args&aggr=yes";
    if ($percentile) $args = "$args&percentile=$nth";
    print "<IMG SRC=rtgplot.cgi?$args><BR>\n";
    if ($errors) 
       print "<IMG SRC=rtgplot.cgi?t1=ifInErrors_$rid&begin=$bt&end=$et&units=errors&impulses=yes>\n";
#  }
} 

if ($pgsql) 
  pg_close($dbc);
else
  mysql_close($dbc);
?>

<P>
<INPUT TYPE="SUBMIT" VALUE="Ok">
</FORM>
<BR>
<HR>
<FONT FACE="Arial" SIZE="2">
<?php
 print "<A HREF=\"http://rtg.sourceforge.net\">RTG</A> Version $VERSION</FONT>";
?>
</BODY>
</HTML>
