<?php
# $Id: view.inc,v 1.1 2008/01/19 03:22:15 btoneill Exp $
# $Author: btoneill $
# $Date: 2008/01/19 03:22:15 $
#####################################################################

function SetupStyle()
{
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<?php
        print "<HTML lang=\"en\">\n";
        print "<HEAD>\n";
	print "  <STYLE TYPE=\"text/css\">\n";
	print "  //<!-- \n";
	print "  body \n";
	print "  {  \n";
	print "    font-family: ".get_scheme("FONTFAMILYDEFAULT").";\n";
	print "    font-size: ".get_scheme("BODYSIZE").";\n";
	print "  }\n";
	print "\n";
	print "  th   \n";
	print "  {  \n";
	print "    font-family: ".get_scheme("FONTFAMILYDEFAULT").";\n";
	print "    font-size: ".get_scheme("THSIZE").";\n";
	print "    font-weight: ".get_scheme("THWEIGHT").";\n";
	print "    background-color: ".get_scheme("BACKGROUND").";\n";
	print "  }\n";
	print "\n";
	print "  td\n";
	print "  {\n";
	print "    font-family: ".get_scheme("FONTFAMILYDEFAULT").";\n";
	print "    font-size: ".get_scheme("TDSIZE").";\n";
	print "  }\n";
	print " \n";
	print "  form\n";
	print "  {\n";
	print "    font-family: ".get_scheme("FONTFAMILYDEFAULT").";\n";
	print "    font-size: ".get_scheme("FORMSIZE").";\n";
	print "  }\n";
	print "\n";
	print "  h1\n";
	print "  {\n";
	print "    font-family: ".get_scheme("FONTFAMILYALT").";\n";
	print "    font-size: ".get_scheme("H1SIZE").";\n";
	print "    font-weight: ".get_scheme("H1WEIGHT")."\n";
	print "  }\n";
	print "\n";
	print "  A:link\n";
	print "  {\n";
	print "    font-family: ".get_scheme("FONTFAMILYDEFAULT").";\n";
	print "    font-size: ".get_scheme("ASIZE").";\n";
	print "    text-decoration: none;\n";
	print "    color: ".get_scheme("LINKCOLOR").";\n";
	print "  }\n";
	print "  A:visited\n";
	print "  {\n";
	print "    font-family: ".get_scheme("FONTFAMILYDEFAULT").";\n";
	print "    font-size: ".get_scheme("AVISITSIZE").";\n";
	print "    text-decoration: none;\n";
	print "    color: ".get_scheme("LINKVISITED").";\n";
	print "  }\n";
	print "  A:hover\n";
	print "  {\n";
	print "    font-family: ".get_scheme("FONTFAMILYDEFAULT").";\n";
	print "    font-size: ".get_scheme("AHOVERSIZE").";\n";
	print "    text-decoration: underline;\n";
	print "    color: ".get_scheme("HOVERCOLOR").";\n";
	print "  }\n";
	print "\n";
	print "  A:link.nav\n";
	print "  {\n";
	print "    font-size: ".get_scheme("ANAVSIZE").";\n";
	print "    font-family: ".get_scheme("FONTFAMILYALT").";\n";
	print "    color: #000000;\n";
	print "  }\n";
	print "\n";
	print "  A:visited.nav\n";
	print "  {\n";
	print "    font-size: ".get_scheme("ANAVSIZE").";\n";
	print "    font-family: ".get_scheme("FONTFAMILYALT").";\n";
	print "     color: #000000;\n";
	print "  }\n";
	print "\n";
	print "  A:hover.nav\n";
	print "  {\n";
	print "    font-size: ".get_scheme("ANAVSIZE").";\n";
	print "    font-family: ".get_scheme("FONTFAMILYALT").";\n";
	print "     color: red\n";
	print "  }\n";
	print "\n";
	print "  .nav\n";
	print "  {\n";
	print "    font-size: ".get_scheme("ANAVSIZE").";\n";
	print "    font-family: ".get_scheme("FONTFAMILYALT").";\n";
	print "    color: #000000;\n";
	print "  }\n";
	print "\n";
	print "  //-->\n";
	print "  </STYLE>\n";

}

function print_footer($VERSION) {
	global $VIEWVERSION;
	global $TARGETVERSION;

	print "<HR />\n";
	print "<TABLE ".table_setup()."WIDTH=100%>\n";
	print "<TR>\n";
	print "<TD ALIGN=left><A HREF=\"http://rtg.sourceforge.net\">RTG</A> ".locale("VERSION",$VERSION)."</TD>\n";
	print "<TD ALIGN=right><A HREF=\"".$_SESSION["last_real"]."\">[".locale("PREVIOUS_TAG")."]</A></TD>\n";
	print "</TR>\n";
	print "<TR>\n";
	print "<TD ALIGN=left>targetmaker $TARGETVERSION</TD><TD ALIGN=right>view.php $VIEWVERSION</TD>";
	print "</TR></TABLE>\n";
	print "</TD></TR></TABLE>\n";
print "</TABLE>\n";
}

# get scheme information 
function get_scheme($type) {
	global $scheme;
	return $scheme[$type];
}

# table setup
function table_setup() {

        $table_string = "BORDER=".get_scheme("TABLE_BORDER");
        $table_string .= " CELLPADDING=".get_scheme("TABLE_CELLPADDING");
        $table_string .= " CELLSPACING=".get_scheme("TABLE_CELLSPACING")." ";

        return $table_string;
}

function print_headers($ver,$ref_url,$refresh,$title) {
        global $xplot_padding;
	global $RTG_PNG;
	global $CLEARGIF;
        global $VERSION;

	if($refresh != 0) {
		$request_uri = preg_replace("/\/$/","",$_SERVER["REQUEST_URI"]);
		$_SESSION["ref_page"] = $request_uri;
	} 
	

	header('Date: '.gmdate('D, d M Y H:i:s \G\M\T', time()));
	header('Last-Modified: '.gmdate('D, d M Y H:i:s \G\M\T', time()));
	header('Expires: '.gmdate('D, d M Y H:i:s \G\M\T', time() + $refresh));
	if($_SESSION['autorefresh'] >= 0) {
		header('Refresh: '.$refresh.'; url='.$ref_url);
	}
	#header("Cache-Control: no-cache, must-revalidate");
	header("Cache-Control: no-cache, No-Check, must-revalidate");
	header("Pragma: no-cache");
	header("Generator: RTG $VERSION");

	$script_array = pathinfo($_SERVER["SCRIPT_NAME"]);
#	$RTG_PNG = $script_array["dirname"]."/rtg.png";
#	$CLEARGIF = $script_array["dirname"]."/dot_clear.gif";

#        print "<meta HTTP-EQUIV=\"Generator\" CONTENT=\"RTG $VERSION\" />\n";
#   print "<meta HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=iso-8859-1\" />\n";
	SetupStyle();
        printheader($title);
	print "</HEAD>\n";
	foreach($_SESSION as $k => $v) { debug(__FUNCTION__,"$k -> $v"); }
	print "<BODY BGCOLOR=\"".get_scheme("BACKGROUND")."\">\n";

	print "<TABLE ".table_setup().">\n";
	list($s_wide,$s_high) = split("x",$_SESSION["pixel_width"]);
	$max_pixel_width = $s_wide - $xplot_padding;
	print "<TR><TD><IMG SRC=\"$CLEARGIF\" WIDTH=$max_pixel_width HEIGHT=1 BORDER=0 /></TD></TR>\n";
	print "<TR><TD><TABLE ".table_setup()."WIDTH=100%>\n";

	print "<TR><TD ALIGN=left WIDTH=30%>\n";
	print "<A HREF=\"http://rtg.sourceforge.net\">\n";
	print "<IMG SRC=\"$RTG_PNG\" BORDER=\"0\" ALT=\"[RTG Home Page]\" /></A></TD>\n"; 
	print "<TD ALIGN=center >\n";
	print "<A HREF=".$_SERVER["SCRIPT_NAME"]."/host>[".locale("HOSTS")."]</A>\n";
	print "<A HREF=".$_SERVER["SCRIPT_NAME"]."/stat>[".locale("STATS")."]</A>\n";
	print "<A HREF=".$_SERVER["SCRIPT_NAME"]."/mib>[".locale("MIB")."]</A>\n";
	print "<A HREF=".$_SERVER["SCRIPT_NAME"]."/config>[".locale("CONFIGURATION")."]</A>\n";
	print "</TD><TD ALIGN=right WIDTH=30%>".date('r')."<BR />\n";
	print "".locale("VIEW_TEXT").": ";
	if($_SESSION["business_view"]) {
		print locale("BUSINESS_TEXT");
	} else {
		print $_SESSION["graph_date"];
	}
        if($_SESSION["show_date"]) {
            print " || ".locale("DATE_TEXT").": ".$_SESSION["cur_date"]."\n";
        } else {
            print " || ".locale("OFFSET_TEXT").": ".$_SESSION["day_offset"]."\n";
        }
	print "</TD></TR>\n";

	#
	# moved this up, and removed a header, can delete soon
	#
	#print "</TABLE>\n";
	#list($s_wide,$s_high) = split("x",$_SESSION["pixel_width"]);
	#$max_pixel_width = $s_wide - 70;
	#print "<TR><TD COLSPAN=3><IMG SRC=\"$CLEARGIF\" WIDTH=$max_pixel_width HEIGHT=1 BORDER=0 /></TD></TR>\n";
	print "<TR><TD COLSPAN=3>\n";
	debug(__FUNCTION__,"refresh is $refresh");

}

