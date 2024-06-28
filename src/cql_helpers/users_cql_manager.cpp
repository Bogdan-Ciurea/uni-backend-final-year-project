/**
 * @file users_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the user object
 * @version 0.1
 * @date 2023-01-06
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "cql_helpers/users_cql_manager.hpp"

CqlResult UsersCqlManager::configure(bool init_db_schema) {
  CqlResult cql_result = ResultCode::OK;

  if (init_db_schema) {
    cql_result = init_schema();

    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << "Failed to initialise users table: " << cql_result.str_code()
                << cql_result.error();
      return cql_result;
    }
  }

  cql_result = init_prepare_statements();

  return cql_result;
}

CqlResult UsersCqlManager::init_prepare_statements() {
  CqlResult cql_result =
      _cql_client->prepare_statement(_prepared_insert_user, INSERT_USER);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise insert user statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_get_user, GET_USER);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select user by uuid prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_get_user_by_email,
                                              GET_USER_BY_EMAIL);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR
        << "Failed to initialise select user by email and password prepared "
           "statement: "
        << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_get_users_by_school,
                                              GET_USERS_BY_SCHOOL);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select users by school prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_update_user, UPDATE_USER);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise update user prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_delete_user, DELETE_USER);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise delete user prepared statement: "
              << cql_result.str_code() << cql_result.error();
  }

  return cql_result;
}

CqlResult UsersCqlManager::init_schema() {
  CqlResult cql_result = _cql_client->execute_statement(CREATE_SCHOOL_KEYSPACE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise school keyspace: "
              << cql_result.str_code() << cql_result.error();
    return cql_result;
  }

  cql_result = _cql_client->execute_statement(CREATE_USERS_TABLE);

  return cql_result;
}

CqlResult UsersCqlManager::create_user(const UserObject& user_data) {
  std::string error_message;

  try {
    CassStatementPtr statement(cass_prepared_bind(_prepared_insert_user.get()));

    cass_statement_bind_int32(statement.get(), 0, user_data._school_id);
    cass_statement_bind_uuid(statement.get(), 1, user_data._user_id);
    cass_statement_bind_string(statement.get(), 2, user_data._email.c_str());
    cass_statement_bind_string(statement.get(), 3, user_data._password.c_str());
    cass_statement_bind_int32(statement.get(), 4,
                              static_cast<int>(user_data._user_type));
    cass_statement_bind_bool(
        statement.get(), 5,
        user_data._changed_password ? cass_true : cass_false);
    cass_statement_bind_string(statement.get(), 6,
                               user_data._first_name.c_str());
    cass_statement_bind_string(statement.get(), 7,
                               user_data._last_name.c_str());
    cass_statement_bind_string(statement.get(), 8,
                               user_data._phone_number.c_str());
    cass_statement_bind_int64(statement.get(), 9,
                              user_data._last_time_online * 1000);

    auto [cql_result, cass_result] =
        _cql_client->execute_statement(statement.get());

    if (cql_result.code() == ResultCode::OK) {
      cql_result = was_applied(cass_result);
    }

    return cql_result;
  } catch (std::exception& ex) {
    error_message = ex.what();
  } catch (...) {
    error_message = "unknown exception";
  }

  return {ResultCode::UNKNOWN_ERROR, error_message};
}

std::pair<CqlResult, UserObject> UsersCqlManager::get_user(
    const int school_id, const CassUuid& user_id) {
  std::vector<UserObject> users;
  // Define the result handler that pre-allocates the memory for the users
  // vector
  auto reserve_lectures = [&users](size_t num_rows) {
    // Reserve capacity for all returned users
    users.reserve(num_rows);
  };

  // Define the row handler that will store the users
  auto copy_lecture_details = [&users](const CassRow* row) {
    users.emplace_back();
    UserObject& user = users.back();

    CqlResult cql_result = map_row_to_user(row, user);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(cass_prepared_bind(_prepared_get_user.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);
  cass_statement_bind_uuid(statement.get(), 1, user_id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_lectures, copy_lecture_details);

  if (cql_result.code() != ResultCode::OK) {
    return {cql_result, UserObject()};
  }

  if (users.size() != 1) {
    return {CqlResult(ResultCode::NOT_FOUND, "User not found"), UserObject()};
  }

  return {cql_result, users.at(0)};
}

std::pair<CqlResult, std::vector<UserObject>>
UsersCqlManager::get_users_by_school(const int school_id) {
  std::vector<UserObject> users;
  // Define the result handler that pre-allocates the memory for the users
  // vector
  auto reserve_lectures = [&users](size_t num_rows) {
    // Reserve capacity for all returned users
    users.reserve(num_rows);
  };

  // Define the row handler that will store the users
  auto copy_lecture_details = [&users](const CassRow* row) {
    users.emplace_back();
    UserObject& user = users.back();

    CqlResult cql_result = map_row_to_user(row, user);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_get_users_by_school.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_lectures, copy_lecture_details);

  if (cql_result.code() != ResultCode::OK) {
    return {cql_result, std::vector<UserObject>()};
  }

  return {cql_result, users};
}

std::pair<CqlResult, UserObject> UsersCqlManager::get_user_by_email(
    const int school_id, const std::string& email) {
  std::vector<UserObject> users;
  // Define the result handler that pre-allocates the memory for the users
  // vector
  auto reserve_lectures = [&users](size_t num_rows) {
    // Reserve capacity for all returned users
    users.reserve(num_rows);
  };

  // Define the row handler that will store the users
  auto copy_lecture_details = [&users](const CassRow* row) {
    users.emplace_back();
    UserObject& user = users.back();

    CqlResult cql_result = map_row_to_user(row, user);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_get_user_by_email.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);
  cass_statement_bind_string(statement.get(), 1, email.c_str());

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_lectures, copy_lecture_details);

  if (cql_result.code() != ResultCode::OK) {
    return {cql_result, UserObject()};
  }

  if (users.size() != 1) {
    return {CqlResult(ResultCode::NOT_FOUND, "User not found"), UserObject()};
  }

  return {cql_result, users.at(0)};
}

CqlResult UsersCqlManager::update_user(
    const int school_id, const CassUuid& user_id, const std::string& email,
    const std::string& password, const UserType& user_type,
    const bool& changed_password, const std::string& first_name,
    const std::string& last_name, const std::string& phone_number,
    const time_t& last_time_online) {
  std::string error_message;

  try {
    CassStatementPtr statement(cass_prepared_bind(_prepared_update_user.get()));

    cass_statement_bind_int32(statement.get(), 8, school_id);
    cass_statement_bind_uuid(statement.get(), 9, user_id);
    cass_statement_bind_string(statement.get(), 0, email.c_str());
    cass_statement_bind_string(statement.get(), 1, password.c_str());
    cass_statement_bind_int32(statement.get(), 2, static_cast<int>(user_type));
    cass_statement_bind_bool(statement.get(), 3,
                             changed_password ? cass_true : cass_false);
    cass_statement_bind_string(statement.get(), 4, first_name.c_str());
    cass_statement_bind_string(statement.get(), 5, last_name.c_str());
    cass_statement_bind_string(statement.get(), 6, phone_number.c_str());
    cass_statement_bind_int64(statement.get(), 7, last_time_online * 1000);

    auto [cql_result, cass_result] =
        _cql_client->execute_statement(statement.get());

    if (cql_result.code() == ResultCode::OK) {
      cql_result = was_applied(cass_result);
    }

    return cql_result;
  } catch (std::exception& ex) {
    error_message = ex.what();
  } catch (...) {
    error_message = "unknown exception";
  }

  return {ResultCode::UNKNOWN_ERROR, error_message};
}

CqlResult UsersCqlManager::delete_user(const int school_id,
                                       const CassUuid& user_id) {
  std::string error_message;

  try {
    CassStatementPtr statement(cass_prepared_bind(_prepared_delete_user.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, user_id);

    auto [cql_result, cass_result] =
        _cql_client->execute_statement(statement.get());

    if (cql_result.code() == ResultCode::OK) {
      cql_result = was_applied(cass_result);
    }

    return cql_result;
  } catch (std::exception& ex) {
    error_message = ex.what();
  } catch (...) {
    error_message = "unknown exception";
  }

  return {ResultCode::UNKNOWN_ERROR, error_message};
}

CqlResult map_row_to_user(const CassRow* row, UserObject& user) {
  CqlResult cql_result = get_int_value(row, 0, user._school_id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_uuid_value(row, 1, user._user_id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_text_value(row, 2, user._email);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_text_value(row, 3, user._password);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  int user_type;
  cql_result = get_int_value(row, 4, user_type);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }
  if (user_type < 0 || user_type > static_cast<int>(UserType::STUDENT)) {
    return {ResultCode::UNKNOWN_ERROR, "Invalid user type"};
  }
  if (user_type == 0)
    user._user_type = UserType::ADMIN;
  else if (user_type == 1)
    user._user_type = UserType::TEACHER;
  else if (user_type == 2)
    user._user_type = UserType::STUDENT;

  cql_result = get_bool_value(row, 5, user._changed_password);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_text_value(row, 6, user._first_name);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_text_value(row, 7, user._last_name);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_text_value(row, 8, user._phone_number);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_long_value(row, 9, user._last_time_online);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }
  user._last_time_online /= 1000;

  return CqlResult(ResultCode::OK);
}