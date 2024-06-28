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

#ifndef COURSE_OBJECT_H
#define COURSE_OBJECT_H

#include <json/json.h>

#include <string>
#include <vector>

#include "cassandra.h"

class CourseObject {
 public:
  CourseObject(){};
  CourseObject(int school_id, std::string id, std::string name,
               std::string course_thumbnail, time_t created_at,
               time_t start_date, time_t end_date,
               std::vector<std::string> files)
      : _school_id(school_id),
        _name(std::move(name)),
        _course_thumbnail(std::move(course_thumbnail)),
        _created_at(created_at),
        _start_date(start_date),
        _end_date(end_date) {
    cass_uuid_from_string(id.c_str(), &_id);
    for (auto &file : files) {
      CassUuid file_id;
      cass_uuid_from_string(file.c_str(), &file_id);
      _files.push_back(file_id);
    }
  }
  CourseObject(int school_id, CassUuid id, std::string name,
               std::string course_thumbnail, time_t created_at,
               time_t start_date, time_t end_date, std::vector<CassUuid> files)
      : _school_id(school_id),
        _id(id),
        _name(std::move(name)),
        _course_thumbnail(std::move(course_thumbnail)),
        _created_at(created_at),
        _start_date(start_date),
        _end_date(end_date),
        _files(std::move(files)) {}

  ~CourseObject(){};

  /**
   * @brief Returns a Json object containing the information about this object
   *
   * @param secure If we just want the most important information
   * @return The Json object
   */
  Json::Value to_json(bool secure);

  int _school_id;
  CassUuid _id;
  std::string _name;
  std::string _course_thumbnail;
  time_t _created_at;
  time_t _start_date;
  time_t _end_date;
  std::vector<CassUuid> _files;
};

#endif  // COURSE_OBJECT_H