function locale() {
	global $locale_string;
	$arg_array = func_get_args();
	$val = $arg_array[0];
	$ret_string = $locale_string[$val];
	$x = 1;
	foreach($arg_array as $v) {
		$ret_string = preg_replace("/%$x/","$arg_array[$x]",$ret_string);
		$x++;
	}
	if($locale_string[$val] != "") {
		return $ret_string;
	} else {
		return "[NO LOCALIZATION FOR $val]";
	}
}

#
# return a nice table of info on the host
#
function print_host_header($header_array) {
	$header_string = "";
	$bgc1 = get_scheme("INFONAME");
	$bgc2 = get_scheme("INFOVALUE");

	if(isset($header_array["Speed:"])) {
		if($header_array["Speed:"] >= 1000 * 1000 * 1000) {
			$speed = sprintf("%dGbps",$header_array["Speed:"] / (1000 * 1000 * 1000));
		} elseif($header_array["Speed:"] >= 1000 * 1000) {
			$speed = sprintf("%dMbps",$header_array["Speed:"] / (1000 * 1000));
		} elseif($header_array["Speed:"] >= 1000) {
			$speed = sprintf("%dKbps",$header_array["Speed:"] / 1000);
		} else { 
			$speed = sprintf("%dbps",$header_array["Speed:"]);
		}

		$header_array["Speed:"] = $speed;
	}

	$header_string .= "<TABLE ".table_setup()." WIDTH=100%>\n";
	foreach($header_array as $k => $v) {
		$header_string .= "<TR><TD ALIGN=left VALIGN=top BGCOLOR=\"$bgc1\" WIDTH=\"5%\"><B>$k</B></TD>\n";
		#$header_string .= "<TD WIDTH=99% ALIGN=left BGCOLOR=\"$bgc2\">$v</TD></TR>\n";
		# fixes problem with really long sysDescr strings like on cisco devices that have returns in them
		$header_string .= "<TD WIDTH=99% ALIGN=left BGCOLOR=\"$bgc2\">".preg_replace("/\n/","<BR />",$v)."</TD></TR>\n";

	}
	$header_string .= "</TABLE>\n";

	return $header_string;
}

function get_graph_size($max_cols) {
        global $xplot_padding;
        global $y_def_height;

	# the size of the graphs
	$yplot = $y_def_height;
	list($s_wide,$s_high) = split("x",$_SESSION["pixel_width"]);
	$max_pixel_width = $s_wide - $xplot_padding;

	# default padding of image size
	# based on rtgplot.h
	$def_table_pad_l = 50;
	$def_table_pad_r = 20;

	$pixel_pad = ($def_table_pad_l + $def_table_pad_r) * $max_cols;
	$xplot = sprintf("%d",($max_pixel_width - $pixel_pad) / $max_cols);

	return array($xplot,$yplot);
}

# return list of screen sizes
function get_screen_sizes() {
	$screen_sizes = array("800x600","1024x768","1152x900","1280x1024","1600x1400");
	return $screen_sizes;

}

# return list of possible date formats
function get_dates() {
	$date_list = array("4h","8h","12h","1d","1w","1m","1y");
	return $date_list;
}

# return list of possible refresh times
function get_refresh_times() {
	$time_list = array("-1","60","120","300","600");
	return $time_list;
}

#
# returns a prettier version of our date
#
function get_date_pretty($graph_len) {

	$time_string = "";
	$time_format = substr($graph_len,-1);
        $time_num = substr($graph_len,0,-1);
        if($time_format == "h") {
		$time_string = "$time_num ".locale("HOUR");
	} elseif($time_format== "d") {
		$time_string = "$time_num ".locale("DAY");
	} elseif($time_format == "w") {
		$time_string = "$time_num ".locale("WEEK");
	} elseif($time_format == "m") {
		$time_string = "$time_num ".locale("MONTH");
	} elseif($time_format == "y") {
		$time_string = "$time_num ".locale("YEAR");
	}
	else {
		$time_string= "$time_num ".locale("SECOND");
	}

#	if($time_num > 1) { $time_string .= "s"; }
	
	return $time_string;
}

 # Query wrapper
function query($dbc, $q) {
    global $total_sql_time;
    global $oracle;
    global $pgsql;
    $start_time = microtime_float();
    if ($pgsql) {
        $ret_val = pg_query($dbc,$q);
    } elseif($oracle) {
        $stid = oci_parse($dbc,$q);
        if (!$stid) {
           $e = oci_error($conn);
           debug(__FUNCTION__,$e['message']);
        }
        $r = oci_execute($stid, OCI_DEFAULT);
        if (!$r) {
            $e = oci_error($stid);
            debug(__FUNCTION__,$e['message']);
        }
        $ret_val = $stid;
    } else {
        $ret_val = mysql_query($q, $dbc);
    }
    $end_time = microtime_float();
    $query_time = $end_time - $start_time;
    debug(__FUNCTION__,sprintf("SQL Query[%0.3f] %s",$query_time,$q));

    $total_sql_time += $query_time;
    return $ret_val;
}

# Fetch wrapper
function fetch_object($r) {
    global $oracle;
    global $pgsql;
    if ($pgsql)
        return (pg_fetch_object($r));
    elseif($oracle) {
        $ret_val = oci_fetch_object($r);
        return ($ret_val);
    } else
	return (mysql_fetch_object($r));
}

function fetch_array($r) {
    global $oracle;
    global $pgsql;
    if($pgsql)                                                                                                                    
        return (pg_fetch_array($r));                                                                                                 
    elseif($oracle) {
        $ret_val = oci_fetch_array($r,OCI_RETURN_NULLS);
        if( !$ret_val) {
            $e = oci_error($r);
            debug(__FUNCTION__,$e['message']);
        }
        return ($ret_val);
    } else
        return (mysql_fetch_array($r));
}

# Num rows wrapper
function num_rows($r) {
    global $oracle;
    global $pgsql;
    if ($pgsql)
        return (pg_num_rows($r));
    elseif ($oracle) {
        #$ret_val = oci_num_rows($r);
        #if( !$ret_val) {
        #    $e = oci_error($r);
        #    debug(__FUNCTION__,$e['message']);
        #}
        #return ($ret_val);
        # Note, this doesn't work for Oracle as oci_num_rows() only
        # works after a fetch has been done. We just sent a 1 to get past the
        # check.
        return 1;
    } else
        return (mysql_num_rows($r));
}


function printheader($title) {
  # Generate Title 
  echo "<TITLE>RTG: $title</TITLE>\n";

#  print "<meta HTTP-EQUIV=\"Generator\" CONTENT=\"RTG $VERSION\" />\n";
#  print "<meta HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=iso-8859-1\" />\n";
#  print "<meta HTTP-EQUIV=\"Refresh\" CONTENT=\"300\">\n";
#  print "<meta HTTP-EQUIV=\"Pragma\" CONTENT=\"no-cache\">\n";
#  print "<meta HTTP-EQUIV=\"Cache-Control\" CONTENT=\"no-cache\">\n";
#  print "<meta HTTP-EQUIV=\"Expires\" CONTENT=\"".gmdate('D, d M Y H:i:s \G\M\T', time() + 300)."\">\n";

}

#function is_host_active($dbc,$rid) {
#	$ret_val = $_SESSION["show_inactive"];
#
#	if($ret_val == "0") {	
#		$selectQuery="SELECT rid from interface where rid=\"$rid\" and status=\"active\"";
#		$selectResult=query($dbc,$selectQuery);
#		if($selectRow=fetch_object($selectResult)) {
#				$ret_val=1;
#		}
#	}
#	
#	return $ret_val;
#}

