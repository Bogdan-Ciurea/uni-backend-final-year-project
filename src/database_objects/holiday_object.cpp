#include "database_objects/holiday_object.hpp"

HolidayType holiday_type_from_int(const int htype) {
  switch (htype) {
    case 0:
      return HolidayType::NATIONAL;
      break;
    case 1:
      return HolidayType::CUSTOM;
      break;

    default:
      break;
  }

  return HolidayType::ERROR;
}

int holiday_type_to_int(const HolidayType type) {
  switch (type) {
    case HolidayType::NATIONAL:
      return 0;
      break;

    case HolidayType::CUSTOM:
      return 1;
      break;

    case HolidayType::ERROR:
      return 2;
      break;

    default:
      break;
  }

  return 2;
}

std::string holiday_type_to_string(const HolidayType htype) {
  switch (htype) {
    case HolidayType::NATIONAL:
      return "NATIONAL";
      break;
    case HolidayType::CUSTOM:
      return "CUSTOM";
      break;
    case HolidayType::ERROR:
      return "ERROR";
      break;
    default:
      break;
  }

  return "No Known Type";
}

Json::Value HolidayObject::to_json(bool secure) {
  Json::Value value;

  if (!secure) {
    value["country_or_school_id"] = _country_or_school_id;
    value["type"] = holiday_type_to_string(_type);
  }

  value["time"] = (Json::Value::Int64)_date;
  value["name"] = _name;

  return value;
}