#include "database_objects/answer_object.hpp"

Json::Value AnswerObject::to_json(bool secure) {
  Json::Value value;
  if (!secure) {
    value["school_id"] = _school_id;
  }

  char* id_uuid_string = (char*)malloc(sizeof(char) * 37);
  cass_uuid_string(_id, id_uuid_string);

  value["id"] = id_uuid_string;

  char* _created_by_uuid_string = (char*)malloc(sizeof(char) * 37);
  cass_uuid_string(_created_by, _created_by_uuid_string);

  value["created_by"] = _created_by_uuid_string;

  value["created_at"] = (Json::Value::Int64)_created_at;
  value["content"] = _content;

  free(id_uuid_string);
  free(_created_by_uuid_string);

  return value;
}