function listhosts($dbc) {
	$max_rows = 4;
	#printheader(locale("LIST_HOST"));

	print "<TABLE ".table_setup()." WIDTH=100%>\n";
	print "<TR>\n";
	print "<TD COLSPAN=$max_rows BGCOLOR=".get_scheme("INFONAME").">".locale("LIST_HOST")."</TD>";
	print "</TR>\n";
        print "<TR><TD>\n";

	$selectQuery="SELECT rid, name FROM router ORDER BY name";
	$selectResult=query($dbc,$selectQuery);
	if (num_rows($selectResult) <= 0)
		print "<BR />No Routers Found.<BR />\n";
	else {
		$column = 0;
		print "<TABLE ".table_setup()."WIDTH=100%>\n";
		while ($selectRow=fetch_object($selectResult)){
			if(is_host_active($dbc,$selectRow->rid)) {
				if($column == 0) {
					print "<TR>\n";
				}
				$request_uri = preg_replace("/\/$/","",$_SERVER["REQUEST_URI"]);
				print "<TD><A HREF=\"".$request_uri."/$selectRow->name\">";
				print "$selectRow->name</A></TD>\n";
				$column++;
				if($column >= $max_rows) {
					print "</TR>\n";
					$column = 0;
				} 
			}
		}
		print "</TABLE>\n";
	}
        print "</TD></TR>\n";
        print "</TABLE>\n";

}

function liststats($dbc) {
	$max_rows = 4;
	$per_width = 100 / $max_rows;

	#printheader(locale("LIST_STAT"));
	$selectQuery = "SELECT class from options_table";
	$selectResult = query($dbc,$selectQuery);
	$class_array = array();
	while($selectRow = fetch_object($selectResult)) {
		$class_array[$selectRow->class]++;
	}

	ksort($class_array);
	
	print "<TABLE ".table_setup()."WIDTH=100%>\n";
	print "<TR>\n";
	print "<TD COLSPAN=$max_rows BGCOLOR=".get_scheme("INFONAME").">".locale("CLASS_LIST")."</TD>";
	print "</TR>\n";

	foreach($class_array as $k => $v) {
		if($column == 0) {
			print "<TR>\n";
		}
		$request_uri = preg_replace("/\/$/","",$_SERVER["REQUEST_URI"]);
		print "<TD WIDTH=$per_width%><A HREF=\"".$request_uri."/$k\">";
		print "$k</A></TD>\n";
		$column++;
		if($column >= $max_rows) {
			print "</TR>\n";
			$column = 0;
		} 
	}

	if($column != 0) { 
		while($column < $max_rows) {
			print "<TD WIDTH=$per_width%></TD>\n";
			$column++;
		}
		print "</TR>\n"; 
	}

	print "</TABLE>\n";
}

function stathosts($dbc,$stat) {
	$max_rows = 4;
	$per_width = 100 / $max_rows;

	#printheader(locale("CLASS_HOSTS",$stat));
	
	print "<TABLE ".table_setup()." WIDTH=100%>\n";
	print "<TR>\n";
	print "<TD COLSPAN=$max_rows BGCOLOR=".get_scheme("CLASSLINE").">".locale("CLASS_HOSTS",$stat)."</TD>";
	print "</TR>\n";

	$selectQuery = "SELECT graphname from options_table WHERE class = '$stat'";
	$selectResult = query($dbc,$selectQuery);
	$host_list = array();
	while($selectRow = fetch_object($selectResult)) {
		list($module_name,$foo) = explode("-",$selectRow->graphname);
#		$selectQuery = "SELECT tablename from mapping_table WHERE graphname=\"$selectRow->graphname\"";
#		$selectResult2 = query($dbc,$selectQuery);
#		while($selectRow2 = fetch_object($selectResult2)) {
                $selectRow2 = get_mapping_table($dbc,$selectRow->graphname);
                foreach($selectRow2 as $table_name => $v) {
			#$selectQuery = "SHOW TABLES LIKE \"$table_name\_%\"";
			$selectQuery = "SELECT tablename from table2iid_mapping WHERE tablename LIKE '".$table_name."_%'";
			$selectResult3 = query($dbc,$selectQuery);
			while($selectRow3 = fetch_array($selectResult3)) {
				list($foo,$rid) = explode("_",$selectRow3[0]);
				$host_list[$module_name][$rid] = 1;
			}
		}
	}

	foreach($host_list as $k => $v) {
	        $column = 0;
		print "<TR><TD BGCOLOR=".get_scheme("INFONAME").">$k</TD></TR>\n";
		print "<TR><TD>\n";
		print "<TABLE ".table_setup()." WIDTH=100%>\n";
		foreach($v as $k2 => $v2) {
			$device = gethost($dbc,$k2);
                        if($device == -1) { continue; }

			if($column == 0) {
				print "<TR>\n";
			}
			$request_uri = preg_replace("/\/$/","",$_SERVER["REQUEST_URI"]);
			print "<TD WIDTH=$per_width%><A HREF=\"".$request_uri."/$device\">";
			print "$device</A></TD>\n";
			$column++;
			if($column >= $max_rows) {
				print "</TR>\n";
				$column = 0;
			} 
		}

		if($column != 0) { 
			while($column < $max_rows) {
				print "<TD WIDTH=$per_width%></TD>\n";
				$column++;
			}
		}
                print "</TR></TABLE>\n";
	}
		
	print "</TABLE></TD></TR>\n";

	print "</TABLE>\n";
}

#
#
#
function displaystats($dbc,$stat,$host,$id) {
	#printheader("$host:$stat:$id");
	$rid = getrid($dbc,$host);
	$max_cols = 2;

	list($xplot,$yplot) = get_graph_size($max_cols);

#	$selectQuery="SELECT class from options_table WHERE graphname=\"$stat\"";
#	$selectResult=query($dbc,$selectQuery);
#	$selectRow=fetch_object($selectResult);
        $selectRow=get_options_table($dbc,$stat);
	$class=$selectRow->class;
	
	$selectQuery="SELECT id, name, description, speed FROM interface WHERE id=$id";
	$selectResult=query($dbc,$selectQuery);
	$selectRow=fetch_object($selectResult);

	$header_array[locale("TABLE_HOST")] = $host;
	$header_array[locale("TABLE_SYSDESCR")] = getsysDescr($dbc,$host);
	$header_array[locale("TABLE_INTERFACE")] = $selectRow->description;
	$header_array[locale("TABLE_DESCRIPTION")] = $selectRow->name;
	if(preg_match('/network$/',$class)) {
		$header_array[locale("TABLE_SPEED")] = $selectRow->speed;
	}

	$host_header = print_host_header($header_array);
	
	
	$et = time();
        if($_SESSION["day_offset"] > 0) {
            $et = $et - (86400 * $_SESSION["day_offset"]);
        }

	$table_header = array();
	$table_data = array();
	$col_num = 0;
	if($_SESSION["include_year"]) {
		$foreach_array=array("8h","24h","7d","1m","1y");
	} else {
		$foreach_array=array("8h","24h","7d","1m");
	}
	#foreach(array("8h","24h","7d","1m","1y") as $graph_date) {
	#foreach(array("8h","24h","7d","1m") as $graph_date) {
	foreach($foreach_array as $graph_date) {
	#foreach(array("24h","7d","1m","1y") as $graph_date) {
		$bt = $et - conv_our_time($graph_date);
		$graph_tag=create_img_tag($dbc,$stat,$rid,$id,$xplot,$yplot,$bt,$et,$header_array[locale("TABLE_INTERFACE")]);
		$table_header[$col_num] = "<TD BGCOLOR=\"".get_scheme("STATDATE")."\" ALIGN=left>".locale("PAST")." ".get_date_pretty($graph_date)."</TD>\n";
		$table_data[$col_num] = "<TD WIDTH=45%>$graph_tag</TD>\n";
		$col_num++;
	}
	
	print "<TABLE ".table_setup()."WIDTH=100%>\n";
	print "<TR><TD COLSPAN=$max_cols>$host_header</TD></TR>\n";
	$col_num = 0;
	$col_count = 0;
	$stat_num = 0;

	foreach($table_header as $k => $v) {
		if($col_num == 0) {
			print "<TR>\n";
		}
		print "$table_header[$k]\n";
		$col_num++;
		if($col_num >= $max_cols) {
			print "<TD WIDTH=99%></TD></TR>\n";
			$col_count = $stat_num - ($max_cols - 1 );
			print "<TR>\n";
			while($col_count <= $stat_num) {
				print $table_data[$col_count]."\n";
				$col_count++;
			}
			print "<TD WIDTH=99%></TD></TR>\n";
			$col_num = 0;
		}
		$stat_num++;
	}
	if($col_num != 0) {
		#print "<TD WIDTH=99%></TD></TR>\n";
		print "<TD></TD></TR>\n";
		$col_count = $stat_num - ($max_cols - 1 );
		print "<TR>\n";
		while($col_count <= $stat_num) {
			print $table_data[$col_count]."\n";
			$col_count++;
		}
		#print "<TD WIDTH=99%></TD></TR>\n";
		print "<TD></TD></TR>\n";
		$col_num = 0;
	}
	print "</TABLE>\n";
}

