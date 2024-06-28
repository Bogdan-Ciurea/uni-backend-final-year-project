#include "relations_managers/environment_manager.hpp"

std::pair<drogon::HttpStatusCode, Json::Value>
EnvironmentManager::create_school(const std::string &name, const int country_id,
                                  const std::string &image_path) {
  Json::Value json_response;

  // Validate the name of the school
  const int name_validation = validate_school_name(name);
  if (name_validation == 0) {
    json_response["error"] = "Invalid school name";
    return std::make_pair(drogon::HttpStatusCode::k400BadRequest,
                          json_response);
  } else if (name_validation == -1) {
    json_response["error"] = "Internal error";
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  // Validate the country id
  const int country_validation = validate_country_id(country_id);
  if (country_validation == 0) {
    json_response["error"] = "Invalid country id";
    return std::make_pair(drogon::HttpStatusCode::k400BadRequest,
                          json_response);
  } else if (country_validation == -1) {
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  // Generate the school id
  const int id = generate_school_id();
  if (id == -1) {
    json_response["error"] = "Internal error";
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  // Create the school
  SchoolObject school_object(id, name, country_id, image_path);
  CqlResult cql_result = _school_cql_manager->create_school(school_object);
  // Check if the school was created
  if (cql_result.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  // Return ok status
  return std::make_pair(drogon::HttpStatusCode::k201Created, json_response);
}

std::pair<drogon::HttpStatusCode, Json::Value> EnvironmentManager::get_school(
    const int school_id) {
  Json::Value json_response;

  // Get the school
  std::pair<CqlResult, SchoolObject> result =
      _school_cql_manager->get_school(school_id);

  if (result.first.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "School not found";
    return std::make_pair(drogon::HttpStatusCode::k404NotFound, json_response);
  } else if (result.first.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  // Return the school
  return std::make_pair(drogon::HttpStatusCode::k200OK,
                        result.second.to_json(false));
}

std::pair<drogon::HttpStatusCode, Json::Value>
EnvironmentManager::get_all_schools() {
  Json::Value json_response;

  // Get all schools
  auto [cql_answer, schools] = _school_cql_manager->get_all_schools();

  if (cql_answer.code() != ResultCode::OK &&
      cql_answer.code() != ResultCode::NOT_FOUND) {
    json_response["error"] = "Internal error";
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  // Return all schools
  for (auto &school_object : schools) {
    json_response.append(school_object.to_json(false));
  }
  return std::make_pair(drogon::HttpStatusCode::k200OK, json_response);
}

std::pair<drogon::HttpStatusCode, Json::Value>
EnvironmentManager::update_school(const int school_id, const std::string &name,
                                  const int country_id,
                                  std::string &image_path) {
  Json::Value json_response;

  // Validate the name of the school
  const int name_validation = validate_school_name(name);
  if (name_validation == 0) {
    json_response["error"] = "Invalid school name";
    return std::make_pair(drogon::HttpStatusCode::k400BadRequest,
                          json_response);
  } else if (name_validation == -1) {
    json_response["error"] = "Internal error";
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  // Validate the country id
  const int country_validation = validate_country_id(country_id);
  if (country_validation == 0) {
    json_response["error"] = "Invalid country id";
    return std::make_pair(drogon::HttpStatusCode::k400BadRequest,
                          json_response);
  } else if (country_validation == -1) {
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  // Check if the image path was given
  if (image_path.empty()) {
    // Get the school
    std::pair<CqlResult, SchoolObject> result =
        _school_cql_manager->get_school(school_id);

    if (result.first.code() == ResultCode::NOT_FOUND) {
      json_response["error"] = "School not found";
      return std::make_pair(drogon::HttpStatusCode::k404NotFound,
                            json_response);
    } else if (result.first.code() != ResultCode::OK) {
      json_response["error"] = "Internal error";
      return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                            json_response);
    }

    // Set the image path
    image_path = result.second._image_path;
  }

  // Update the school
  CqlResult cql_result = _school_cql_manager->update_school(
      school_id, name, country_id, image_path);

  if (cql_result.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "School not found";
    return std::make_pair(drogon::HttpStatusCode::k404NotFound, json_response);
  } else if (cql_result.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  // Return ok status
  return std::make_pair(drogon::HttpStatusCode::k200OK, json_response);
}

std::pair<drogon::HttpStatusCode, Json::Value>
EnvironmentManager::delete_school(const int school_id) {
  Json::Value json_response;

  // Delete the school
  CqlResult cql_result = _school_cql_manager->delete_school(school_id);

  if (cql_result.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "School not found";
    return std::make_pair(drogon::HttpStatusCode::k404NotFound, json_response);
  } else if (cql_result.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  // Delete the school's custom holidays
  cql_result =
      _holiday_cql_manager->delete_holidays(school_id, HolidayType::CUSTOM);

  if (cql_result.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  // Return ok status
  return std::make_pair(drogon::HttpStatusCode::k200OK, json_response);
}

// Country related methods
std::pair<drogon::HttpStatusCode, Json::Value>
EnvironmentManager::create_country(const std::string &name,
                                   const std::string &code) {
  Json::Value json_response;

  // Validate the name of the country
  const int name_validation = validate_country_name(name);
  if (name_validation == 0) {
    json_response["error"] = "Invalid country name";
    return std::make_pair(drogon::HttpStatusCode::k400BadRequest,
                          json_response);
  } else if (name_validation == -1) {
    json_response["error"] = "Internal error";
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  // Validate the code of the country
  const int code_validation = validate_country_code(code);
  if (code_validation == 0) {
    json_response["error"] = "Invalid country code";
    return std::make_pair(drogon::HttpStatusCode::k400BadRequest,
                          json_response);
  } else if (code_validation == -1) {
    json_response["error"] = "Internal error";
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  // Generate the id of the country
  const int country_id = generate_country_id();

  // Create the country
  CountryObject country_object(country_id, name, code);
  CqlResult result = _country_cql_manager->create_country(country_object);

  if (result.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  return std::make_pair(drogon::HttpStatusCode::k201Created, json_response);
}

std::pair<drogon::HttpStatusCode, Json::Value> EnvironmentManager::get_country(
    const int country_id) {
  Json::Value json_response;

  // Get the country
  auto [cql_result, country] = _country_cql_manager->get_country(country_id);

  if (cql_result.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Country not found";
    return std::make_pair(drogon::HttpStatusCode::k404NotFound, json_response);
  } else if (cql_result.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  // Return the country object
  return std::make_pair(drogon::HttpStatusCode::k200OK, country.to_json(false));
}

std::pair<drogon::HttpStatusCode, Json::Value>
EnvironmentManager::get_all_countries() {
  Json::Value json_response;

  // Get all the countries
  auto [cql_result, countries] = _country_cql_manager->get_all_countries();

  if (cql_result.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  // Return the countries
  for (auto &country : countries) {
    json_response.append(country.to_json(false));
  }
  return std::make_pair(drogon::HttpStatusCode::k200OK, json_response);
}

std::pair<drogon::HttpStatusCode, Json::Value>
EnvironmentManager::update_country(const int country_id,
                                   const std::string &name,
                                   const std::string &code) {
  Json::Value json_response;

  // Check if the country exists
  auto [cql_result, country] = _country_cql_manager->get_country(country_id);

  if (cql_result.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Country not found";
    return std::make_pair(drogon::HttpStatusCode::k404NotFound, json_response);
  } else if (cql_result.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  // Validate the name of the country
  const int name_validation = validate_country_name(name);
  if (name_validation == 0) {
    json_response["error"] = "Invalid country name";
    return std::make_pair(drogon::HttpStatusCode::k400BadRequest,
                          json_response);
  } else if (name_validation == -1) {
    json_response["error"] = "Internal error";
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  // Validate the code of the country
  const int code_validation = validate_country_code(code);
  if (code_validation == 0) {
    json_response["error"] = "Invalid country code";
    return std::make_pair(drogon::HttpStatusCode::k400BadRequest,
                          json_response);
  } else if (code_validation == -1) {
    json_response["error"] = "Internal error";
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  // Update the country
  CqlResult cql_response =
      _country_cql_manager->update_country(country_id, name, code);

  if (cql_response.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Country not found";
    return std::make_pair(drogon::HttpStatusCode::k404NotFound, json_response);
  } else if (cql_response.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  // Return ok status
  return std::make_pair(drogon::HttpStatusCode::k200OK, json_response);
}

std::pair<drogon::HttpStatusCode, Json::Value>
EnvironmentManager::delete_country(const int country_id) {
  Json::Value json_response;

  // Delete the country
  CqlResult cql_result = _country_cql_manager->delete_country(country_id);

  if (cql_result.code() == ResultCode::NOT_APPLIED) {
    json_response["error"] = "Country not found";
    return std::make_pair(drogon::HttpStatusCode::k404NotFound, json_response);
  } else if (cql_result.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  // Delete the holidays of the country
  cql_result =
      _holiday_cql_manager->delete_holidays(country_id, HolidayType::NATIONAL);

  if (cql_result.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  // Delete all schools of the country
  auto [cql_response, schools] = _school_cql_manager->get_all_schools();

  if (cql_response.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  for (auto &school : schools) {
    if (school._country_id == country_id) {
      auto [http_response, json_response_from_delete_school] =
          delete_school(school._id);
      if (http_response != drogon::HttpStatusCode::k200OK) {
        return std::make_pair(http_response, json_response_from_delete_school);
      }
    }
  }

  // Return ok status
  return std::make_pair(drogon::HttpStatusCode::k200OK, json_response);
}

// Holiday related methods
std::pair<drogon::HttpStatusCode, Json::Value>
EnvironmentManager::create_holiday(const int country_or_school_id,
                                   const HolidayType type, const time_t date,
                                   const std::string &name) {
  Json::Value json_response;

  // Create the holiday
  CqlResult cql_result(ResultCode::OK);
  if (type == HolidayType::NATIONAL) {
    // Check if the country exists
    auto [cql_response, country] =
        _country_cql_manager->get_country(country_or_school_id);
    if (cql_response.code() == ResultCode::NOT_FOUND) {
      json_response["error"] = "Country not found";
      return std::make_pair(drogon::HttpStatusCode::k404NotFound,
                            json_response);
    } else if (cql_response.code() != ResultCode::OK) {
      json_response["error"] = "Internal error";
      return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                            json_response);
    }

    // Create the holiday
    HolidayObject holiday(country._id, HolidayType::NATIONAL, date, name);
    cql_result = _holiday_cql_manager->create_holiday(holiday);
  } else if (type == HolidayType::CUSTOM) {
    // Check if the school exists
    auto [cql_response, school] =
        _school_cql_manager->get_school(country_or_school_id);

    if (cql_response.code() == ResultCode::NOT_FOUND) {
      json_response["error"] = "School not found";
      return std::make_pair(drogon::HttpStatusCode::k404NotFound,
                            json_response);
    } else if (cql_response.code() != ResultCode::OK) {
      json_response["error"] = "Internal error";
      return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                            json_response);
    }

    // Create the holiday
    HolidayObject holiday(school._id, HolidayType::CUSTOM, date, name);
    cql_result = _holiday_cql_manager->create_holiday(holiday);
  } else {
    json_response["error"] = "Invalid holiday type";
    return std::make_pair(drogon::HttpStatusCode::k400BadRequest,
                          json_response);
  }

  if (cql_result.code() == ResultCode::NOT_APPLIED) {
    json_response["error"] = "Country or school not found";
    return std::make_pair(drogon::HttpStatusCode::k404NotFound, json_response);
  } else if (cql_result.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  // Return ok status
  return std::make_pair(drogon::HttpStatusCode::k201Created, json_response);
}

std::pair<drogon::HttpStatusCode, Json::Value> EnvironmentManager::get_holidays(
    int country_or_school_id, HolidayType type) {
  Json::Value json_response;

  if (type == HolidayType::CUSTOM) {
    // Check if the school exists
    auto [cql_response, school] =
        _school_cql_manager->get_school(country_or_school_id);

    if (cql_response.code() == ResultCode::NOT_FOUND) {
      Json::Value json_response;
      json_response["error"] = "School not found";
      return std::make_pair(drogon::HttpStatusCode::k404NotFound,
                            json_response);
    } else if (cql_response.code() != ResultCode::OK) {
      Json::Value json_response;
      json_response["error"] = "Internal error";
      return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                            json_response);
    }

    // Get the holidays
    auto [cql_result, holidays] =
        _holiday_cql_manager->get_holidays(school._id, HolidayType::CUSTOM);

    if (cql_result.code() != ResultCode::OK &&
        cql_result.code() != ResultCode::NOT_FOUND)
      return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                            json_response);

    // Return the holidays
    for (auto &holiday : holidays) {
      json_response.append(holiday.to_json(false));
    }

    country_or_school_id = school._country_id;
    type = HolidayType::NATIONAL;
  }

  // Check if the country exists
  auto [cql_response, country] =
      _country_cql_manager->get_country(country_or_school_id);

  if (cql_response.code() == ResultCode::NOT_FOUND) {
    Json::Value json_response;
    json_response["error"] = "Country not found";
    return std::make_pair(drogon::HttpStatusCode::k404NotFound, json_response);
  } else if (cql_response.code() != ResultCode::OK) {
    Json::Value json_response;
    json_response["error"] = "Internal error";
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  // Get the holidays
  auto [cql_result, holidays] =
      _holiday_cql_manager->get_holidays(country._id, type);

  if (cql_result.code() != ResultCode::OK &&
      cql_result.code() != ResultCode::NOT_FOUND)
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);

  // Return the holidays
  for (auto &holiday : holidays) {
    json_response.append(holiday.to_json(false));
  }

  return std::make_pair(drogon::HttpStatusCode::k200OK, json_response);
}

std::pair<drogon::HttpStatusCode, Json::Value>
EnvironmentManager::delete_holiday(const int country_or_school_id,
                                   const HolidayType type, const time_t date) {
  Json::Value json_response;

  HolidayObject holiday(country_or_school_id, type, date, "");
  CqlResult cql_result = _holiday_cql_manager->delete_specific_holiday(holiday);

  if (cql_result.code() == ResultCode::NOT_APPLIED) {
    json_response["error"] = "Holiday not found";
    return std::make_pair(drogon::HttpStatusCode::k404NotFound, json_response);
  } else if (cql_result.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  return std::make_pair(drogon::HttpStatusCode::k200OK, json_response);
}

std::pair<drogon::HttpStatusCode, Json::Value>
EnvironmentManager::delete_holidays(const int country_or_school_id,
                                    const HolidayType type) {
  Json::Value json_response;

  CqlResult cql_result =
      _holiday_cql_manager->delete_holidays(country_or_school_id, type);

  if (cql_result.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return std::make_pair(drogon::HttpStatusCode::k500InternalServerError,
                          json_response);
  }

  return std::make_pair(drogon::HttpStatusCode::k200OK, json_response);
}

int EnvironmentManager::validate_country_id(const int &country) {
  if (country <= 0) return 0;

  auto [cql_result, country_object] =
      _country_cql_manager->get_country(country);

  if (cql_result.code() == ResultCode::NOT_FOUND) return 0;
  if (cql_result.code() != ResultCode::OK) return -1;

  return 1;
}

int EnvironmentManager::generate_school_id() {
  auto [cql_result, school_object] = _school_cql_manager->get_all_schools();

  if (cql_result.code() != ResultCode::OK &&
      cql_result.code() != ResultCode::NOT_FOUND)
    return -1;

  int school_id = 1;

  // Keep in mind that the list of schools is not sorted
  std::unordered_map<int, int> school_ids;

  for (auto &school : school_object) {
    school_ids[school._id] = 1;
  }

  while (school_ids.find(school_id) != school_ids.end()) {
    school_id++;
  }

  return school_id;
}

int EnvironmentManager::generate_country_id() {
  auto [cql_result, country_object] = _country_cql_manager->get_all_countries();

  if (cql_result.code() != ResultCode::OK &&
      cql_result.code() != ResultCode::NOT_FOUND)
    return -1;
  if (cql_result.code() == ResultCode::NOT_FOUND) return 1;

  int country_id = 1;

  // Keep in mind that the list of countries is not sorted
  std::unordered_map<int, int> country_ids;

  for (auto &country : country_object) {
    country_ids[country._id] = 1;
  }

  while (country_ids.find(country_id) != country_ids.end()) {
    country_id++;
  }

  return country_id;
}

int EnvironmentManager::validate_school_name(const std::string &name) {
  if (name.empty()) return 0;
  if (name.length() > 50) return 0;

  // Accepted characters: a-z, A-Z, 0-9, space, - and '
  for (auto &c : name) {
    if (c < '0' || (c > '9' && c < 'A') || (c > 'Z' && c < 'a') || c > 'z') {
      if (c != ' ' && c != '-') return 0;
    }
  }

  auto [cql_result, schools_object] = _school_cql_manager->get_all_schools();

  if (cql_result.code() != ResultCode::OK &&
      cql_result.code() != ResultCode::NOT_FOUND)
    return -1;

  for (auto &school : schools_object) {
    if (school._name == name) return 0;
  }

  return 1;
}

int EnvironmentManager::validate_country_name(const std::string &name) {
  if (name.empty()) return 0;
  if (name.size() > 50) return 0;

  // Accepted characters: a-z, A-Z, 0-9, space, - and '
  for (auto &c : name) {
    if (c < '0' || (c > '9' && c < 'A') || (c > 'Z' && c < 'a') || c > 'z') {
      if (c != ' ' && c != '-') return 0;
    }
  }

  auto [cql_result, countries_object] =
      _country_cql_manager->get_all_countries();

  if (cql_result.code() != ResultCode::OK &&
      cql_result.code() != ResultCode::NOT_FOUND)
    return -1;

  for (auto &country : countries_object) {
    if (country._name == name) return 0;
  }

  return 1;
}

int EnvironmentManager::validate_country_code(const std::string &code) {
  if (code.empty()) return 0;
  if (code.size() > 50) return 0;

  // Accepted characters: a-z, A-Z
  for (auto &c : code) {
    if (c < 'A' || (c > 'Z' && c < 'a') || c > 'z') return 0;
  }

  auto [cql_result, countries_object] =
      _country_cql_manager->get_all_countries();

  if (cql_result.code() != ResultCode::OK &&
      cql_result.code() != ResultCode::NOT_FOUND)
    return -1;

  for (auto &country : countries_object) {
    if (country._code == code) return 0;
  }

  return 1;
}
