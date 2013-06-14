<?php
$ROOT_FOLDER = dirname($argv[0]) . "/..";
include_once($ROOT_FOLDER . "/sdk/php/stats_rrdb.php");

$CONFIG_FILE = "$ROOT_FOLDER/stats-rrdb.conf";
$PID_FILE = "/tmp/stats-rrdb.pid";

function start_server() {
	global $ROOT_FOLDER, $CONFIG_FILE, $PID_FILE;

	echo "=================== Starting server\n";
	system("valgrind $ROOT_FOLDER/src/statsrrdb  --config \"$CONFIG_FILE\" --daemon \"$PID_FILE\"");
	$pid=file_get_contents($PID_FILE);
	echo "=================== Started server $pid\n";
}
function stop_server() {
	global $PID_FILE;

	echo "=================== Stopping server\n";
        $pid=file_get_contents($PID_FILE);
	system("kill $pid");
	echo "=================== Stopped server $pid\n";

}

function restart_server() {
	stop_server();
	start_server();
}

function check_result($actual, $expected) {
	if($actual == $expected) {
		echo "MATCH: $actual\n";
	} else {
		echo "!!!!! NO MATCH: '$actual' (expected '$expected')\n";
	}
}

restart_server();

try {
	// create
	$mode = isset($argv[1]) ? $argv[1] : StatsRRDB::Mode_Tcp;
	echo "Starting mode {$mode}\n";
	$stats_rrdb = new StatsRRDB($mode);

	echo "== CREATE METRIC 'test1' \n";
	$resp = $stats_rrdb->send_command("create metric 'test1' keep 5 secs for 30 sec, 10 secs for 1 min, 30 secs for 10 min;"); 
	check_result($resp, "OK");

	echo "== CREATE METRIC 'test2' \n";
	$resp = $stats_rrdb->send_command("create metric 'test2' keep 10 secs for 1 week, 1 min for 1 month, 10 min for 1 year, 30 min for 10 years;");
	check_result($resp, "OK");

	echo "== SHOW METRICS\n";
	$resp = $stats_rrdb->send_command("show metrics like 'te';");
	check_result($resp, "test2;test1;");

	for($ii = 0; $ii < 20; $ii++) {
		echo "== UPDATE METRIC 'test1' $ii -> 1\n";
		$ts = 1371104586 + $ii;
		$resp = $stats_rrdb->send_command("UPDATE 'test2' ADD 1 AT $ts ;");
		check_result($resp, "OK");
	}	

	echo "== SELECT * FROM METRIC 'test1'\n";
	$resp = $stats_rrdb->send_command("SELECT * FROM 'test1' BETWEEN 1371104580 AND 1371104990 ;");
	check_result($resp, "ts,count,sum,sum_sqr,min,max\n");

	echo "== SELECT * FROM METRIC 'test2'\n";
	$resp = $stats_rrdb->send_command("SELECT * FROM 'test2' BETWEEN 1371104580 AND 1371104990 ;");
	// check_result($resp, "ts,count,sum,sum_sqr,min,max\n1371104580,2,3,5,1,2\n");
	echo "$resp\n";
	
	restart_server();

	echo "== SHOW METRICS\n";
    $resp = $stats_rrdb->send_command("show metrics like 'te';");
	check_result($resp, "test1;test2;");

	echo "== SHOW METRIC POLICY\n";
	$resp = $stats_rrdb->send_command("show metric policy 'test1';");
	check_result($resp, "5 secs for 30 secs, 10 secs for 1 min, 30 secs for 10 mins");

    echo "== SHOW METRIC POLICY\n";
    $resp = $stats_rrdb->send_command("show metric policy 'test2';");
	check_result($resp, "10 secs for 1 week, 1 min for 1 month, 10 mins for 1 year, 30 mins for 10 years");
    
	for($ii = 0; $ii < 20; $ii++) {
		echo "== UPDATE METRIC 'test1' $ii -> 1\n";
		$ts = 1371104586 + 30 + $ii;
		$resp = $stats_rrdb->send_command("UPDATE 'test2' ADD 1 AT $ts ;");
		check_result($resp, "OK");
	}

	echo "== SELECT * FROM METRIC 'test1'\n";
	$resp = $stats_rrdb->send_command("SELECT * FROM 'test1' BETWEEN 1371104580 AND 1371104990 ;");
	check_result($resp, "ts,count,sum,sum_sqr,min,max\n");

	echo "== SELECT * FROM METRIC 'test2'\n";
	$resp = $stats_rrdb->send_command("SELECT * FROM  metric 'test2' BETWEEN 1371104580 AND 1371104990 ;");
	// check_result($resp, "ts,count,sum,sum_sqr,min,max\n1371104580,3,6,14,1,3\n");
	echo "$resp\n";

	echo "== SHOW METRIC (error)\n";
	$resp = $stats_rrdb->send_command("show metric 'test';");
	check_result($resp, "ERROR: Unable to parse the statement");

	echo "== DROP METRIC 'test1'\n";
	$resp = $stats_rrdb->send_command("drop metric 'test1';");
	check_result($resp, "OK");

	echo "== DROP METRIC 'test2'\n";
	$resp = $stats_rrdb->send_command("drop metric 'test2';");
	check_result($resp, "OK");
} catch(Exception $e) {
	echo "Exception: " . $e->getMessage() . "\n";
}

stop_server();


