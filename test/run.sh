#!/bin/bash

TEST_FOLDER=`dirname $0`
ROOT_FOLDER="$TEST_FOLDER/../"
CONFIG_FILE="$ROOT_FOLDER/stats-rrdb.conf"
PID_FILE="/tmp/stats-rrdb.pid"

echo "Starting server"
$ROOT_FOLDER/src/statsrrdb  --config "$CONFIG_FILE" --daemon "$PID_FILE"
PID=`cat "$PID_FILE"`

echo "Running tests"
php $TEST_FOLDER/run.php

echo "Stopping server"
kill $PID

