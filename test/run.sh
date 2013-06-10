#!/bin/bash

TEST_FOLDER=`dirname $0` `
ROOT_FOLDER="$(TEST_FOLDER)/../"

$(ROOT_FOLDER)/src/src/statsrrdb  --config stats-rrdb.conf

