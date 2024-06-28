/**
 * @file school_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the school object
 * @version 0.1
 * @date 2022-12-10
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "cql_helpers/school_cql_manager.hpp"

CqlResult SchoolCqlManager::configure(bool init_db_schema) {
  CqlResult cql_result = ResultCode::OK;

  if (init_db_schema) {
    cql_result = init_schema();

    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << "Failed to initialise country table: "
                << cql_result.str_code() << cql_result.error();
      return cql_result;
    }
  }

  cql_result = init_prepare_statements();

  return cql_result;
}

CqlResult SchoolCqlManager::init_prepare_statements() {
  CqlResult cql_result =
      _cql_client->prepare_statement(_prepared_insert_school, INSERT_SCHOOL);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise insert school prepared statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_select_school, SELECT_SCHOOL);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select school prepared statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_select_all_schools,
                                              SELECT_ALL_SCHOOLS);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select all schools prepared statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_update_school, UPDATE_SCHOOL);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise update school prepared statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_delete_school, DELETE_SCHOOL);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise delete school prepared statement: "
              << cql_result.str_code() << cql_result.error();
  }

  return cql_result;
}

CqlResult SchoolCqlManager::init_schema() {
  CqlResult cql_result =
      _cql_client->execute_statement(CREATE_ENVIRONMENT_KEYSPACE);

  if (cql_result.code() != ResultCode::OK) {
    return cql_result;
  }

  cql_result = _cql_client->execute_statement(CREATE_TABLE_SCHOOLS);

  return cql_result;
}

CqlResult SchoolCqlManager::create_school(const SchoolObject& school_data) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_insert_school.get()));

    cass_statement_bind_int32(statement.get(), 0, school_data._id);
    cass_statement_bind_string(statement.get(), 1, school_data._name.c_str());
    cass_statement_bind_int32(statement.get(), 2, school_data._country_id);
    cass_statement_bind_string(statement.get(), 3,
                               school_data._image_path.c_str());

    auto [cql_result, cass_result] =
        _cql_client->execute_statement(statement.get());

    if (cql_result.code() == ResultCode::OK)
      cql_result = was_applied(cass_result);

    return cql_result;
  } catch (std::exception& ex) {
    error_message = ex.what();
  } catch (...) {
    error_message = "unknown exception";
  }

  return {ResultCode::UNKNOWN_ERROR, error_message};
}

std::pair<CqlResult, SchoolObject> SchoolCqlManager::get_school(const int id) {
  std::vector<SchoolObject> schools;

  // Define the result handler that pre-allocate space in schools vector
  auto reserve_schools = [&schools](size_t num_rows) {
    // Reserve capacity for all returned schools
    schools.reserve(num_rows);
  };

  // Define the row handler that will store the schools
  auto copy_school_details = [&schools](const CassRow* row) {
    schools.emplace_back();
    SchoolObject& school = schools.back();

    CqlResult cql_result = map_row_to_school(row, school);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(cass_prepared_bind(_prepared_select_school.get()));
  cass_statement_bind_int32(statement.get(), 0, id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_schools, copy_school_details);

  if (cql_result.code() != ResultCode::OK) return {cql_result, SchoolObject()};

  if (schools.size() != 1)
    return {{ResultCode::NOT_FOUND, "No entries found"}, SchoolObject()};

  return {cql_result, schools.at(0)};
}

std::pair<CqlResult, std::vector<SchoolObject>>
SchoolCqlManager::get_all_schools() {
  std::vector<SchoolObject> schools;

  // Define the result handler that pre-allocate space in schools vector
  auto reserve_schools = [&schools](size_t num_rows) {
    // Reserve capacity for all returned schools
    schools.reserve(num_rows);
  };

  // Define the row handler that will store the schools
  auto copy_school_details = [&schools](const CassRow* row) {
    schools.emplace_back();
    SchoolObject& school = schools.back();

    CqlResult cql_result = map_row_to_school(row, school);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_select_all_schools.get()));

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_schools, copy_school_details);

  if (cql_result.code() != ResultCode::OK) return {cql_result, schools};

  return {cql_result, schools};
}

CqlResult SchoolCqlManager::update_school(const int id, const std::string name,
                                          const int country_id,
                                          std::string image_path) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_update_school.get()));

    cass_statement_bind_string(statement.get(), 0, name.c_str());
    cass_statement_bind_int32(statement.get(), 1, country_id);
    cass_statement_bind_string(statement.get(), 2, image_path.c_str());
    cass_statement_bind_int32(statement.get(), 3, id);

    auto [cql_result, cass_result] =
        _cql_client->execute_statement(statement.get());

    if (cql_result.code() == ResultCode::OK)
      cql_result = was_applied(cass_result);

    return cql_result;
  } catch (std::exception& ex) {
    error_message = ex.what();
  } catch (...) {
    error_message = "unknown exception";
  }

  return {ResultCode::UNKNOWN_ERROR, error_message};
}

CqlResult SchoolCqlManager::delete_school(const int id) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_delete_school.get()));

    cass_statement_bind_int32(statement.get(), 0, id);

    auto [cql_result, cass_result] =
        _cql_client->execute_statement(statement.get());

    if (cql_result.code() == ResultCode::OK)
      cql_result = was_applied(cass_result);

    return cql_result;
  } catch (std::exception& ex) {
    error_message = ex.what();
  } catch (...) {
    error_message = "unknown exception";
  }

  return {ResultCode::UNKNOWN_ERROR, error_message};
}

CqlResult map_row_to_school(const CassRow* row, SchoolObject& school) {
  CqlResult cql_result = get_int_value(row, 0, school._id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_text_value(row, 1, school._name);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_int_value(row, 2, school._country_id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_text_value(row, 3, school._image_path);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  return {ResultCode::OK};
}