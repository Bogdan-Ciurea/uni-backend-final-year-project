/**
 * @file announcements_object.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief Class that stores the announcement object
 * @version 0.1
 * @date 2022-12-16
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef ANNOUNCEMENTS_OBJECT_H
#define ANNOUNCEMENTS_OBJECT_H

#include <json/json.h>

#include <string>
#include <vector>

#include "cassandra.h"

class AnnouncementObject {
 public:
  AnnouncementObject() {}
  AnnouncementObject(int school_id, std::string id, time_t created_at,
                     std::string created_by, std::string title,
                     std::string content, bool allow_answers,
                     std::vector<std::string> files)
      : _school_id(school_id),
        _created_at(created_at),
        _title(std::move(title)),
        _content(std::move(content)),
        _allow_answers(allow_answers) {
    cass_uuid_from_string(id.c_str(), &_id);
    cass_uuid_from_string(created_by.c_str(), &_created_by);
    for (auto& file : files) {
      CassUuid file_id;
      cass_uuid_from_string(file.c_str(), &file_id);
      _files.push_back(file_id);
    }
  }
  AnnouncementObject(int school_id, CassUuid id, time_t created_at,
                     CassUuid created_by, std::string title,
                     std::string content, bool allow_answers,
                     std::vector<CassUuid> files)
      : _school_id(school_id),
        _id(id),
        _created_at(created_at),
        _created_by(created_by),
        _title(std::move(title)),
        _content(std::move(content)),
        _allow_answers(allow_answers),
        _files(std::move(files)) {}
  ~AnnouncementObject() {}

  /**
   * @brief Returns a Json object containing the information about this object
   *
   * @param secure If we just want the most important information
   * @return The Json object
   */
  Json::Value to_json(bool secure);

  int _school_id;
  CassUuid _id;
  time_t _created_at;
  CassUuid _created_by;
  std::string _title;
  std::string _content;
  bool _allow_answers;
  std::vector<CassUuid> _files;
};

#endif