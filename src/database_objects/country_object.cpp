#include "database_objects/country_object.hpp"

Json::Value CountryObject::to_json(bool secure) {
  Json::Value value;
  if (!secure) {
    value["id"] = _id;
    value["code"] = _code;
  }

  value["name"] = _name;

  if (_holidays.size())
    for (HolidayObject& holiday : _holidays)
      value["holidays"].append(holiday.to_json(secure));

  return value;
}