#
# display all id's in the listed class
#
function displayclass($dbc,$class,$host) {
    #printheader("$class on $host");

    print "<TABLE ".table_setup()." WIDTH=100%>\n";
    print "<TR>";
    print "<TD BGCOLOR=\"".get_scheme("STATHOST")."\" WIDTH=1%> </TD>\n";
    print "<TD WIDTH=45% ALIGN=left BGCOLOR=\"".get_scheme("STATNAME")."\">".locale("CLASSDISPLAY",$class,$host)."</TD>";
    print "</A><TD WIDTH=99%></TD></TR>\n";
    $x = 0;


    $split_array = split(",",$host);
    
    $sql_expr = array();
    foreach($split_array as $sp_host) {
        array_push($sql_expr,"name LIKE '".preg_replace('/\*/','%',$sp_host)."'");
    }


    debug(__FUNCTION__,"sql_expr: ".implode(' OR ',$sql_expr) );

    #$selectQuery="SELECT name from router where name LIKE '".preg_replace('/\*/','%',$host)."' ORDER BY name";
    $selectQuery="SELECT name from router where ".implode(' OR ',$sql_expr)." ORDER BY name";
    $selectResult0 = query($dbc,$selectQuery);

    while($selectRow0=fetch_object($selectResult0)) {
        $this_host = $selectRow0->name;
        num_of_devices(1);
        debug(__FUNCTION__,"found host [$this_host]");

        $rid = getrid($dbc,$this_host);
        if($rid == -1) {
            print "ERROR: $this_host is not in the database\n";
            return 1;
        }
        
        $active = is_host_active($dbc,$rid);
        if($active <= 0) { 
            debug(__FUNCTION__,"$rid is not active, skipping...");
            continue; 
        }

        $table_list = get_table_list($dbc,$rid);

        if(!isset($table_list)) { continue; }

        $class_array = array();
        foreach($table_list as $table_name => $v) {
            $selectRow2=get_mapping_table_by_tablename($dbc,$table_name);
            if($selectRow2) {
                $selectRow3 = get_options_table($dbc,$selectRow2->graphname);
                list($module_name,$foo) = split("-",$selectRow2->graphname);
                $class_array[$selectRow3->class][$module_name] = 1;
            }
        }

        if(!isset($class_array[$class])) { 
            continue; 
        }

        foreach($class_array[$class] as $module => $v) {
            if($x % 2 == 0) {
                $bgcolor=get_scheme("STATHOST");
            }
            else { $bgcolor=get_scheme("STATHOSTALT"); }

            $html_string = stat_html_string($dbc,$this_host,$module,$class,$rid,$_SESSION["graph_date"]);
            if($html_string != "") {
                $split_host = preg_split('//',$this_host);
                $ver_host = "";
                foreach($split_host as $char) { 
                    $ver_host .= "$char<BR />"; 
                }
                print "<TR><TD BGCOLOR=\"$bgcolor\"><FONT FACE=\"ARIAL\" SIZE=2 COLOR=\"black\">\n";
                print "<A HREF=\"".$_SERVER["SCRIPT_NAME"]."/host/$this_host\" COLOR=\"black\">$ver_host</A></TD>\n";
                print "<TD>\n";
                debug(__FUNCTION__,"html_string start");
                print "$html_string\n";
                debug(__FUNCTION__,"html_string insert end");
                print "</TD></TR>\n";
            }
        }
        $x++;
    }

    print "</TABLE>\n";
}

#
# display a list of all mib "names" that are in
# the database, this is really just the first part 
# of the table names. 
#
function display_mibs($dbc) {

	$max_rows = 4;
	$per_width = 100 / $max_rows;
	
#	$selectQuery="SHOW TABLES LIKE '%\_%'";
        $selectQuery="SELECT tablename FROM table2iid_mapping where tablename LIKE '%_%'";
	$selectResult=query($dbc,$selectQuery);
	$mib_array = array();

	while($selectRow=fetch_array($selectResult)) {
		list($table_name,$rid) = split("_",$selectRow[0]);

		# make sure it's a stat with an rid at the end
		if(is_numeric($rid)) {
			$mib_array[$table_name]++;
		}
	}

	ksort($mib_array);

	print "<TABLE ".table_setup()."WIDTH=100%>\n";
	print "<TR>\n";
	print "<TD COLSPAN=$max_rows BGCOLOR=".get_scheme("INFONAME").">".locale("MIB_LIST")."</TD>";
	print "</TR>\n";
	foreach($mib_array as $k => $v) {
		if($column == 0) {
			print "<TR>\n";
		}
		$request_uri = preg_replace("/\/$/","",$_SERVER["REQUEST_URI"]);
		print "<TD WIDTH=$per_width%><A HREF=\"".$request_uri."/$k\">";
		print "$k ($v)</A></TD>\n";
		$column++;
		if($column >= $max_rows) {
			print "</TR>\n";
			$column = 0;
		} 
	}

	if($column != 0) { 
		while($column < $max_rows) {
			print "<TD WIDTH=$per_width%></TD>\n";
			$column++;
		}
		print "</TR>\n"; 
	}

	print "</TABLE>\n";

}

#
# display a list of hosts that have the requested
# mib name.
#
function display_mib_hosts($dbc,$mib_name) {

	$max_rows = 4;
	$per_width = 100 / $max_rows;

#	$selectQuery="SHOW TABLES LIKE '$mib_name\_%'";
        $selectQuery="SELECT tablename FROM table2iid_mapping WHERE tablename LIKE '".$mib_name."_%'";
	$selectResult=query($dbc,$selectQuery);
	$host_array = array();
	while($selectRow=fetch_array($selectResult)) {
		list($table_name,$rid) = split("_",$selectRow[0]);
		# make sure it's a stat with an rid at the end
		if(is_numeric($rid)) {
			$host_name = gethost($dbc,$rid);
			if($host_name != -1) {
				$host_array[$host_name] = $rid;
			}
		}
	}
	ksort($host_array);

	print "<TABLE ".table_setup()."WIDTH=100%>\n";
	print "<TR>\n";
	print "<TD COLSPAN=$max_rows BGCOLOR=".get_scheme("INFONAME").">".locale("MIB_HOSTS",$mib_name)."</TD>";
	print "</TR>\n";
	foreach($host_array as $k => $v) {
		if($column == 0) {
			print "<TR>\n";
		}
		$request_uri = preg_replace("/\/$/","",$_SERVER["REQUEST_URI"]);
		print "<TD WIDTH=$per_width%><A HREF=\"".$request_uri."/$k\">";
		print "$k</A></TD>\n";
		$column++;
		if($column >= $max_rows) {
			print "</TR>\n";
			$column = 0;
		} 
	}

	if($column != 0) { 
		while($column < $max_rows) {
			print "<TD WIDTH=$per_width%></TD>\n";
			$column++;
		}
		print "</TR>\n"; 
	}
	
	print "</TABLE>\n";

}

#
# display all interfaces for the requested mib name on
# the requested device
#
function display_mib_interfaces($dbc,$mib_name,$router) {

	$max_rows = 4;
	$per_width = 100 / $max_rows;

	$rid = getrid($dbc,$router);
	#$selectQuery="SELECT DISTINCT id from ".$mib_name."_$rid  WHERE dtime > DATE_SUB(SYSDATE(),INTERVAL 12 HOUR);";
#	$selectQuery="SELECT DISTINCT id from ".$mib_name."_$rid";
#	$selectResult=query($dbc,$selectQuery);
	$iid_array = array();
#	$iid_array["all"] = "All";
        $all_iid = "";
#	while($selectRow=fetch_object($selectResult)) {
#		$iid = $selectRow->id;
        $iid_list = get_active_ids($dbc,$rid,$mib_name."_$rid");
        foreach($iid_list as $iid => $v) {
                $selectResult2 = get_active_int($dbc,$rid,$iid);
#		$selectQuery="SELECT name, description from interface where id=\"$iid\" AND status=\"active\"";
#		$selectResult2=query($dbc,$selectQuery);
		if($selectResult2) {
#			$selectResult2 = fetch_object($selectResult2);
			$iid_array[$iid] = "$selectResult2->name($selectResult2->description)";
                        $all_iid.="$iid,";
		}

	}
        # remove trailing ,
	#$iid_array[rtrim($all_iid,",")] = "All";
	$iid_array[$all_iid] = "All";

	ksort($iid_array);
	
	print "<TABLE ".table_setup()." WIDTH=100%>\n";
	print "<TR>\n";
	print "<TD COLSPAN=$max_rows BGCOLOR=".get_scheme("INFONAME").">".locale("MIB_IIDS",$router,$mib_name)."</TD>";
	print "</TR>\n";

	foreach($iid_array as $k => $v) {
		if($column == 0) {
			print "<TR>\n";
		}
		$request_uri = preg_replace("/\/$/","",$_SERVER["REQUEST_URI"]);
		print "<TD WIDTH=$per_width%><A HREF=\"".$request_uri."/".rtrim($k,",")."\">";
		print "$v</A></TD>\n";
		$column++;
		if($column >= $max_rows) {
			print "</TR>\n";
			$column = 0;
		} 
	}

	if($column != 0) { 
		while($column < $max_rows) {
			print "<TD WIDTH=$per_width%></TD>\n";
			$column++;
		}
		print "</TR>\n"; 
	}
	print "</TABLE>\n";

}

