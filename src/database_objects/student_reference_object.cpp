#include "database_objects/student_reference_object.hpp"

Json::Value StudentReferenceObject::to_json(bool secure) {
  Json::Value value;
  if (!secure) {
    value["school_id"] = _school_id;
  }

  char* id_uuid_string = (char*)malloc(sizeof(char) * 37);
  cass_uuid_string(_student_id, id_uuid_string);

  value["id"] = id_uuid_string;

  value["reference"] = _reference;
  value["type"] = static_cast<int>(_type);

  free(id_uuid_string);

  return value;
}