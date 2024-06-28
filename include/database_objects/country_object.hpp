/**
 * @file country_object.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief Class that stores the country object
 * @version 0.1
 * @date 2022-11-23
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef COUNTRY_OBJECT_H
#define COUNTRY_OBJECT_H

#include <string>
#include <vector>

#include "holiday_object.hpp"

class CountryObject {
 public:
  CountryObject() {}
  CountryObject(int id, std::string name, std::string code)
      : _id(id), _name(std::move(name)), _code(std::move(code)) {}
  ~CountryObject() {}

  /**
   * @brief Returns a Json object containing the information about this object
   *
   * @param secure If we just want the most important information
   * @return The Json object
   */
  Json::Value to_json(bool secure);

  int _id;
  std::string _name;
  std::string _code;
  std::vector<HolidayObject> _holidays;
};

#endif