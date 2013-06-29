<?php
$ROOT_FOLDER = dirname($argv[0]) . "/../../";
include_once($ROOT_FOLDER . "/examples/php/sdk/stats_rrdb.php");

$CONFIG_FILE = "$ROOT_FOLDER/src/tests/stats-rrdb.conf";

/**
 * Params from the config file
 */
$HOST = '127.0.0.1';
$PORT = 9876;
$TEST_FOLDER = "/tmp/stats-rrdb.test";
$PID_FILE = "$TEST_FOLDER/stats-rrdb.pid";

function start_server() {
	global $ROOT_FOLDER, $TEST_FOLDER, $CONFIG_FILE, $PID_FILE;

	@mkdir($TEST_FOLDER);

	echo "=================== Starting server\n";
	system("$ROOT_FOLDER/src/stats-rrdb  --config \"$CONFIG_FILE\" --daemon \"$PID_FILE\"");
	sleep(3);
	$pid=file_get_contents($PID_FILE);
	echo "=================== Started server $pid\n";
}

function stop_server() {
	global $PID_FILE;

	echo "=================== Stopping server\n";
	$pid=file_get_contents($PID_FILE);
	system("kill $pid");
	sleep(3);
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

function start_test($name) {
	echo "=========== {$name}\n";
}

function cleanup(StatsRRDB $stats_rrdb, $num) {
	for($ii = 0; $ii < $num; $ii++) {
		try {
			$stats_rrdb->drop_metric("test{$ii}");
		} catch(Exception $e) {
			// ignore errors
		}
	}
}


restart_server();

try {
	// params
	$start_ts = 1371104490;
	$num = 30;
	
	// create & cleanup
	start_test("CREATE THE CLIENT");
	$stats_rrdb = new StatsRRDB($HOST, $PORT);
	cleanup($stats_rrdb, 10);

	// create metrics and test policy
	start_test("CREATE METRIC 'test1");
	$resp = $stats_rrdb->create_metric("test1", "1 sec for 10 sec, 10 secs for 30 secs, 30 secs for 10 min"); 
	check_result($resp, "OK");
	$resp = $stats_rrdb->show_metric_policy("test1");
	check_result($resp, "1 sec for 10 secs, 10 secs for 30 secs, 30 secs for 10 mins");
	
	start_test("CREATE METRIC 'test2'");
	$resp = $stats_rrdb->create_metric("test2", "10 secs for 1 week, 1 min for 1 month, 10 mins for 1 year, 30 mins for 10 years"); 
	check_result($resp, "OK");
	$resp = $stats_rrdb->show_metric_policy("test2");
	check_result($resp, "10 secs for 1 week, 1 min for 1 month, 10 mins for 1 year, 30 mins for 10 years");
	
	// show metrics: should have 2 in the response
	start_test("SHOW METRICS");
	$resp = $stats_rrdb->show_metrics("test");
	check_result(count($resp), 2);
	
	// update test1
	start_test("UPDATE METRIC 'test1' 1 x {$num} times");
	for($ii = 0; $ii < $num; $ii++) {
		$stats_rrdb->update('test1', 1.0, $start_ts + $ii);		
	}	

	// select all one line
	start_test("SELECT * FROM METRIC 'test1'");
	$resp = $stats_rrdb->select('test1', 'all', $start_ts, $start_ts + $num + 1, "0 secs");
	check_result(count($resp), min(10 / 1, $num / 1) + min(30 / 10, $num / 10 - 1) + 1); // +1 for the header row, 10  - per sec, $num / 10 - per 10 secs excluding last one
	
	// select all 5 sec intervals
	start_test("SELECT * FROM METRIC 'test1'  GROUP BY 5 sec");
	$resp = $stats_rrdb->select('test1', 'all', $start_ts, $start_ts + $num + 1, "5 secs");
	check_result(count($resp), $num / 5 + 1); // +1 for the header row

	// select all 1 year -> result is one line
	start_test("SELECT * FROM METRIC 'test1' GROUP BY 1 year");
	$resp = $stats_rrdb->select('test1', 'all', $start_ts, $start_ts + $num + 1, "1 year");
	check_result(count($resp), 1 + 1); // +1 for the header row
	check_result($resp[1][0], $start_ts); // ts
	check_result($resp[1][1], $num);      // count
	check_result($resp[1][2], $num);      // sum
	check_result($resp[1][3], 1.0);       // avg
	check_result($resp[1][4], 0.0);       // stddev
	check_result($resp[1][5], 1.0);       // min
	check_result($resp[1][6], 1.0);       // max

	// test 2 is empty
	start_test("SELECT * FROM METRIC 'test2'");
	$resp = $stats_rrdb->select('test2', 'all', $start_ts, $start_ts + 2*$num + 1);
	check_result(count($resp), 1); // +1 for the header row
		
	/////////////////
	//
	// Restart
	//
	/////////////////
	restart_server();

	// checks metrics after restart
	start_test("SHOW METRICS after restart");
	$resp = $stats_rrdb->show_metrics("test");
	check_result(count($resp), 2);

	start_test("SHOW METRIC POLICY 'test1' after restart");
	$resp = $stats_rrdb->show_metric_policy("test1");
	check_result($resp, "1 sec for 10 secs, 10 secs for 30 secs, 30 secs for 10 mins");
		
	start_test("SHOW METRIC POLICY 'test2' after restart");
	$resp = $stats_rrdb->show_metric_policy("test2");
	check_result($resp, "10 secs for 1 week, 1 min for 1 month, 10 mins for 1 year, 30 mins for 10 years");
	
	// update test1 aga9j
	start_test("UPDATE METRIC 'test1' again: 1 x {$num} times");
	for($ii = 0; $ii < $num; $ii++) {
		$stats_rrdb->update('test1', 1.0, $start_ts + $num + $ii);
	}
	
	// select all one line
	start_test("SELECT * FROM METRIC 'test1'");
	$resp = $stats_rrdb->select('test1', 'all', $start_ts, $start_ts + 2*$num + 1, "0 secs");
	check_result(count($resp), min(10 / 1, 2*$num / 1 - 1) + min(30 / 10, 2*$num / 10 - 1) +  min(10*60 / 30, 2*$num / 30 - 2) + 1); // +1 for the header row, 10  - per sec, $num / 10 - per 10 secs excluding last one
	
	// select all 5 sec intervals
	start_test("SELECT * FROM METRIC 'test1'  GROUP BY 5 sec");
	$resp = $stats_rrdb->select('test1', 'all', $start_ts, $start_ts + 2*$num + 1, "5 secs");
	check_result(count($resp), 2*$num / 5 + 1); // +1 for the header row
	
	// select all 1 year -> result is one line
	start_test("SELECT * FROM METRIC 'test1' GROUP BY 1 year");
	$resp = $stats_rrdb->select('test1', 'all', $start_ts, $start_ts + 2*$num + 1, "1 year");
	check_result(count($resp), 1 + 1); // +1 for the header row
	check_result($resp[1][0], $start_ts); // ts
	check_result($resp[1][1], 2*$num);      // count
	check_result($resp[1][2], 2*$num);      // sum
	check_result($resp[1][3], 1.0);       // avg
	check_result($resp[1][4], 0.0);       // stddev
	check_result($resp[1][5], 1.0);       // min
	check_result($resp[1][6], 1.0);       // ma

	// test 2 is still empty
	start_test("SELECT * FROM METRIC 'test2'");
	$resp = $stats_rrdb->select('test2', 'all', $start_ts, $start_ts + 2*$num + 1);
	check_result(count($resp), 1); // +1 for the header row
	
	// check that we get an error
	start_test("SELECT METRIC 'xxx' (error)");
	$error = "";
	try {
		$resp = $stats_rrdb->select('xxx', 'all', $start_ts, $start_ts + 2*$num + 1);
		print_r($resp);
	} catch(Exception $e) {
		$error = $e->getMessage();
	}
	check_result($error, "ERROR: The metric 'xxx' does not exist");
	
	// cleanup
	start_test("DROP METRICS");
	$resp = $stats_rrdb->drop_metric('test1');
	check_result($resp, "OK");
	$resp = $stats_rrdb->drop_metric('test2');
	check_result($resp, "OK");
	
	
	// 
	start_test("SHOW STATUS");
	$resp = $stats_rrdb->show_status();
	check_result(count($resp) > 10, true); // we have at least 10 self.xxx metrics
		
	start_test("SHOW STATUS LIKE 'tcp'");
	$resp = $stats_rrdb->show_status('tcp');
	check_result(count($resp), 3); // this will fail often
			
} catch(Exception $e) {
	echo "Exception: " . $e->getMessage() . "\n";
}

stop_server();


