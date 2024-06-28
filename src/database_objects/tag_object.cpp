#include "database_objects/tag_object.hpp"

Json::Value TagObject::to_json(bool secure) {
  Json::Value value;
  if (!secure) {
    value["school_id"] = _school_id;
  }

  char* id_uuid_string = (char*)malloc(sizeof(char) * 37);
  cass_uuid_string(_id, id_uuid_string);

  value["id"] = id_uuid_string;

  free(id_uuid_string);

  value["name"] = _name;
  value["colour"] = _colour;

  return value;
}