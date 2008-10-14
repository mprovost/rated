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
   $PHP_SELF = "targetmaker.php";
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
   $sclass = $_GET['sclass'];
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

 function fetch_array($r) {
   if($pgsql)
     return (pg_fetch_array($r));
   else
     return (mysql_fetch_array($r));
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

if (!$rid && !$iid && !$sclass) {
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

if ($rid && !$sclass) {
  echo "<SELECT NAME=\"rid\" SIZE=10>\n"; 
  echo "<OPTION SELECTED VALUE=\"$rid\">$router\n";
  echo "</SELECT>\n";
  
  $selectQuery="SHOW TABLES LIKE '%\\_$rid'";
  $selectResult=query($dbc, $selectQuery);

  $class_array = array();
  while ($selectRow=fetch_array($selectResult)){
  	list($table_name,$foo) = split("_",$selectRow[0]);
  	$selectQuery="SELECT * from mapping_table where tablename=\"$table_name\"";
  	$selectResult2=query($dbc, $selectQuery);
	while ($selectRow2=fetch_object($selectResult2)) {
	   $selectQuery="SELECT * from options_table where graphname=\"$selectRow2->graphname\"";
	   $selectResult3=query($dbc,$selectQuery);
	   $selectRow3 = fetch_object($selectResult3);
	   $class_array[$selectRow3->class] = "$selectRow2->graphname";
	}
  }
  echo "<SELECT MULTIPLE NAME=\"sclass[]\" SIZE=10>\n"; 

  foreach($class_array as $k => $v ) {
  	   echo "<OPTION VALUE=\"$k\">$k\n";
  }
  echo "</SELECT>\n";

}

if ($rid && $sclass && !$iid) {
  echo "<SELECT NAME=\"rid\" SIZE=10>\n"; 
  echo "<OPTION SELECTED VALUE=\"$rid\">$router\n";
  echo "</SELECT>\n";

  echo "<SELECT NAME=\"sclass\" SIZE=10>\n"; 
  
  $has_tables = array();
  $selectQuery="SHOW TABLES LIKE '%\\_$rid'";
  $selectResult=query($dbc, $selectQuery);
  while($selectRow = fetch_array($selectResult)) {
  	list($table_name,$foo) = split("_",$selectRow[0]);
	$has_tables[$table_name] = 1;
  }

  $stat_list = array();
  foreach($sclass as $f_class) {
	echo "<OPTION SELECTED VALUE=\"$f_class\">$f_class\n";
  	$selectQuery="SELECT graphname from options_table where class=\"$f_class\"";
	$selectResult=query($dbc,$selectQuery);
	while($selectRow = fetch_object($selectResult)) {
		echo "<!-- [$f_class] -> $selectRow->graphname -->\n";
		array_push($stat_list,$selectRow->graphname);
	}
  }
  echo "</SELECT>\n";

  $table_list = array();
  $iid_list = array();
  foreach($stat_list as $stat) {
	$selectQuery="SELECT tablename from mapping_table where graphname=\"$stat\"";
	$selectResult=query($dbc,$selectQuery);
	while($selectRow = fetch_object($selectResult)) {
		$f_table = $selectRow->tablename;
		echo "<!-- [$stat] -> $f_table -->\n";
		if(isset($has_tables[$f_table])) {
			echo "<!-- yay! we found $f_table -->\n";
			$selectQuery="SELECT DISTINCT id from ".$f_table."_$rid  WHERE dtime > DATE_SUB(CURDATE(),INTERVAL 24 HOUR);";
			print "<!-- $selectQuery -->\n";
			$selectResult2=query($dbc,$selectQuery);
			while($selectRow2 = fetch_object($selectResult2)) {
				echo "<!-- found valid iid of $selectRow2->id -->\n";
  				$selectQuery="SELECT id, name, description FROM interface WHERE id=$selectRow2->id ORDER BY name";
				print "<!-- $selectQuery -->\n";
  				$selectResult3=query($dbc, $selectQuery);
				while ($selectRow3=fetch_object($selectResult3)){
				     echo "<!-- blee $selectRow3->id -- $selectRow3->name -->\n";
				     $iid_list[$selectRow3->id] = "$selectRow3->name ($selectRow3->description)";		     
  				}
			}
		}
	}
  }

  echo "<SELECT MULTIPLE NAME=\"iid[]\" SIZE=10>\n"; 
  foreach($iid_list as $k => $v) {
	echo "<OPTION VALUE=\"$k\">$v\n";
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

  $time_range="dtime>FROM_UNIXTIME($bt) AND dtime<=FROM_UNIXTIME($et)";
  $range="$range AND id=$iid[0]";
  
  $selectQuery="SELECT description, name, speed FROM interface WHERE rid=$rid AND id=$iid[0]";
  $selectResult=query($dbc, $selectQuery);
  $selectRow=fetch_object($selectResult);

  echo "<TABLE BORDER=0>\n";
  echo "<TD><I>Device</I>:</TD><TD>$router ($rid)</TD><TR>\n";
  if(count($iid) == 1 || $aggr) {
  	if($aggr) {
		foreach($iid as $val) {
			$selectQuery="SELECT description, name, speed FROM interface WHERE rid=$rid AND id=$val";
			echo "<!-- $selectQuery -->\n";
			$selectResult2=query($dbc, $selectQuery);
			$selectRow2=fetch_object($selectResult2);
			$interface_value = $interface_value."$selectRow2->name($val) ";

		}
	}
	else {
		$interface_value = "$selectRow->name ($iid[0])";
	}	
	echo "<TD><I>Interface</I>:</TD><TD>$interface_value</TD><TR>\n";
  }
  if($sclass == "network" && count($iid) == 1) {
	  printf ("<TD><I>Speed</I>:</TD><TD>%2.3f Mbps</TD><TR>\n", $selectRow->speed / 1000000);
  }
  if(count($iid) == 1) {
  	echo "<TD><I>Description</I>:</TD><TD>$selectRow->description</TD><TR>\n";
  }
  if($aggr) {
  	echo "<TD><I>Description</I>:</TD><TD>Aggregated interfaces</TD><TR>\n";
  }
  print strftime("<TD><I>Period</I>:</TD><TD>%m/%d/%Y %H:%M - ", $bt);
  print strftime("%m/%d/%Y %H:%M</TD>\n", $et);
  echo "</TABLE>\n";
  
  $has_tables = array();
  $selectQuery="SHOW TABLES LIKE '%\\_$rid'";
  $selectResult=query($dbc, $selectQuery);
  while($selectRow = fetch_array($selectResult)) {
  	list($table_name,$foo) = split("_",$selectRow[0]);
	$has_tables[$table_name] = 1;
  }

  $stat_list = array();
  $options_list = array();
  $selectQuery="SELECT graphname,options from options_table where class=\"$sclass\"";
  $selectResult=query($dbc,$selectQuery);
  while($selectRow = fetch_object($selectResult)) {
	echo "<!-- [$f_class] -> $selectRow->graphname -->\n";
	array_push($stat_list,$selectRow->graphname);
	$options_list[$selectRow->graphname] = $selectRow->options;
  }

  $foo_iid = array();
  if($aggr) {
	$foo_iid[0] = $iid[0];
  }
  else {
	$foo_iid = $iid;
  }
 
  foreach($foo_iid as $t_iid) {
  	if(count($iid) > 1 && !$aggr) {
  		$selectQuery="SELECT description, name, speed FROM interface WHERE rid=$rid AND id=$t_iid";
		$selectResult=query($dbc, $selectQuery);
		$selectRow=fetch_object($selectResult);
		echo "<TABLE BORDER=0>\n";
  		echo "<TD><I>Interface</I>:</TD><TD>$selectRow->name ($iid[0])</TD><TR>\n";
		if($sclass == "network") {
			printf ("<TD><I>Speed</I>:</TD><TD>%2.3f Mbps</TD><TR>\n", $selectRow->speed / 1000000);
		}
		echo "<TD><I>Description</I>:</TD><TD>$selectRow->description</TD><TR>\n";
		echo "</TABLE>\n";
	}

  	foreach($stat_list as $stat) {
		$table_list = array();
		$selectQuery="SELECT tablename from mapping_table where graphname=\"$stat\"";
		$selectResult=query($dbc,$selectQuery);
		$x = 0;
		$args="";
		while($selectRow = fetch_object($selectResult)) {
			$f_table = $selectRow->tablename;
			if(isset($has_tables[$f_table])) {
				$selectQuery="SELECT id from ".$f_table."_$rid where id=$t_iid AND $time_range LIMIT 1";
				$selectResult2=query($dbc,$selectQuery);
				$selectRow2 = fetch_array($selectResult2);
				if(isset($selectRow2[0])) {
					$x++;
					$table_list["t$x"]=$f_table;
					echo "<!-- yay! we found $f_table for $stat -->\n";
					$args=$args."t$x=".$f_table."_$rid&";
				} else { 
					echo "<!-- $f_table isn't valid for this interface! -->\n"; 
				}
			}
		}
		if($x > 0) {
			if($aggr) {
				foreach($iid as $val) {
					$args=$args."iid=$val&";
				}
			}
			else {
				$args=$args."iid=$t_iid&";
			}
			$graph_options = split(" ",$options_list[$stat]);
			foreach($graph_options as $g_opt) {
				$args=$args."$g_opt&";
			}
    			if ($xplot) $args = $args."xplot=$xplot&";
			if ($yplot) $args = $args."yplot=$yplot&";
			if ($borderb) $args = $args."borderb=$borderb&";
			if ($aggr) $args = $args."aggr=yes&";
    			if ($percentile) $args = $args."percentile=$nth&";
    			if ($scalex) $args = $args."scalex=$scalex&";
			if($sclass == "network") {
	    			if ($scaley) $args = $args."scaley=$scaley&";
			}
			$args = $args."begin=$bt&end=$et";
			echo "<!-- [$stat][$args] -->\n";
    			print "<IMG SRC=$rtgplot?$args><BR>\n";
		}
  	}
	echo "<P>\n";	
  }

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
