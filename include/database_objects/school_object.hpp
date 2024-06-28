/**
 * @file school_object.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief Class that stores the school object
 * @version 0.1
 * @date 2022-11-23
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef SCHOOL_OBJECT_H
#define SCHOOL_OBJECT_H

#include "country_object.hpp"

class SchoolObject {
 public:
  SchoolObject() {}
  SchoolObject(int id) : _id(std::move(id)) {}
  SchoolObject(int id, std::string _name, const int country,
               std::string image_path)
      : _id(id),
        _name(std::move(_name)),
        _country_id(country),
        _image_path(std::move(image_path)) {}
  ~SchoolObject() {}

  /**
   * @brief Returns a Json object containing the information about this object
   *
   * @param secure If we just want the most important information
   * @return The Json object
   */
  Json::Value to_json(bool secure);

  int _id;
  std::string _name;
  int _country_id;
  std::string _image_path;

  CountryObject _country;
  std::vector<HolidayObject> _holidays;
};

#endif