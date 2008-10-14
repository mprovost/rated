<?
/*
 *******************************************************************************
 * Take old targets format: "host\toid\tbits\tcommunity\ttable\tid\tspeed\tdesc"
 * and convert to:
 * host [host] {
 * 	community;
 * 	snmpver;
 *	target [oid] {
 * 		bits;
 *		table;
 * 		id;
 *		speed;
 *		description;
 * 	};
 * };
 * The old RTG system could only handle one SNMP Version per daemon, but now
 * it can handle both simultaneously.
 * The old RTG system allowed user's to skip the speed data per-target...
 * this is required information for the new system because of the confusion
 * it caused when no present.
 *******************************************************************************
 *
 * 
 * Set configuration files and speed default.
 * $usedefault: When 0, error when finding a target entry with no speed.
 *              When 1, use the $default_speed value.
 * Make two arrays of files. One with targets being polled with SNMP V2, and
 * the other with SNMP V1.
 */

$v1_targetfiles = array();
$v2_targetfiles = array();
$v1_targetfiles[] = "/usr/local/rtg/etc/targets.cfg";
//$v2_targetfiles[] = "/usr/local/v2rtg/etc/targets.cfg";
$usedefault = 1;
$default_speed = 100000000;
/*
 * Print this to a file that won't just overwrite targets.cfg.
 * No matter how many daemons are running for different SNMP versions,
 * poll times, etc. One can simply cat rtg0_8.targets.cfg >> targets.cfg
 * at his/her leisure.
 */
$output = "./rtg0_8targets.cfg";
$targets = array();

function buildHosts(&$targets,$elements, $ver) {
    $rname = $elements[0];
    $targets[$rname]['community'] = $elements[3];
    $targets[$rname]['snmpver'] = $ver;
    $targets[$rname]['targets'][$elements[1]] = array();
    $targets[$rname]['targets'][$elements[1]]['bits'] = $elements[2];
    $targets[$rname]['targets'][$elements[1]]['table'] = $elements[4];
    $targets[$rname]['targets'][$elements[1]]['id'] = $elements[5];
    $targets[$rname]['targets'][$elements[1]]['speed'] = $elements[6];
    $targets[$rname]['targets'][$elements[1]]['descr'] = $elements[7];
}
function parseLine(&$elements, $lineno, $input) {
    global $usedefault, $default_speed;
    if (sizeof($elements) != 8) {
        /*
         * We do not have an speed value.
         */
        if (!$usedefault) {
            die("Missing speed in $input on line $lineno\n");
        }
        else {
            array_push($elements, $elements[6]);
            /*
             * The array should now contain 7 elements.
             */
            $elements[6] = $default_speed;
            if (sizeof($elements) != 8) {
                /*
                 * This line was malformed, regardless of speed.
                 * How did you get the poller started to begin with?
                 */
                die("Malformed line in $input on line $lineno\n");
            }
            $elements[6] = $default_speed;
        }
    }
}

function parseFile(&$targets, $input, $ver) {
    if (!($fin = fopen($input, "r"))) {
        die("Failed to open $input for reading.\n");
    }
    else {
        $lineno = 0;
        while(!feof($fin)) {
            $elements = array();
            $old_line = trim(fgets($fin));
            $lineno++;
            if (empty($old_line)) {
                continue;
            }
            if (strstr($old_line, "#")) {
                continue;
            }
            $elements = explode("\t", $old_line);
            parseLine($elements, $lineno, $input);
            /*
             * We have a valid line.
             */
            buildHosts($targets, $elements, $ver);
        }/* while(!feof($fin)) */
        fclose($fin);
    }
}
if (!($fout = fopen($output, "w"))) {
    die("Failed to open $output for writing.\n");
}
else {
    foreach ($v1_targetfiles as $input) {
        parseFile($targets, $input, 1);
    }

    foreach ($v2_targetfiles as $input) {
        parseFile($targets, $input, 2);
    }

    foreach ($targets as $rname => $host_array) {
        fwrite($fout, "host $rname {\n");
        fwrite($fout, "\tcommunity " . $host_array['community'] .
                      ";\n");
        fwrite($fout, "\tsnmpver " . $host_array['snmpver'] . ";\n");
        foreach ($host_array['targets'] as $oid => $oid_array) {
            fwrite($fout, "\ttarget $oid {\n");
            foreach ($oid_array as $key => $val) {
                if (!strcmp($key, "descr")) {
                    fwrite($fout, "\t\t$key \"$val\";\n");
                }
                else {
                    fwrite($fout, "\t\t$key $val;\n");
                }
            }
            fwrite($fout, "\t};\n");
        }
        fwrite($fout, "};\n");
    }
    fclose($fout);
}
?>
