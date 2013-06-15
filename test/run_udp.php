<?php
$ROOT_FOLDER = dirname($argv[0]) . "/..";
include_once($ROOT_FOLDER . "/sdk/php/stats_rrdb.php");

$CONFIG_FILE = "$ROOT_FOLDER/stats-rrdb.conf";
$PID_FILE = "/tmp/stats-rrdb.pid";

function start_server() {
	global $ROOT_FOLDER, $CONFIG_FILE, $PID_FILE;

	echo "=================== Starting server\n";
	system("$ROOT_FOLDER/src/statsrrdb  --config \"$CONFIG_FILE\" --daemon \"$PID_FILE\"");
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
	$stats_rrdb_udp = new StatsRRDB(StatsRRDB::Mode_Udp);
	$stats_rrdb_tcp = new StatsRRDB(StatsRRDB::Mode_Tcp);
	
	// 
	// Create
	//
	echo "== CREATE METRIC 'test1' \n";
	$resp = $stats_rrdb_udp->send_command("C|test1|5 secs for 10 sec, 10 secs for 20 secs, 20 secs for 10 min"); 
	check_result($resp, "OK");

	echo "== CREATE METRIC 'test2' \n";
	$resp = $stats_rrdb_udp->send_command("c|test2|10 secs for 1 week, 1 min for 1 month, 10 min for 1 year, 30 min for 10 years");
	check_result($resp, "OK");

	//
	// Update the first time
	//
	for($ii = 0; $ii < 30; $ii++) {
		echo "== UPDATE METRIC 'test1' $ii -> 1\n";
		$ts = 1371104586 + $ii;
		$resp = $stats_rrdb_udp->send_command("U|test1|1|$ts");
		check_result($resp, "OK");
	}	
	
	//
	// Restart
	//
	restart_server();

	//
	// Update the first time
	//
	for($ii = 0; $ii < 30; $ii++) {
		echo "== UPDATE METRIC 'test1' $ii -> 1\n";
		$ts = 1371104586 + 30 + $ii;
		$resp = $stats_rrdb_udp->send_command("U|test1|1|$ts");
		check_result($resp, "OK");
	}

	
	//
	// Select data (TCP!!!)
	//
	echo "== SELECT * FROM METRIC 'test1' BETWEEN 1371104600 AND 1371104990 GROUP BY 0 sec\n";
	$resp = $stats_rrdb_tcp->send_command("SELECT * FROM 'test1' BETWEEN 1371104600 AND 1371104990 GROUP BY 0 sec;");
	echo "$resp\n";
	
	echo "== SELECT * FROM METRIC 'test1' BETWEEN 1371104600 AND 1371104990 GROUP BY 1 sec\n";
    $resp = $stats_rrdb_tcp->send_command("SELECT * FROM 'test1' BETWEEN 1371104600 AND 1371104990 GROUP BY 1 sec;");
    echo "$resp\n";
	
	echo "== SELECT * FROM METRIC 'test1' BETWEEN 1371104600 AND 1371104990 GROUP BY 5 sec\n";
	$resp = $stats_rrdb_tcp->send_command("SELECT * FROM 'test1' BETWEEN 1371104600 AND 1371104990 GROUP BY 5 sec;");
	echo "$resp\n";
	
	echo "== SELECT * FROM METRIC 'test1' BETWEEN 1371104500 AND 1371104990 GROUP BY 10 sec\n";
	$resp = $stats_rrdb_tcp->send_command("SELECT * FROM 'test1' BETWEEN 1371104500 AND 1371104990 GROUP BY 10 sec;");
	echo "$resp\n";
	
	echo "== SELECT * FROM METRIC 'test1' BETWEEN 1371104500 AND 1371104990 GROUP BY 15 sec\n";
	$resp = $stats_rrdb_tcp->send_command("SELECT * FROM 'test1' BETWEEN 1371104500 AND 1371104990 GROUP BY 15 sec;");
	echo "$resp\n";
	
	
	//
	// Drop
	//
	echo "== DROP METRIC 'test1'\n";
	$resp = $stats_rrdb_udp->send_command("d|test1");
	check_result($resp, "OK");

	echo "== DROP METRIC 'test2'\n";
	$resp = $stats_rrdb_udp->send_command("d|test2");
	check_result($resp, "OK");
} catch(Exception $e) {
	echo "Exception: " . $e->getMessage() . "\n";
}

stop_server();


