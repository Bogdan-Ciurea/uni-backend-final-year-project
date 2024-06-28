#include "database_objects/question_object.hpp"

Json::Value QuestionObject::to_json(bool secure) {
  Json::Value value;
  if (!secure) {
    value["school_id"] = _school_id;
  }

  char* question_uuid_string = (char*)malloc(sizeof(char) * 37);
  cass_uuid_string(_question_id, question_uuid_string);
  value["id"] = question_uuid_string;

  value["content"] = _text;
  value["created_at"] = (Json::Value::Int64)_time_added;

  char* added_by_user_uuid_string = (char*)malloc(sizeof(char) * 37);
  cass_uuid_string(_added_by_user_id, added_by_user_uuid_string);
  value["added_by_user_id"] = added_by_user_uuid_string;

  free(question_uuid_string);
  free(added_by_user_uuid_string);

  return value;
}