#
# display the requested iids for the given device for
# the given mib
#
function display_mib_graph($dbc,$mib_name,$router,$iid) {

	$rid = getrid($dbc,$router);
	
	$selectQuery="SELECT graphname from mapping_table where tablename='$mib_name'";

	$selectResult=query($dbc,$selectQuery);

	$selectRow = fetch_object($selectResult);
	$graph_name = $selectRow->graphname;

#	$selectQuery="SELECT options from options_table where graphname=\"$graph_name\"";
#	$selectResult=query($dbc,$selectQuery);
#	$selectRow = fetch_object($selectResult);
        $selectRow = get_options_table($dbc,$graph_name);

	$graph_options = $selectRow->options;

	$args = "";
	$rtgplot = $_SESSION["rtgplot"];
        $iid_args="";
	$num_iids=0;
        if($newrtgplot=0) {
            foreach(explode(',',$iid) as $v) {
                $iid_args .= "iid=$v&amp;";
	        $num_iids++;
            }
	    $borderb = $num_iids * 11 + 37;
	    $args = $iid_args."t1=".$mib_name."_$rid";
	    $args .= "&amp;borderb=$borderb";

            foreach(explode(' ',$graph_options) as $v) {
                $args .= "&amp;$v";
            }
	
            $graph_len = $_SESSION["graph_date"];
	
            if($_SESSION["business_view"]) {
                $start_time = preg_split('/\./',$_SESSION["business_start"]);
		$end_time = preg_split("/\./",$_SESSION["business_end"]);
		$cur_date = getdate();
		# mon mday yday year
		$bt = mktime($start_time[0],$start_time[1],0,$cur_date["mon"],$cur_date["mday"],$cur_date["year"]);
		$et = mktime($end_time[0],$end_time[1],0,$cur_date["mon"],$cur_date["mday"],$cur_date["year"]);

	    } else {
		$et = time();
		$bt = $et - conv_our_time($graph_len);
	    }

	    if($_SESSION["day_offset"] > 0) {
                $bt = $bt - (86400 * $_SESSION["day_offset"]);
		$et = $et - (86400 * $_SESSION["day_offset"]);
            }

            $args .= "&amp;begin=$bt&amp;end=$et";
        } else {

        }

	$header_array = array();

	$header_array[locale("TABLE_HOST")] = $router;
	$header_array[locale("TABLE_SYSDESCR")] = getsysDescr($dbc,$host);
	#	$header_array["<HR />"]="<HR/>";
	$host_header = print_host_header($header_array);
	
	print "<TABLE ".table_setup()." WIDTH=100%>\n";
	print "<TR><TD COLSPAN=2>$host_header</TD></TR>\n";
        
	print "<TR><TD COLSPAN=2 ALIGN=left BGCOLOR=".get_scheme("STATBACK").">\n";
	print "<IMG SRC=\"$rtgplot?$args\" BORDER=0 /></TD></TR>\n";
       
	print "<TR><TD><TABLE WIDTH=100% ".table_setup().">\n";
	print "<TR><TD BGCOLOR=".get_scheme("STATNAME").">".locale("TABLE_INTERFACE")."</TD>\n";
	print "<TD BGCOLOR=".get_scheme("INTDESC").">".locale("TABLE_DESCRIPTION")."</TD></TR>\n";
	foreach(explode(',',$iid) as $v) {
		$selectQuery = "SELECT name,description from interface WHERE id=$v";
		$selectResult = query($dbc,$selectQuery);
		if($selectResult) {
			$selectRow = fetch_object($selectResult);
			$table_name = $mib_name."_$rid:$v";
			print "<TR><TD BGCOLOR=".get_scheme("INFONAME").">$table_name</TD>\n";
			print "<TD WIDTH=99% BGCOLOR=".get_scheme("INFOVALUE")."> $selectRow->name</TD>\n";
			print "</TR>\n";
			#$header_array[$table_name] = " $selectRow->name($selectRow->description)";
		}
	}
	print "</TABLE></TD></TR>\n";
	
	print "</TABLE>\n";

}

function displayhost($dbc,$host) {
	#printheader("$host");
	
	$rid = getrid($dbc,$host);
	if($rid == -1) {
		print "ERROR: $host is not in the database\n";
		return 1;
	}

        $table_list = get_table_list($dbc,$rid);
	#$selectQuery="SHOW TABLES LIKE '%\\_$rid'";
	#$selectResult=query($dbc,$selectQuery);
	$class_array = array();
	#while ($selectRow=fetch_array($selectResult)){
        foreach($table_list as $table_name => $v) {
#		list($table_name,$foo) = split("_",$selectRow[0]);
		$selectQuery="SELECT * from mapping_table where tablename='$table_name'";
		$selectResult2=query($dbc, $selectQuery);
		while ($selectRow2=fetch_object($selectResult2)) {
#			$selectQuery="SELECT * from options_table where graphname=\"$selectRow2->graphname\"";
#			$selectResult3=query($dbc,$selectQuery);
#			$selectRow3 = fetch_object($selectResult3,$dbc);
                        $selectRow3 = get_options_table($dbc,$selectRow2->graphname);
			list($module_name,$foo) = split("-",$selectRow2->graphname);
			$class_array[$selectRow3->class][$module_name] = 1;
			}
	}
	
	ksort($class_array);
	print "<TABLE ".table_setup()." WIDTH=100%>\n";
	$header_array[locale("TABLE_HOST")] = $host;
	$header_array[locale("TABLE_SYSDESCR")] = getsysDescr($dbc,$host);
	$host_header = print_host_header($header_array);

	print "<TR><TD>$host_header</TD></TR>\n";
	#print "$host_header</TR></TD>\n";
	print "<TR><TD ALIGN=center BGCOLOR=\"".get_scheme("CLASSLINE")."\">\n";
	foreach($class_array as $k => $v ) {
		print "<A HREF=\"".$_SERVER["SCRIPT_NAME"]."/stat/$k/$host\">[$k]</A>\n";
	}
	print "</TD></TR>\n";
	
	
        $loop_start = time();
	foreach($class_array as $k_array => $v_array) {
		foreach($v_array as $module => $v) {
                        debug(__FUNCTION__,"$k_array - $module - $v");
			# we need to get what will be graphed
			$html_string = stat_html_string($dbc,$host,$module,$k_array,$rid,$_SESSION["graph_date"]);


			if($html_string != "") {
				print "<TR>";
				# this was doing something wacky with formatting!
				#print "<TD WIDTH=45% ALIGN=left BGCOLOR=\"".get_scheme("STATNAME")."\"><A name=\"$k_array\">".locale("CLASSSTATS",$k_array)."</A></TD><TD WIDTH=99%></TD>";
				print "<TD WIDTH=45% ALIGN=left BGCOLOR=\"".get_scheme("STATNAME")."\"><A name=\"$k_array\">".locale("CLASSSTATS",$k_array)."</A></TD>";
				print "</TR>\n";
				print "<TR><TD>\n";
                                debug(__FUNCTION__,"html_string start");
				print "$html_string\n";
				debug(__FUNCTION__,"html_string insert end");
                                print "</TD></TR>\n";
			}
		}
	}
	print "</TABLE>\n";

}

