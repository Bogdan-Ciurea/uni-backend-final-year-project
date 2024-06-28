/**
 * @file todo_object.hpp
 * @author Bogdan Ciurea (sc20bac@leeds.ac.uk)
 * @brief Class that stores the lecture object
 * @version 0.1
 * @date 2023-01-07
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef TODO_OBJECT_H
#define TODO_OBJECT_H

#include <json/json.h>

#include <string>
#include <vector>

#include "cassandra.h"

enum class TodoType { NOT_STARTED, IN_PROGRESS, DONE };

class TodoObject {
 public:
  TodoObject() {}
  TodoObject(int school_id, std::string todo_id, std::string text,
             TodoType type)
      : _school_id(school_id), _text(std::move(text)), _type(type) {
    cass_uuid_from_string(todo_id.c_str(), &_todo_id);
  }
  TodoObject(int school_id, CassUuid todo_id, std::string text, TodoType type)
      : _school_id(school_id),
        _todo_id(todo_id),
        _text(std::move(text)),
        _type(type) {}
  ~TodoObject() {}

  /**
   * @brief Returns a Json object containing the information about this object
   *
   * @param secure If we just want the most important information
   * @return The Json object
   */
  Json::Value to_json(bool secure);

  int _school_id;
  CassUuid _todo_id;
  std::string _text;
  TodoType _type;
};

#endif