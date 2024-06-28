#!/bin/bash

path=$(readlink -f "${BASH_SOURCE:-$0}")   # path of the file
W_DIR=$(dirname $path)                     # working directory

. $W_DIR/set_variables.sh

delete_cass_container()
{
    echo "Clearing resources..."
    docker stop ${CASS_NAME} > /dev/null
    docker rm ${CASS_NAME} > /dev/null
    echo "Container removed successfully"
}

wait_for_container_to_start()
{
    # even if cassandra containter has started, we also need cqlsh to start
    echo "Waiting for cassandra to start..."
    time_elapsed=0
    while [[ $(${CASS_DO} bash -c "cqlsh << ${CASS_RUN_CHECK}" | wc -c) -eq 0 ]]; do
        (( time_elapsed+=${CASS_RETRY_INTERVAL} ))
        if [[ $OSTYPE == 'darwin'* ]]; then
            sleep ${CASS_RETRY_INTERVAL}
        else
            sleep ${CASS_RETRY_INTERVAL}s
        fi

        if [[ $time_elapsed -ge $CASS_WAIT_LIMIT ]]; then exit 1; fi
    done

    echo "Cassandra container started"
}

deploy_cass_container()
{
    # launching the container
    echo "Deploying a cassandra container..."
    docker run -p ${CASS_PORT}:9042 --name ${CASS_NAME} --privileged=true -d $CASS_IMMAGE

    wait_for_container_to_start
}

# check if the container exists
echo "Searching for a cassandra container..."
docker container inspect ${CASS_NAME} > /dev/null
if [[ $? -ne 0 ]]; then
    echo "No cassandra container found"
    deploy_cass_container
else
    if [[ $(${CASS_DO} bash -c "cqlsh << ${CASS_RUN_CHECK}" | wc -c) -eq 0 ]]; then
        echo "Cassandra container is stopped. Starting ..."
        docker start ${CASS_NAME} >> /dev/null
        wait_for_container_to_start
    else
        echo "Cassandra container already running"
    fi

    if [[ $(docker port ${CASS_NAME} | grep ${CASS_PORT}/tcp | wc -c) -eq 0 ]]; then
        echo "Cassandra port error. Recreating container"
        delete_cass_container
        deploy_cass_container
    fi
fi