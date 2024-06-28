/**
 * @file lecture_object.hpp
 * @author Bogdan Ciurea (sc20bac@leeds.ac.uk)
 * @brief Class that stores the lecture object
 * @version 0.1
 * @date 2023-01-06
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef LECTURE_OBJECT_H
#define LECTURE_OBJECT_H

#include <json/json.h>

#include <string>
#include <vector>

#include "cassandra.h"

class LectureObject {
 public:
  LectureObject(){};
  LectureObject(int school_id, std::string course_id, time_t starting_time,
                int duration, std::string location)
      : _school_id(school_id),
        _starting_time(starting_time),
        _duration(duration),
        _location(std::move(location)) {
    cass_uuid_from_string(course_id.c_str(), &_course_id);
  }
  LectureObject(int school_id, CassUuid course_id, time_t starting_time,
                int duration, std::string location)
      : _school_id(school_id),
        _course_id(course_id),
        _starting_time(starting_time),
        _duration(duration),
        _location(std::move(location)) {}
  ~LectureObject(){};

  /**
   * @brief Returns a Json object containing the information about this object
   *
   * @param secure If we just want the most important information
   * @return The Json object
   */
  Json::Value to_json(bool secure);

  int _school_id;
  CassUuid _course_id;
  time_t _starting_time;
  int _duration;
  std::string _location;
};

#endif