/**
 * @file grade_object.hpp
 * @author Bogdan Ciurea (sc20bac@leeds.ac.uk)
 * @brief Class that stores the grade object
 * @version 0.1
 * @date 2023-01-03
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef GRADE_OBJECT_H
#define GRADE_OBJECT_H

#include <json/json.h>

#include <string>
#include <vector>

#include "cassandra.h"

class GradeObject {
 public:
  GradeObject() {}
  GradeObject(int school_id, std::string id, std::string evaluated_id,
              std::string evaluator_id, std::string course_id, int grade,
              int out_of, time_t created_at, float weight)
      : _school_id(school_id),
        _grade(grade),
        _out_of(out_of),
        _created_at(created_at),
        _weight(weight) {
    cass_uuid_from_string(id.c_str(), &_id);
    cass_uuid_from_string(evaluated_id.c_str(), &_evaluated_id);
    cass_uuid_from_string(evaluator_id.c_str(), &_evaluator_id);
    cass_uuid_from_string(course_id.c_str(), &_course_id);
  }
  GradeObject(int school_id, CassUuid id, CassUuid evaluated_id,
              CassUuid evaluator_id, CassUuid course_id, int grade, int out_of,
              time_t created_at, float weight)
      : _school_id(school_id),
        _id(id),
        _evaluated_id(evaluated_id),
        _evaluator_id(evaluator_id),
        _course_id(course_id),
        _grade(grade),
        _out_of(out_of),
        _created_at(created_at),
        _weight(weight) {}
  ~GradeObject() {}

  /**
   * @brief Returns a Json object containing the information about this object
   *
   * @param secure If we just want the most important information
   * @return The Json object
   */
  Json::Value to_json(bool secure);

  int _school_id;
  CassUuid _id;
  CassUuid _evaluated_id;
  CassUuid _evaluator_id;
  CassUuid _course_id;
  int _grade;
  int _out_of;
  time_t _created_at;
  float _weight;
};

#endif