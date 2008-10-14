<?php
# $Id: view.php,v 1.1 2008/01/19 03:22:15 btoneill Exp $
# $Author: btoneill $
# $Date: 2008/01/19 03:22:15 $
#####################################################################
#
#
#

include('./common.php');
include('./view.inc');


# Send headers 
$expireTime = 60*60*24*7; // 7 days

#
# These are for default rtgplot data, change
# if you change the defaults. This is the size of
# the space around the graph in each image
#
$border_b = 70; # this is the bottom area for the legend, default is 70
$xplot_padding = 70;
$yplot_padding = 20;

# height for the yplot area (note, this is not dynamic
# based on screen resolution).
$y_def_height = 100;

session_set_cookie_params($expireTime);
session_start();
session_name("targetmaker");

$config_file = "./view.cfg";
$locale_dir = "./locales";
$scheme_dir = "./schemes";

$_SESSION["rtgplot"] = $rtgplot;


$VIEWVERSION='$Revision: 1.1 $';

$total_sql_time = 0;
$php_start_time = time();

#
# read and parse our config file
#
$config_file_contents = file($config_file);
foreach($config_file_contents as $fl) {
	$fl = rtrim($fl);
	if(preg_match("/^#.*/",$fl)) { continue; }
	$config_values = preg_split('/:/',$fl);
	$cookie_values[$config_values[0]]["default"] = $config_values[1];
	$cookie_values[$config_values[0]]["values"] = preg_split('/,/',$config_values[2]);
	$cookie_values[$config_values[0]]["name"] = $config_values[3];
	$cookie_values[$config_values[0]]["description"] = $config_values[4];
	if(!isset($_SESSION[$config_values[0]])) {
		$_SESSION[$config_values[0]] = $config_values[1];
	}
}

$locale_file = "$locale_dir/".$_SESSION["locale"].".loc";
$locale_file_contents = file($locale_file);
foreach($locale_file_contents as $fl) {
	$fl = rtrim($fl);
	if(preg_match("/^#.*/",$fl)) { continue; }
	$locale_values = preg_split('/=/',$fl,2);
	$locale_string[$locale_values[0]] = $locale_values[1];
}

$scheme_file = "$scheme_dir/".$_SESSION["color_scheme"];
$scheme_file_contents = file($scheme_file);
foreach($scheme_file_contents as $fl) {
	$fl = rtrim($fl);
	if(preg_match("/^#.*/",$fl)) { continue; }
	$scheme_values = preg_split('/=/',$fl,2);
	$scheme[$scheme_values[0]] = $scheme_values[1];
}


if($_SERVER["QUERY_STRING"]) {
    $query_values = preg_split('/&/',$_SERVER["QUERY_STRING"]);
    foreach($query_values as $v) {
        $var_values = preg_split('/=/',$v);
        #debug(__FUNCTION__,"1:[$var_values[0]] 2:[$var_values[1]]");
        if($var_values[0] == "date") {
            #debug(__FUNCTION__,"whee");
            $_SESSION["show_date"] = 1;
            $_SESSION["cur_date"] = $var_values[1];
        #    $_SESSION["day_offset"] = 0;
#            $_SESSION["graph_date"] = "24h";
        }
        if($var_values[0] == "nodate") {
            #debug(__FUNCTION__,"whee");
            $_SESSION["show_date"] = 0;
        }
    }

}

$refresh = $_SESSION["autorefresh"];

/* Connect to RTG MySQL Database */
if($pgsql) {
# do postgres connect stuff
} elseif($oracle) {
# do oracle connect stuff
    $dbc = oci_connect($user,$pass,$db);
    if(!$dbc) {
        $e = oci_error();
        die($e['message']);
    }
} else {
    $dbc=mysql_connect ($host, $user, $pass) or
    $dbc=mysql_connect ("$host:/var/lib/mysql/mysql.sock", $user, $pass) or 
       die (locale(MYSQL_CONNECT_FAIL));
    mysql_select_db($db,$dbc);
}    

$path_array = split("/",$_SERVER["PATH_INFO"]);


