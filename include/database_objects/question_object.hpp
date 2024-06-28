/**
 * @file question_object.hpp
 * @author Bogdan Ciurea (sc20bac@leeds.ac.uk)
 * @brief Class that stores the question object
 * @version 0.1
 * @date 2023-01-07
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef QUESTION_OBJECT_H
#define QUESTION_OBJECT_H

#include <json/json.h>

#include <string>
#include <vector>

#include "cassandra.h"

class QuestionObject {
 public:
  QuestionObject(){};
  QuestionObject(int school_id, std::string question_id, std::string text,
                 time_t time_added, std::string added_by_user_id)
      : _school_id(school_id), _text(std::move(text)), _time_added(time_added) {
    cass_uuid_from_string(question_id.c_str(), &_question_id);
    cass_uuid_from_string(added_by_user_id.c_str(), &_added_by_user_id);
  }
  QuestionObject(int school_id, CassUuid question_id, std::string text,
                 time_t time_added, CassUuid added_by_user_id)
      : _school_id(school_id),
        _question_id(question_id),
        _text(std::move(text)),
        _time_added(time_added),
        _added_by_user_id(added_by_user_id) {}
  ~QuestionObject(){};

  /**
   * @brief Returns a Json object containing the information about this object
   *
   * @param secure If we just want the most important information
   * @return The Json object
   */
  Json::Value to_json(bool secure);

  int _school_id;
  CassUuid _question_id;
  std::string _text;
  time_t _time_added;
  CassUuid _added_by_user_id;
};

#endif