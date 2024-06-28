/**
 * @file course_object.hpp
 * @author Bogdan Ciurea (sc20bac@leeds.ac.uk)
 * @brief Class that stores the course object
 * @version 0.1
 * @date 2023-01-02
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "database_objects/course_object.hpp"

Json::Value CourseObject::to_json(bool secure) {
  Json::Value json;

  if (!secure) {
    json["school_id"] = _school_id;
    json["created_at"] = (Json::Value::Int64)_created_at;
    json["start_date"] = (Json::Value::Int64)_start_date;
    json["end_date"] = (Json::Value::Int64)_end_date;
  }

  char* id_uuid_string = (char*)malloc(sizeof(char) * 37);
  cass_uuid_string(_id, id_uuid_string);

  json["id"] = id_uuid_string;

  json["school_id"] = _school_id;
  json["name"] = _name;
  json["course_thumbnail"] = _course_thumbnail;

  for (auto& file : _files) {
    char* file_uuid_string = (char*)malloc(sizeof(char) * 37);
    cass_uuid_string(file, file_uuid_string);
    json["files"].append(file_uuid_string);
  }

  free(id_uuid_string);

  return json;
}