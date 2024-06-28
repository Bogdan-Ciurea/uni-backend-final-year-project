/**
 * @file school_object.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief Class that stores the school object
 * @version 0.1
 * @date 2022-12-01
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "database_objects/school_object.hpp"

Json::Value SchoolObject::to_json(bool secure) {
  Json::Value value;

  if (!secure) {
    value["id"] = _id;
  }

  value["name"] = _name;
  value["country_id"] = _country_id;
  value["image_path"] = _image_path;

  for (HolidayObject& holiday : _holidays)
    value["holidays"].append(holiday.to_json(secure));

  return value;
}