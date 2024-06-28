#include "database_objects/user_object.hpp"

Json::Value UserObject::to_json(bool secure) {
  Json::Value value;
  if (!secure) {
    value["school_id"] = _school_id;
    value["password"] = _password;
    value["phone_number"] = _phone_number;
    value["last_time_online"] = (Json::Value::Int64)_last_time_online;
    value["changed_password"] = _changed_password;
  }

  char* user_uuid_string = (char*)malloc(sizeof(char) * 37);
  cass_uuid_string(_user_id, user_uuid_string);
  value["user_id"] = user_uuid_string;

  value["email"] = _email;
  value["user_type"] = static_cast<int>(_user_type);
  value["first_name"] = _first_name;
  value["last_name"] = _last_name;

  free(user_uuid_string);

  return value;
}