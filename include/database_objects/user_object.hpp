/**
 * @file user_object.hpp
 * @author Bogdan Ciurea (sc20bac@leeds.ac.uk)
 * @brief Class that stores the user object
 * @version 0.1
 * @date 2023-01-06
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef USER_OBJECT_H
#define USER_OBJECT_H

#include <json/json.h>

#include <string>
#include <vector>

#include "cassandra.h"

enum class UserType { ADMIN, TEACHER, STUDENT };

class UserObject {
 public:
  UserObject(){};
  UserObject(int school_id, std::string user_id, std::string email,
             std::string password, UserType user_type, bool changed_password,
             std::string first_name, std::string last_name,
             std::string phone_number, time_t last_time_online)
      : _school_id(school_id),
        _email(std::move(email)),
        _password(std::move(password)),
        _user_type(user_type),
        _changed_password(changed_password),
        _first_name(std::move(first_name)),
        _last_name(std::move(last_name)),
        _phone_number(std::move(phone_number)),
        _last_time_online(last_time_online) {
    cass_uuid_from_string(user_id.c_str(), &_user_id);
  }
  UserObject(int school_id, CassUuid user_id, std::string email,
             std::string password, UserType user_type, bool changed_password,
             std::string first_name, std::string last_name,
             std::string phone_number, time_t last_time_online)
      : _school_id(school_id),
        _user_id(user_id),
        _email(std::move(email)),
        _password(std::move(password)),
        _user_type(user_type),
        _changed_password(changed_password),
        _first_name(std::move(first_name)),
        _last_name(std::move(last_name)),
        _phone_number(std::move(phone_number)),
        _last_time_online(last_time_online) {}
  ~UserObject(){};

  /**
   * @brief Returns a Json object containing the information about this object
   *
   * @param secure If we just want the most important information
   * @return The Json object
   */
  Json::Value to_json(bool secure);

  int _school_id;
  CassUuid _user_id;
  std::string _email;
  std::string _password;
  UserType _user_type;
  bool _changed_password;
  std::string _first_name;
  std::string _last_name;
  std::string _phone_number;
  time_t _last_time_online;
};

#endif