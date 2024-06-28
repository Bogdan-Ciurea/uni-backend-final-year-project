/**
 * @file holiday_object.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief Class that stores the holiday object
 * @version 0.1
 * @date 2022-11-23
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef HOLIDAY_OBJECT_H
#define HOLIDAY_OBJECT_H

#include <json/json.h>

#include <string>

enum class HolidayType { NATIONAL = 0, CUSTOM = 1, ERROR = 2 };

class HolidayObject {
 public:
  int _country_or_school_id;
  HolidayType _type;
  time_t _date;
  std::string _name;

  HolidayObject() {}
  HolidayObject(int country_id, HolidayType htype, time_t time,
                std::string name)
      : _country_or_school_id(country_id),
        _type(htype),
        _date(time),
        _name(std::move(name)) {}
  ~HolidayObject() {}

  /**
   * @brief Returns a Json object containing the information about this object
   *
   * @param secure If we just want the most important information
   * @return The Json object
   */
  Json::Value to_json(bool secure);
};

HolidayType holiday_type_from_int(const int htype);

int holiday_type_to_int(const HolidayType type);

std::string holiday_type_to_string(const HolidayType htype);

#endif