#
# create html string to display graphs of stat
#
function stat_html_string($dbc,$host,$module,$class,$rid,$graph_len) {
        global $newrtgplot;

	$max_cols = 2;
	$bgcolor = get_scheme("INTDESC");
	$bgcolor2 = get_scheme("STATBACK");
	$bgcolor3 = get_scheme("STATCOLNAME");

        debug(__FUNCTION__,"Entered function");
	
	# make sure html_string is clean
	$html_string = "";

	$has_tables = get_table_list($dbc,$rid);

#	$selectQuery="SELECT graphname from options_table where class=\"$class\" and graphname like \"$module%\"";
#	$selectResult2=query($dbc,$selectQuery);
#	while($selectRow2 = fetch_object($selectResult2)) {
        $selectRow2 = get_options_table_w_class($dbc,"$module%",$class);
        foreach($selectRow2 as $graphname => $vv) { debug(__FUNCTION__,"$graphname == $vv");}
        foreach($selectRow2 as $graphname => $vv) {
#		$selectQuery="SELECT tablename from mapping_table where graphname=\"$selectRow2->graphname\"";
#		$selectResult=query($dbc,$selectQuery);
#		while($selectRow = fetch_object($selectResult)) {
                $selectRow = get_mapping_table($dbc,$graphname);
		foreach($selectRow as $table_name => $v) {
			#$f_table = $selectRow->tablename;
			$f_table = $table_name;
			debug(__FUNCTION__," [$stat] $f_table");
			if(isset($has_tables[$f_table])) {
				debug(__FUNCTION__,"yay! we found $f_table");
#				$selectQuery="SELECT DISTINCT id from ".$f_table."_$rid  WHERE dtime > DATE_SUB(SYSDATE(),INTERVAL 12 HOUR);";
				#$selectQuery="SELECT DISTINCT id from ".$f_table."_$rid";
#				$selectResult2=query($dbc,$selectQuery);
#				while($selectRow2 = fetch_object($selectResult2)) {
                                $active_iid_list = get_active_ids($dbc,$rid,$f_table."_$rid");
                                foreach($active_iid_list as $iid => $v) {
					#debug(__FUNCTION__,"found valid iid of $selectRow2->id");
					debug(__FUNCTION__,"found valid iid of $iid");
#					$selectQuery="SELECT id, name, description FROM interface WHERE id=$iid AND status=\"active\"";
#					$selectResult3=query($dbc, $selectQuery);
                                        $selectRow3 = get_active_int($dbc,$rid,$iid);
					if($selectRow3){
						debug(__FUNCTION__,"blee $selectRow3->id - $selectRow3->name");
						$iid_list[$selectRow3->id] = "$selectRow3->name ($selectRow3->description)";
					}
				}
			}
		}
	}

	#
	# if we didnt' find any iid's, we return nothing
	#
	if(!isset($iid_list)) { return ""; }

	if(sizeof($iid_list) > $_SESSION["compress_at"]) {
		$we_compress = 1;
	} else  { $we_compress = 0; }
	ksort($iid_list);

        $et = time();
        $bt = $et - conv_our_time($graph_len);

        if($_SESSION["show_date"]) {
                $show_date = preg_split('/-/',$_SESSION["cur_date"]);
                $et = mktime(23,59,59,$show_date[0],$show_date[1],$show_date[2]);
                $bt = $et - conv_our_time("24h") + 4;

#	} elseif($_SESSION["business_view"]) {
#		$start_time = preg_split('/\./',$_SESSION["business_start"]);
#		$end_time = preg_split("/\./",$_SESSION["business_end"]);
#		$cur_date = getdate();
#		# mon mday yday year
#		$bt = mktime($start_time[0],$start_time[1],0,$cur_date["mon"],$cur_date["mday"],$cur_date["year"]);
#		$et = mktime($end_time[0],$end_time[1],0,$cur_date["mon"],$cur_date["mday"],$cur_date["year"]);
	} elseif($_SESSION["day_offset"] > 0) {
		$bt = $bt - (86400 * $_SESSION["day_offset"]);
		$et = $et - (86400 * $_SESSION["day_offset"]);
	}

        if($_SESSION["business_view"]) {
		$start_time = preg_split('/\./',$_SESSION["business_start"]);
		$end_time = preg_split("/\./",$_SESSION["business_end"]);
		$cur_date = getdate($et);
		# mon mday yday year
		$bt = mktime($start_time[0],$start_time[1],0,$cur_date["mon"],$cur_date["mday"],$cur_date["year"]);
		$et = mktime($end_time[0],$end_time[1],0,$cur_date["mon"],$cur_date["mday"],$cur_date["year"]);
        }

#	if($_SESSION["day_offset"] > 0) {
#		$bt = $bt - (86400 * $_SESSION["day_offset"]);
#		$et = $et - (86400 * $_SESSION["day_offset"]);
#	}

#	$time_range="dtime>FROM_UNIXTIME($bt) AND dtime<=FROM_UNIXTIME($et)";

	# have we figured out the graph size to use?
	$have_graph_size = 0;
	# have we checked to see if we compact interfaces
	# into the same table row?
	$check_compact = 0;
	# do we compact interfaces into the same table row?
	$compact_rows = 0;

	$compress_col = array();

	# used as a counter for id's in compacting rows
	$z = 0; 
	foreach($iid_list as $k => $v) {
		$num_graphs = 0;
#		$selectQuery="SELECT description, name, speed FROM interface WHERE rid=$rid AND id=$k";
#		$selectResult2=query($dbc, $selectQuery);
#		$selectRow2=fetch_object($selectResult2);
                $selectRow2=get_active_int($dbc,$rid,$k);
		$interface_name = $selectRow2->name;
		$interface_description= $selectRow2->description;

		$stat_list = array();
		$options_list = array();
		#$selectQuery="SELECT graphname,options from options_table where class=\"$class\" and graphname like \"$module%\"";
		#$selectResult=query($dbc,$selectQuery);
		#while($selectRow = fetch_object($selectResult)) {
                $selectRow = get_options_table_w_class($dbc,"$module%",$class);
                foreach($selectRow as $graphname => $vv) {
			$found_table = 0;
#			$selectQuery="SELECT tablename from mapping_table where graphname=\"$selectRow->graphname\"";
#			$selectResult2=query($dbc,$selectQuery);
#			while($selectRow2 = fetch_object($selectResult2)) {
                        $selectRow2 = get_mapping_table($dbc,$graphname);
                        foreach($selectRow2 as $table_name => $v) {
				if($has_tables[$table_name]) {
					$found_table = 1;
				}
			}
			
			if($found_table > 0) {
				array_push($stat_list,$graphname);
				$options_list[$graphname] = $vv;
				$num_graphs++;
			}
		}

		# set the graph sizes so everything looks pretty and easy
		# to read as you scroll down a page
		if($have_graph_size == 0) {
			if($num_graphs > $max_cols) {
			        debug(__FUNCTION__,"num_graphs is $num_graphs");
				$max_cols = $num_graphs;
			}
			list($xplot,$yplot) = get_graph_size($max_cols);
			$have_graph_size = 1;
		}

		# do we compact rows to hvae more then one id?
		if($check_compact == 0) {
			if($num_graphs == 1) {
				$compact_rows = 1;
			}
			$check_compact = 1;
		}

		# only set the title rowif we'll be doing somethignin the foreach loop
		if(isset($stat_list[0])) {
			$table_header[$z] = locale("INTERFACETITLE",$interface_name,$interface_description,$module)."\n";
			
		}
	
		$y = 0;
		foreach($stat_list as $lstat) {
			$table_data[$z][$y] = "<A HREF=\"".$_SERVER["SCRIPT_NAME"]."/stat/$lstat/$host/$k\">";
			if($we_compress) {
				$table_data[$z][$y] .= "$interface_name ($interface_description)";
				$compress_col[$y] = "<I>$lstat</I>";
			} else {
				$table_data[$z][$y] .= create_img_tag($dbc,$lstat,$rid,$k,$xplot,$yplot,$bt,$et,chop($table_header[$z]));
			}
			$table_data[$z][$y] .= "</A>\n";
			$y++;
		}
		$z++;
	}

	$html_string .= "<TABLE ".table_setup()." WIDTH=100%>\n";
	if($we_compress) {
		$html_string .= "<TR><TD BGCOLOR=\"$bgcolor\" COLSPAN=$max_cols>".locale("COMPRESSTEXT",$_SESSION["compress_at"])."</TD>";
		$html_string .= "</TR>\n";
	}

	$col_num = 0;
	$stat_num = 0;
	if($compact_rows) {
		
		# little header if we are compressing data down to save space/speed things up
		if($we_compress) {
			$html_string .= "<TR>\n";
			$w=0;
			while($w++ < $max_cols) {
				$html_string .= "<TD BGCOLOR=\"$bgcolor3\">$compress_col[0]</TD>";
			}
			$html_string .= "</TR>\n";
		}

		while(isset($table_header[$stat_num])) {
			if($col_num == 0) {
				$html_string .= "<TR>\n";
			}

			if(!$we_compress) {
                                if($newrtgplot == 0) {
    				    $html_string .= "<TD BGCOLOR=\"$bgcolor\" ALIGN=left ><I>$table_header[$stat_num]</I></TD>\n";
                                } 
			}

			$col_num++;
			if($col_num >= $max_cols) {
				$html_string .= "</TR>\n";
				$col_num = 0;
				# print out the stats for the col
				$col_count = $stat_num - ($max_cols - 1 );
				$html_string .= "<TR>\n";
				while($col_count <= $stat_num) {
					$html_string .= "<TD BGCOLOR=\"$bgcolor2\">".$table_data[$col_count][0]."</TD>\n";
					$col_count++;
				}
				$html_string .= "</TR>\n";
			} 

			$stat_num++;
		}

		# if col_num isn't zero, we didn't finish...
		if($col_num != 0) {
			$col_num = 0;
			# print out the stats for the col
			$col_count = $stat_num - ($max_cols - 1 );
			$html_string .= "<TD BGCOLOR=\"$bgcolor\" COLSPAN=$col_count ></TD></TR>\n";
			$html_string .= "<TR>\n";
			while($col_count <= $stat_num) {
				$html_string .= "<TD BGCOLOR=\"$bgcolor2\">".$table_data[$col_count][0]."</TD>\n";
				$col_count++;
			}
			$html_string .= "</TR>\n";
		}

		
		
	} else {
		# little header if we are doing compressing on this stat
		if($we_compress) {
		#	print "<TR>\n";
                        $html_string .= "<TR>\n";
			$w=0;
			while($w < $max_cols) {
				$html_string .= "<TD BGCOLOR=\"$bgcolor3\">$compress_col[$w]</TD>";
				$w++;
			}
			#print "</TR>\n";
                        $html_string .= "</TR>\n";
		}
#		$html_string .= "<TR>\n";
		foreach($table_header as $z => $th) {
        		$html_string .= "<TR>\n";
			if(!$we_compress) {
                                if($newrtgplot == 0) {
				#$html_string .= "<TD BGCOLOR=\"$bgcolor\" COLSPAN=$max_cols ALIGN=left ><I>$th</I></TD></TR>\n";
				$html_string .= "<TD BGCOLOR=\"$bgcolor\" COLSPAN=$max_cols ALIGN=left ><I>$th</I></TD>\n";
                                }
#				$html_string .= "<TR>\n";
			}
			foreach($table_data[$z] as $td) {
				$html_string .= "<TD BGCOLOR=\"$bgcolor2\">$td</TD>\n";
			}
			$html_string .= "</TR>\n";
		}
	} 
	
#	$html_string .= "</TR>\n";


	
	$html_string .= "</TABLE>";

	return $html_string;
}

