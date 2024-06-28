#!/bin/bash

CASS_PORT="9042"                     # container ip
CASS_NAME="cassandra-test-container" # container name
CASS_IMMAGE="cassandra"              # cassandra docker immage
CASS_DO="docker exec $CASS_NAME"     # execute code for cassandra

CASS_WAIT_LIMIT=300                  # number of seconds to wait for Cassandra to startuntil exiting
CASS_RETRY_INTERVAL=10               # the refresh interval

CASS_GET_KEYSPACES="DESCRIBE KEYSPACES;"
CASS_RUN_CHECK="EOF
$CASS_GET_KEYSPACES
EOF"
