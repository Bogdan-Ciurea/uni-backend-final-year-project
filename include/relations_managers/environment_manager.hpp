/**
 * @file environment_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief The purpose of this class is to manage the environment objects
 * of the database.
 * @version 0.1
 * @date 2023-01-25
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef ENVIRONMENT_MANAGER_H
#define ENVIRONMENT_MANAGER_H

#include "../cql_helpers/country_cql_manager.hpp"
#include "../cql_helpers/holiday_cql_manager.hpp"
#include "../cql_helpers/school_cql_manager.hpp"
#include "drogon/drogon.h"

class EnvironmentManager {
 public:
  EnvironmentManager(SchoolCqlManager *schoolCqlManager,
                     HolidayCqlManager *holidayCqlManager,
                     CountryCqlManager *countryCqlManager)
      : _school_cql_manager(schoolCqlManager),
        _holiday_cql_manager(holidayCqlManager),
        _country_cql_manager(countryCqlManager) {}
  ~EnvironmentManager() {}

  ///
  /// The methods bellow are used to manage the environment objects.
  /// They are used to create, read, update and delete the environment objects.
  /// They are also used to validate the environment objects.
  /// If the environment object is valid, the method will return a pair with
  /// the HTTP status code 200 or 201 and an empty JSON object.
  /// If the environment object is not valid, the method will return a
  /// pair with the HTTP status code 400 or 404 and a JSON object containing the
  /// error message. If the environment object could not be created, read,
  /// updated or deleted due to internal errors, the method will return a pair
  /// with the HTTP status code 500 and a JSON object containing the error
  /// message.
  ///

  // School related methods
  std::pair<drogon::HttpStatusCode, Json::Value> create_school(
      const std::string &name, const int country_id,
      const std::string &image_path);
  std::pair<drogon::HttpStatusCode, Json::Value> get_school(
      const int school_id);
  std::pair<drogon::HttpStatusCode, Json::Value> get_all_schools();
  std::pair<drogon::HttpStatusCode, Json::Value> update_school(
      const int school_id, const std::string &name, const int country_id,
      std::string &image_path);
  std::pair<drogon::HttpStatusCode, Json::Value> delete_school(
      const int school_id);

  // Country related methods
  std::pair<drogon::HttpStatusCode, Json::Value> create_country(
      const std::string &name, const std::string &code);
  std::pair<drogon::HttpStatusCode, Json::Value> get_country(
      const int country_id);
  std::pair<drogon::HttpStatusCode, Json::Value> get_all_countries();
  std::pair<drogon::HttpStatusCode, Json::Value> update_country(
      const int country_id, const std::string &name, const std::string &code);
  std::pair<drogon::HttpStatusCode, Json::Value> delete_country(
      const int country_id);

  // Holiday related methods
  std::pair<drogon::HttpStatusCode, Json::Value> create_holiday(
      const int country_or_school_id, const HolidayType type, const time_t date,
      const std::string &name);
  std::pair<drogon::HttpStatusCode, Json::Value> get_holidays(
      int country_or_school_id, HolidayType type);
  std::pair<drogon::HttpStatusCode, Json::Value> delete_holiday(
      const int country_or_school_id, const HolidayType type,
      const time_t date);
  std::pair<drogon::HttpStatusCode, Json::Value> delete_holidays(
      const int country_or_school_id, const HolidayType type);

 private:
  SchoolCqlManager *_school_cql_manager;
  HolidayCqlManager *_holiday_cql_manager;
  CountryCqlManager *_country_cql_manager;

  /**
   * @brief Sees if the country is in the database.
   *
   * @param country The id of the school
   * @return  1     If the id is valid
   * @return  0     If the id is not valid
   * @return -1     If the id could not be validated due to internal errors
   */
  int validate_country_id(const int &country);

  /**
   * @brief Generates a unique id for a school.
   *
   * @return int  The id of the school
   * @return -1   If the id could not be generated
   */
  int generate_school_id();

  /**
   * @brief Generates a unique id for a country.
   *
   * @return int  The id of the school
   * @return -1   If the id could not be generated
   */
  int generate_country_id();

  /**
   * @brief Validates the name of a school. It must be between 1 and 50
   * characters and it must not contain any special characters. It must also not
   * be empty and it must be unique.
   *
   * @param name  The name of the school
   * @return  1   If the name is valid
   * @return  0   If the name is not valid
   * @return -1   If the id could not be validated due to internal errors
   */
  int validate_school_name(const std::string &name);

  /**
   * @brief Validates the name of a country. It must be between 1 and 50
   * characters and it must not contain any special characters. It must also not
   * be empty and it must be unique.
   *
   * @param name  The name of the school
   * @return  1   If the name is valid
   * @return  0   If the name is not valid
   * @return -1   If the id could not be validated due to internal errors
   */
  int validate_country_name(const std::string &name);

  /**
   * @brief Validates the code of a country. It must be between 1 and 50
   * characters and it must not contain any special characters. It must also not
   * be empty and it must be unique.
   *
   * @param code  The code of the school
   * @return  1   If the name is valid
   * @return  0   If the name is not valid
   * @return -1   If the id could not be validated due to internal errors
   */
  int validate_country_code(const std::string &code);
};

#endif