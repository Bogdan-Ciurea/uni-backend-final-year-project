#include "database_objects/announcements_object.hpp"

Json::Value AnnouncementObject::to_json(bool secure) {
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
  value["title"] = _title;
  value["content"] = _content;
  value["allow_answers"] = _allow_answers;

  for (auto& file : _files) {
    char* file_uuid_string = (char*)malloc(sizeof(char) * 37);
    cass_uuid_string(file, file_uuid_string);
    value["files"].append(file_uuid_string);
    free(file_uuid_string);
  }
  free(id_uuid_string);
  free(_created_by_uuid_string);

  return value;
}
