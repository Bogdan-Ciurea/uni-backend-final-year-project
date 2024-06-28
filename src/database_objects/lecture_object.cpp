#include "database_objects/lecture_object.hpp"

Json::Value LectureObject::to_json(bool secure) {
  Json::Value value;
  if (!secure) {
    value["school_id"] = _school_id;
  }

  char* course_uuid_string = (char*)malloc(sizeof(char) * 37);
  cass_uuid_string(_course_id, course_uuid_string);
  value["course_id"] = course_uuid_string;

  value["starting_time"] = (Json::Value::Int64)_starting_time;
  value["duration"] = _duration;
  value["location"] = _location;

  free(course_uuid_string);

  return value;
}