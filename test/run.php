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


start_server();

try {
	// create
	$mode = isset($argv[1]) ? $argv[1] : StatsRRDB::Mode_Tcp;
	echo "Starting mode {$mode}\n";
	$stats_rrdb = new StatsRRDB($mode);

	echo "== CREATE METRIC 'test1' \n";
	$resp = $stats_rrdb->send_command("create metric 'test1' keep 5 secs for 1 min, 1 min for 1 week, 10 min for 1 year, 1 hour for 10 years ;"); 
	echo "$resp\n";

	echo "== CREATE METRIC 'test2' \n";
	$resp = $stats_rrdb->send_command("create metric 'test2' keep 10 secs for 1 week, 1 min for 1 month, 10 min for 1 year, 30 min for 10 years;");
	echo "$resp\n";

	echo "== SHOW METRICS\n";
	$resp = $stats_rrdb->send_command("show metrics like 'te';");
	echo "$resp\n";

	system("ls -la /tmp/rrdb/*/*");	
	restart_server();

	echo "== SHOW METRICS\n";
        $resp = $stats_rrdb->send_command("show metrics like 'te';");
        echo "$resp\n";


	echo "== SHOW METRIC\n";
	$resp = $stats_rrdb->send_command("show metric 'test1';");
	echo "$resp\n";

        echo "== SHOW METRIC\n";
        $resp = $stats_rrdb->send_command("show metric 'test2';");
        echo "$resp\n";

	echo "== SHOW METRIC (error)\n";
	$resp = $stats_rrdb->send_command("show metric 'test';");
	echo "$resp\n";

	echo "== DROP METRIC 'test1'\n";
	$resp = $stats_rrdb->send_command("drop metric 'test1';");
	echo "$resp\n";

	echo "== DROP METRIC 'test2'\n";
	$resp = $stats_rrdb->send_command("drop metric 'test2';");
	echo "$resp\n";
} catch(Exception $e) {
	echo "Exception: " . $e->getMessage() . "\n";
}

stop_server();


