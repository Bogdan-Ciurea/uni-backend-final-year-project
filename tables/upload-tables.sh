#!/bin/bash

# Find the Cassandra container running on port 9042
CONTAINER_ID=$(docker ps --filter "expose=9042/tcp" --format "{{.ID}}")

if [ -z "$CONTAINER_ID" ]
then
    echo "No Cassandra container running on port 9042"
    exit 1
fi

# Define an array of table names
tables=("environment.countries" "environment.holidays_by_country_or_school" "environment.schools" 
"schools.announcements" "schools.announcements_by_tag" "schools.answers" "schools.answers_by_announcement_or_question" "schools.courses" "schools.courses_by_user" "schools.files" 
"schools.grades" "schools.lectures" "schools.questions" "schools.questions_by_course" "schools.student_reference" "schools.tags" "schools.tags_by_user" 
"schools.todos" "schools.todos_by_user" "schools.tokens" "schools.users" "schools.users_by_course" "schools.users_by_tag")

# Upload each CSV file to the corresponding table and delete the temporary CSV file
for table in "${tables[@]}"
do
    docker cp $(pwd)/${table}.csv $CONTAINER_ID:/tmp/${table}.csv
    docker exec -it $CONTAINER_ID cqlsh -e "COPY $table FROM '/tmp/${table}.csv' WITH HEADER=TRUE"
    docker exec $CONTAINER_ID rm /tmp/${table}.csv
    echo "Uploaded $(pwd)/${table}.csv to table ${table}"
done
