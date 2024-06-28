/**
 * @file answer_object.hpp
 * @author Bogdan Ciurea (sc20bac@leeds.ac.uk)
 * @brief Class that stores the answers object
 * @version 0.1
 * @date 2022-12-31
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef ANSWER_OBJECT_H
#define ANSWER_OBJECT_H

#include <json/json.h>

#include <string>
#include <vector>

#include "cassandra.h"

class AnswerObject {
 public:
  AnswerObject() {}
  AnswerObject(int school_id, std::string id, time_t created_at,
               std::string created_by, std::string content)
      : _school_id(school_id),
        _created_at(created_at),
        _content(std::move(content)) {
    cass_uuid_from_string(id.c_str(), &_id);
    cass_uuid_from_string(created_by.c_str(), &_created_by);
  }
  AnswerObject(int school_id, CassUuid id, time_t created_at,
               CassUuid created_by, std::string content)
      : _school_id(school_id),
        _id(id),
        _created_at(created_at),
        _created_by(created_by),
        _content(std::move(content)) {}
  ~AnswerObject(){};

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
  std::string _content;
};

#endif  // ANSWER_OBJECT_H