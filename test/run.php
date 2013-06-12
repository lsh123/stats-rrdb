<?php
$ROOT_FOLDER = dirname($argv[0]) . "/..";
include_once($ROOT_FOLDER . "/sdk/php/stats_rrdb.php");

// create
$mode = isset($argv[1]) ? $argv[1] : StatsRRDB::Mode_Tcp;
echo "Starting mode {$mode}\n";
$stats_rrdb = new StatsRRDB($mode);

echo "== CREATE METRIC 'test1' \n";
$resp = $stats_rrdb->send_command("create metric 'test1' keep 5 secs for 1 min, 1 min for 1 week, 10 min for 1 year, 1 hour for 10 years ;"); 
echo "$resp\n";

echo "== CREATE METRIC 'test2' \n";
$resp = $stats_rrdb->send_command("create metric 'test2' keep 5 secs for 1 min, 1 min for 1 hour, 1 hour for 1 day;");
echo "$resp\n";

echo "== SHOW METRICS\n";
$resp = $stats_rrdb->send_command("show metrics like 'te';");
echo "$resp\n";

echo "== SHOW METRIC\n";
$resp = $stats_rrdb->send_command("show metric 'test1';");
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

