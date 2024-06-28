#include "database_objects/todo_object.hpp"

Json::Value TodoObject::to_json(bool secure) {
  Json::Value value;
  if (!secure) {
    value["school_id"] = _school_id;
  }

  char* todo_uuid_string = (char*)malloc(sizeof(char) * 37);
  cass_uuid_string(_todo_id, todo_uuid_string);
  value["todo_id"] = todo_uuid_string;

  value["text"] = _text;
  value["type"] = static_cast<int>(_type);

  free(todo_uuid_string);

  return value;
}