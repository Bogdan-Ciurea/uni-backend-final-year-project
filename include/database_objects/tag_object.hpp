/**
 * @file tag_object.hpp
 * @author Bogdan Ciurea (sc20bac@leeds.ac.uk)
 * @brief Class that stores the tag object
 * @version 0.1
 * @date 2023-01-04
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef TAG_OBJECT_H
#define TAG_OBJECT_H

#include <json/json.h>

#include <string>
#include <vector>

#include "cassandra.h"

class TagObject {
 public:
  TagObject() {}
  TagObject(int school_id, std::string id, std::string name, std::string colour)
      : _school_id(school_id),
        _name(std::move(name)),
        _colour(std::move(colour)) {
    cass_uuid_from_string(id.c_str(), &_id);
  }
  TagObject(int school_id, CassUuid id, std::string name, std::string colour)
      : _school_id(school_id),
        _id(id),
        _name(std::move(name)),
        _colour(std::move(colour)) {}
  ~TagObject() {}

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
  std::string _colour;
};

#endif