/**
 * @file student_reference.hpp
 * @author Bogdan Ciurea (sc20bac@leeds.ac.uk)
 * @brief Class that stores the student reference object
 * @version 0.1
 * @date 2023-01-06
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef STUDENT_REFERENCE_H
#define STUDENT_REFERENCE_H

#include <json/json.h>

#include <string>
#include <vector>

#include "cassandra.h"

enum class ReferenceType { EMAIL = 0, PHONE_NUMBER = 1 };

class StudentReferenceObject {
 public:
  StudentReferenceObject(){};
  StudentReferenceObject(int school_id, std::string student_id,
                         std::string reference, ReferenceType type)
      : _school_id(school_id), _reference(std::move(reference)), _type(type) {
    cass_uuid_from_string(student_id.c_str(), &_student_id);
  }
  StudentReferenceObject(int school_id, CassUuid student_id,
                         std::string reference, ReferenceType type)
      : _school_id(school_id),
        _student_id(student_id),
        _reference(std::move(reference)),
        _type(type) {}
  ~StudentReferenceObject(){};

  /**
   * @brief Returns a Json object containing the information about this object
   *
   * @param secure If we just want the most important information
   * @return The Json object
   */
  Json::Value to_json(bool secure);

  int _school_id;
  CassUuid _student_id;
  std::string _reference;
  ReferenceType _type;
};

#endif