function create_img_tag($dbc,$stat,$rid,$iid,$xplot,$yplot,$bt,$et,$graph_title) {
        global $xplot_padding;
        global $yplot_padding;
        global $border_b;
        global $newrtgplot;

        $our_border_b = $border_b;

	$img_string = "";
	$table_list = array();

	$has_tables = get_table_list($dbc,$rid);

	$options_list = array();
#	$selectQuery="SELECT graphname,options from options_table where graphname=\"$stat\"";
#	$selectResult=query($dbc,$selectQuery);
        $selectRow = get_options_table($dbc,$stat);
	if($selectRow) {
		$options_list[$selectRow->graphname] = $selectRow->options;
	}
	
#	$time_range="dtime>FROM_UNIXTIME($bt) AND dtime<=FROM_UNIXTIME($et)";

	$x = 0;
	$args="";

        $DO = array();
        $LO = array();
#	$selectQuery="SELECT tablename from mapping_table where graphname=\"$stat\"";
#	$selectResult=query($dbc,$selectQuery);
#	while($selectRow = fetch_object($selectResult)) {
        $selectRow = get_mapping_table($dbc,$stat);
        foreach(array_reverse($selectRow) as $table_name => $v) {
		#$f_table = $selectRow->tablename;
		$f_table = $table_name;
		if(isset($has_tables[$f_table])) {
                        # We tried to only get interfaces that have real data in them, but this
                        # has proven too costly with SQL query times, so we are just going to trust
                        # the active tag in the interface table...
                        # $selectQuery="SELECT id from ".$f_table."_$rid where id=$iid AND $time_range LIMIT 1";
#			$selectQuery="SELECT id from interface where id=$iid AND status=\"active\"";
#			$selectResult2=query($dbc,$selectQuery);
#			$selectRow2 = fetch_array($selectResult2);
                        $selectRow2 = get_active_int($dbc,$rid,$iid);
			if(isset($selectRow2->id)) {
				$x++;
				$table_list["t$x"]=$f_table;
				debug(__FUNCTION__,"yay! we found $f_table for $stat");
                                if($newrtgplot == 1) {
                                    $DO[$x]="$x:".$f_table."_$rid:$iid";
		                    $graph_options = split(" ",$options_list[$stat]);
                                    $add_opts = "";
		                    foreach($graph_options as $g_opt) {
                                        if(preg_match("/factor/",$g_opt)) {
                                            $add_opts = $g_opt;
                                        }
                                    }
                                    $LO[$x]="$x:$x:$add_opts:$f_table";
                                } else {
                                    $args=$args."t$x=".$f_table."_$rid&amp;";
                                }
			} else {
				debug(__FUNCTION__,"$f_table isn't valid for this interface!");
			}	
		}
	}	

	if($x > 0) {
                if($newrtgplot == 1) {
		    $graph_options = split(" ",$options_list[$stat]);
		    foreach($graph_options as $g_opt) {
		        $g_opt = preg_replace("/%/","%25",$g_opt);
		        $g_opt = preg_replace("/:/","%3b",$g_opt);
                        if(!preg_match("/factor/",$g_opt)) {
                            $arg_add=$arg_add."$g_opt,";
                        }
                        if(preg_match("/^borderb/",$g_opt)) {
                            list($foo,$our_border_b) = explode("=",$g_opt);
                        }
		    }
                    $arg_add = substr("$arg_add", 0, -1);
		    $graph_title= preg_replace("/%/","%25",$graph_title);
		    $graph_title= preg_replace("/:/","%3B",$graph_title);
                    $PO="$graph_title:$xplot:$yplot:$arg_add:$bt:$et";
                    #$PO="Custom+Title:$xplot:$yplot::$bt:$et";
                    $args = "PO=$PO&amp;";
		    debug(__FUNCTION__,"[PO][$PO]");
                    foreach($DO as $foo) {
                        $args = $args."DO=$foo&amp;";
                        debug(__FUNCTION__,"[DO][$foo]");
                    }
                    foreach($LO as $foo) {
                        $args = $args."LO=$foo&amp;";
                        debug(__FUNCTION__,"[LO][$foo]");
                    }
                    #$args = substr("$args", 0, -1);
                    $args = preg_replace("/\&amp\;$/","",$args);
		    $args = preg_replace("/ /","+",$args);
		    $args = preg_replace("/#/","%23",$args);
		    $args = preg_replace("/!/","%21",$args);
		    debug(__FUNCTION__,"[$stat][$args]");

                } else {
		    $args=$args."iid=$iid&amp;xplot=$xplot&amp;yplot=$yplot&amp;";
		    $graph_options = split(" ",$options_list[$stat]);
		    foreach($graph_options as $g_opt) {
			$args=$args."$g_opt&amp;";
                        if(preg_match("/^borderb/",$g_opt)) {
                            list($foo,$our_border_b) = explode("=",$g_opt);
                        }
		    }
		    $args = $args."begin=$bt&amp;end=$et";
		    debug(__FUNCTION__,"[$stat][$args]");
		    $args = preg_replace("/%/","%25",$args);
                }

                $args = preg_replace("/\&amp\;$/","",$args);

                # This is for IMG SRC size tag which allows us
                # to completely build out the tables that hold the graphs 
                # before the images load, makes a much nicer page load.
                $graph_wide = $xplot + $xplot_padding;
                $graph_high = $yplot + $yplot_padding + $our_border_b;

		$rtgplot = $_SESSION["rtgplot"];
		$img_string = "<IMG WIDTH=$graph_wide HEIGHT=$graph_high SRC=$rtgplot?$args BORDER=0 />";
	}


	return $img_string;
}


#
# get rid of host
#
function getrid($dbc,$host) {
	$selectQuery="SELECT rid from router where name='$host'";
	$selectResult=query($dbc,$selectQuery);
	if($selectRow=fetch_array($selectResult)) {
		return $selectRow[0];
	} else {
		return -1;
	}
}

#
# get name of host from rid
# 
function gethost($dbc,$rid) {
	$selectQuery="SELECT name from router where rid='$rid'";
	$selectResult=query($dbc,$selectQuery);
	if($selectRow=fetch_array($selectResult)) {
		return $selectRow[0];
	} else {
		return -1;
	}
}
#
#
# get sysDescr
#
function getsysDescr($dbc,$host) {
	$rid=getrid($dbc,$host);
	$selectQuery="SELECT description from interface WHERE rid='$rid' AND name='sysDescr'";
	$selectResult=query($dbc,$selectQuery);
	$selectRow=fetch_object($selectResult);

	return $selectRow->description;
}

