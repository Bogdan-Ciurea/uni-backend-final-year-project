#include "database_objects/grade_object.hpp"

Json::Value GradeObject::to_json(bool secure) {
  Json::Value value;
  if (!secure) {
    value["school_id"] = _school_id;
  }

  char* id_uuid_string = (char*)malloc(sizeof(char) * 37);
  cass_uuid_string(_id, id_uuid_string);

  value["id"] = id_uuid_string;

  char* evaluated_id_uuid_string = (char*)malloc(sizeof(char) * 37);
  cass_uuid_string(_evaluated_id, evaluated_id_uuid_string);

  value["evaluated_id"] = evaluated_id_uuid_string;

  char* evaluator_id_uuid_string = (char*)malloc(sizeof(char) * 37);
  cass_uuid_string(_evaluator_id, evaluator_id_uuid_string);

  value["evaluator_id"] = evaluator_id_uuid_string;

  char* course_id_uuid_string = (char*)malloc(sizeof(char) * 37);
  cass_uuid_string(_course_id, course_id_uuid_string);

  value["course_id"] = course_id_uuid_string;

  free(id_uuid_string);
  free(evaluated_id_uuid_string);
  free(evaluator_id_uuid_string);
  free(course_id_uuid_string);

  value["grade"] = _grade;
  value["out_of"] = _out_of;
  value["created_at"] = (Json::Value::Int64)_created_at;
  value["weight"] = _weight;

  return value;
}