$update_last = 0;
if($path_array[1] == "host") {
	print_headers($VERSION,$_SERVER["REQUEST_URI"],$refresh,$path_array[2]);
	if($path_array[2]) {
		displayhost($dbc,$path_array[2]);
		$update_last = 1;
	} else { 
		listhosts($dbc);		
	}

} elseif($path_array[1] == "stat") {
	print_headers($VERSION,$_SERVER["REQUEST_URI"],$refresh,"$path_array[3]:$path_array[2]:$path_array[4]");
  	if($path_array[2] && $path_array[3] && $path_array[4]) {
		displaystats($dbc,$path_array[2],$path_array[3],$path_array[4]);
		$update_last = 1;
  	} elseif($path_array[2] && $path_array[3]) {
		displayclass($dbc,$path_array[2],$path_array[3]);
		$update_last = 1;
  	} elseif($path_array[2]) {
		stathosts($dbc,$path_array[2]);
		$update_last = 1;
	} else {
		liststats($dbc);
	}
} elseif($path_array[1] == "mib") {
	print_headers($VERSION,$_SERVER["REQUEST_URI"],$refresh,"");
	$update_last = 1;
	if($path_array[2] && $path_array[3] && $path_array[4]) {
		display_mib_graph($dbc,$path_array[2],$path_array[3],$path_array[4]);
	} elseif($path_array[2] && $path_array[3]) {
		display_mib_interfaces($dbc,$path_array[2],$path_array[3]);
	} elseif($path_array[2]) {
		display_mib_hosts($dbc,$path_array[2]);
	} else {
		display_mibs($dbc);
	}
} elseif($path_array[1] != "") {

	$bg1=get_scheme("CONFIGNAME");
	$bg2=get_scheme("CONFIGVALUE");
	if($path_array[1] == "config") {
		print_headers($VERSION,$_SERVER["REQUEST_URI"],$refresh,"");
		print "<TABLE BORDER=0 CELLPADDING=0 CELLSPACING=0 WIDTH=100%>\n";
		print "<TR><TD BGCOLOR=\"$bg1\" WIDTH=15%><B>".locale("VARNAME")."</B></TD>\n";
		print "<TD BGCOLOR=\"$bg2\"><B>".locale("VARCURRENT")."</B></TD></TR>\n";
		foreach($cookie_values as $k => $v) {
			print "<TR><TD BGCOLOR=\"$bg1\">\n";
			print "<A HREF=\"".$_SERVER["SCRIPT_NAME"]."/$k\">".locale($cookie_values[$k]["name"])."</A>\n";
			print "</TD><TD BGCOLOR=\"$bg2\">".$_SESSION[$k]."</TD>\n";
			print "</TD></TR>\n";

		}
		print "</TABLE>\n";

	} elseif(isset($cookie_values[$path_array[1]])) {
		if(isset($path_array[2])) {
			$_SESSION[$path_array[1]] = $path_array[2];
			if($_SERVER["REQUEST_URI"] != $_SESSION["ref_page"]) {
				print_headers($VERSION,$_SESSION["ref_page"],0,"");
			} else {
				print_headers($VERSION,$_SERVER["REQUEST_URI"],$_SESSION["autorefresh"],"");
			}
		} elseif(is_array($cookie_values[$path_array[1]]["values"])) {
			print_headers($VERSION,$_SERVER["REQUEST_URI"],$_SESSION["autorefresh"],"");
			print "<TABLE BORDER=0 CELLPADDING=0 CELLSPACING=0 WIDTH=100%>\n";
			print "<TR><TD BGCOLOR=\"$bg1\" WIDTH=15%>\n";
		#	print "<TR><TD BGCOLOR=\"$bg1\">\n";
			print locale("VARNAME").":</TD><TD BGCOLOR=\"$bg2\">".locale($cookie_values[$path_array[1]]["name"])."<BR>\n";
			print "</TD></TR><TR><TD BGCOLOR=\"$bg1\">\n";
			print locale("VARCOOKIENAME").":</TD><TD BGCOLOR=\"$bg2\">".$path_array[1]."<BR>\n";
			print "</TD></TR><TR><TD BGCOLOR=\"$bg1\">\n";
			print locale("VARDESCRIPTION").":</TD><TD BGCOLOR=\"$bg2\">".locale($cookie_values[$path_array[1]]["description"])."<BR>\n";
			print "</TD></TR><TR><TD BGCOLOR=\"$bg1\">\n";
			print locale("VARDEFAULT").":</TD><TD BGCOLOR=\"$bg2\">".$cookie_values[$path_array[1]]["default"]."<BR>\n";
			print "</TD></TR><TR><TD BGCOLOR=\"$bg1\">\n";
			print locale("VARCURRENT").":</TD><TD BGCOLOR=\"$bg2\">".$_SESSION["$path_array[1]"]."<BR>\n";
			print "</TD></TR><TR><TD BGCOLOR=\"$bg1\">\n";

			print locale("VAROPTIONS").":</TD><TD BGCOLOR=\"$bg2\">\n";
			foreach($cookie_values[$path_array[1]]["values"] as $val) {
				$icb = $ice = $idb = $ide = "";
				if($val == $_SESSION[$path_array[1]]) {
					$icb = "<B>";
					$ice = "</B>";
				}
				if($val == $cookie_values[$path_array[1]]["default"]) {
					$idb = "<I>";
					$ide = "</I>";
				}
				print "<A HREF=\"".$_SERVER["SCRIPT_NAME"]."/".$path_array[1]."/$val\">$icb$idb [$val]$ice$ide</A>\n";
			}
			print "</TD></TR></TABLE>\n";
		} else {
			print_headers($VERSION,$_SERVER["REQUEST_URI"],$_SESSION["autorefresh"],"");
			print "Sorry, we aren't handling text cookie values yet!\n";
		}

	} else {
		print_headers($VERSION,$_SERVER["REQUEST_URI"],$refresh,"");
		print locale("VALIDOPTIONVAR",$path_array[1])."\n";
	}
} else {
	print_headers($VERSION,$_SERVER["REQUEST_URI"],$refresh,"");
	print "<P><A HREF=\"".$_SERVER["SCRIPT_NAME"]."/host\">".locale("VIEWHOSTS")."</A>\n";
	print "<P><A HREF=\"".$_SERVER["SCRIPT_NAME"]."/stat\">".locale("VIEWSTATS")."</A>\n";
}

$php_end_time = time();
$php_total_time = $php_end_time - $php_start_time;
debug(__FUNCTION__,"total sql time: $total_sql_time");
debug(__FUNCTION__,"total php time: $php_total_time");

print_footer($VERSION);
if($update_last) {
	$_SESSION["last_real"] = $_SERVER["REQUEST_URI"];
}

?>
</BODY>
</HTML>