function is_host_active($dbc,$rid) {
        static $router_cache = array();

        if(count($router_cache) > 0) {
            debug(__FUNCTION__,"$rid's active is cached, using them...");
        } else {
            debug(__FUNCTION__,"$rid's active is not cached, querying...");
            # we check to see if it's active by looking for sysDescr in the 
            # interface file and seeing if it's active
#           $selectQuery="SELECT rid FROM interface WHERE status=\"active\" AND name=\"sysDescr\"";
            $query_date = time() - (100 * 60 * 60);
            #$selectQuery="SELECT b.rid FROM table2iid_mapping AS a, interface AS b, router AS c WHERE a.tablename LIKE 'ifInOctets_%' AND a.dtime > $query_date AND b.id = a.id AND c.rid = b.rid ORDER BY b.rid";
            $selectQuery="SELECT interface.rid FROM interface,table2iid_mapping,router WHERE table2iid_mapping.tablename LIKE 'ifInOctets_%' AND table2iid_mapping.dtime > $query_date AND interface.id = table2iid_mapping.id AND router.rid = interface.rid ORDER BY interface.rid";
            $selectResult=query($dbc,$selectQuery);
            while($selectRow = fetch_object($selectResult)) {
                $router_cache[$selectRow->rid] = 1;
            } 
        }

        return $router_cache[$rid];
}


#
# return list of tables for the host
#
function get_table_list($dbc,$rid) {
        static $table_cache = array();
        $table_list = array();

        # threshold for doing full table dump
        $full_dump_th = 3;

        if( count($table_cache[$rid]) > 0 || count($table_cache) > $full_dump_th ) {
            debug(__FUNCTION__,"$rid's tables are cached, using them...");
        } else {
            debug(__FUNCTION__,"$rid's tables are not cached, querying...");
            #$selectQuery="SHOW TABLES LIKE '%\\_%'";
            # We are doing id,tablename in this with hopes of caching the later call
            # of the same select, but in a different function, with different output 
            # types, and i've yet to find an elegant way to combine the two, so
            # hopes of query cacheing for speed is being done here.
            $num_dev = num_of_devices(0);
            $query_date = time() - (68 * 60 * 60);
            if($num_dev > $full_dump_th) {
                debug(__FUNCTION__,"We have more then $full_dump_th devices, grabbing entire table2iid_mapping table");
                $selectQuery="SELECT id,tablename FROM table2iid_mapping WHERE tablename LIKE '%_%' and dtime > $query_date)";

            } else {
                $selectQuery="SELECT id,tablename FROM table2iid_mapping WHERE tablename LIKE '%_$rid' and dtime > $query_date";
            }
            $selectResult=query($dbc, $selectQuery);
            while($selectRow = fetch_object($selectResult)) {
                list($table_name,$table_rid) = split("_",$selectRow->tablename);
                $table_cache[$table_rid][$table_name] = 1;
            }
        }

        # make sure we return an array even if there isn't anything in it
        # otherwise we get complaints from php if someone tries to use the 
        # result as an array
        if(count($table_cache[$rid]) <= 0) { $table_cache[$rid] = array(); }

    return $table_cache[$rid];
}

#
# get seconds from our graph_len string
#
function conv_our_time($graph_len) {

	$time_mod = "";
	$time_format = substr($graph_len,-1);
        $time_num = substr($graph_len,0,-1);
        if($time_format == "h") {
		$time_mod = $time_num * 60 * 60;
	} elseif($time_format== "d") {
		$time_mod = $time_num * 24 * 60 * 60;
	} elseif($time_format == "w") {
		$time_mod = $time_num * 7 * 24 * 60 * 60;
	} elseif($time_format == "m") {
		$time_mod = $time_num * 30 * 24 * 60 * 60;
	} elseif($time_format == "y") {
		$time_mod = $time_num * 365 * 24 * 60 * 60;
	}
	else {
		$time_mod = $time_num;
	}
	
	return $time_mod;
}

function get_active_ids($dbc,$rid,$table) {
        static $iid_cache = array();

        # threshold for doing full table dump
        $full_dump_th = 3;

        if(count($iid_cache[$rid]) > 0 || count($iid_cache) > $full_dump_th) {
            debug(__FUNCTION__,"table $table active id info is cached, using it...");
        } else {
            debug(__FUNCTION__,"table $table active id info is not cached, querying...");
            # $selectQuery="SELECT * FROM interface WHERE rid=$rid AND status=\"active\"";
            # $selectQuery="SELECT DISTINCT id from $table WHERE dtime > DATE_SUB(SYSDATE(),INTERVAL 24 HOUR)";
            # we are doing 28 hours to give some leway for targetmaker running for a long time...
            $num_dev = num_of_devices(0);

            $query_date = time() - (68 * 60 * 60);
            if($num_dev > $full_dump_th) {
                debug(__FUNCTION__,"We have more then $full_dump_th devices, grabbing entire table2iid_mapping table");
                $selectQuery="SELECT id,tablename FROM table2iid_mapping WHERE tablename LIKE '%_%' and dtime > $query_date";
            } else {
                $selectQuery="SELECT id,tablename FROM table2iid_mapping WHERE tablename LIKE '%_$rid' and dtime > $query_date";
            }
            $selectResult=query($dbc, $selectQuery);
            while($selectRow = fetch_object($selectResult)) {
                list($foo,$t_rid) = split("_",$selectRow->tablename);
                $iid_cache[$t_rid][$selectRow->tablename][$selectRow->id]++;
            }
        }

        if(count($iid_cache[$rid][$table]) <= 0) { $iid_cache[$rid][$table] = array(); }

        return $iid_cache[$rid][$table];
}


function get_active_int($dbc,$rid,$iid) {
        static $iid_cache = array();

        if(count($iid_cache[$rid]) > 0) {
            debug(__FUNCTION__,"int $iid's info is cached, using it...");
        } else {
            debug(__FUNCTION__,"int $iid's info is not cached, querying...");
            #$selectQuery="SELECT * FROM interface WHERE id=$iid AND status=\"active\"";
            $selectQuery="SELECT * FROM interface WHERE rid=$rid AND status='active'";
            $selectResult=query($dbc, $selectQuery);
            while($selectRow = fetch_object($selectResult)) {
                #$iid_cache[$iid] = $selectRow;
                $iid_cache[$rid][$selectRow->id] = $selectRow;
            }
        }

        return $iid_cache[$rid][$iid];
}

function get_options_table($dbc,$graphname) {
        static $options_cache = array();

        if($options_cache[$graphname]) {
            debug(__FUNCTION__,"$graphname options info is cached, using it...");
        } else {
            debug(__FUNCTION__,"$graphname options info is not cached, querying...");
            $selectQuery="SELECT * FROM options_table WHERE graphname LIKE '$graphname'";
            $selectResult=query($dbc, $selectQuery);
            if($selectRow = fetch_object($selectResult)) {
                $options_cache[$graphname] = $selectRow;
            }
        }

        return $options_cache[$graphname];
}

function get_options_table_w_class($dbc,$graphname,$class) {
        static $options_cache = array();

        if(count($options_cache[$graphname][$class]) > 0) {
            debug(__FUNCTION__,"$graphname with $class options info is cached, using it...");
        } else {
            debug(__FUNCTION__,"$graphname with $class options info is not cached, querying...");
            $selectQuery="SELECT * FROM options_table WHERE graphname LIKE '$graphname' AND class='$class'";
            $selectResult=query($dbc, $selectQuery);
            $options_cache[$graphname][$class] = array();
            while($selectRow = fetch_object($selectResult)) {
                $options_cache[$graphname][$class][$selectRow->graphname] = $selectRow->options;
            }
        }

        return $options_cache[$graphname][$class];
}

function get_mapping_table($dbc,$graphname) {
        static $mapping_cache = array();
        $mapping_array = array();

        if(count($mapping_cache) > 0) {
            debug(__FUNCTION__,"$graphname mapping info is cached, using it...");
        } else {
            debug(__FUNCTION__,"$graphname mapping info is not cached, querying...");
            $selectQuery="SELECT * FROM mapping_table";
            $selectResult=query($dbc, $selectQuery);
            while($selectRow = fetch_object($selectResult)) {
                $mapping_cache[$selectRow->graphname][$selectRow->tablename] = 1;
            }
        }

        return $mapping_cache[$graphname];
}

function num_of_devices($num_dev) {
        static $total_devices;

        if($num_dev > 0) {
            $total_devices += $num_dev;
            debug(__FUNCTION__,"Adding $num_dev to number of devices, total $total_devices");
        }

        return $total_devices;
}

function get_mapping_table_by_tablename($dbc,$tablename) {
        static $mapping_cache = array();

        if($mapping_cache[$tablename]) {
            debug(__FUNCTION__,"$tablename mapping info is cached, using it...");
        } else {
            debug(__FUNCTION__,"$tablename mapping info is not cached, querying...");
            $selectQuery="SELECT * FROM mapping_table WHERE tablename='$tablename'";
            $selectResult=query($dbc, $selectQuery);
            if($selectRow = fetch_object($selectResult)) {
                $mapping_cache[$tablename] = $selectRow;
            }
        }

        return $mapping_cache[$tablename];
}

?>
