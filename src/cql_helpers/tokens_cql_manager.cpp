/**
 * @file tokens_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the users tokens
 * @version 0.1
 * @date 2023-01-06
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "cql_helpers/tokens_cql_manager.hpp"

CqlResult TokensCqlManager::configure(bool init_db_schema) {
  CqlResult cql_result = ResultCode::OK;

  if (init_db_schema) {
    cql_result = init_schema();

    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << "Failed to initialise users_id table: "
                << cql_result.str_code() << cql_result.error();
      return cql_result;
    }
  }

  cql_result = init_prepare_statements();

  return cql_result;
}

CqlResult TokensCqlManager::init_prepare_statements() {
  CqlResult cql_result =
      _cql_client->prepare_statement(_prepared_insert_token, INSERT_TOKEN);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise insert token statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_get_user_from_token,
                                              GET_USER_FROM_TOKEN);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select user from token prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_delete_token, DELETE_TOKEN);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise delete token prepared statement: "
              << cql_result.str_code() << cql_result.error();
  }

  return cql_result;
}

CqlResult TokensCqlManager::init_schema() {
  CqlResult cql_result = _cql_client->execute_statement(CREATE_SCHOOL_KEYSPACE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise school keyspace: "
              << cql_result.str_code() << cql_result.error();
    return cql_result;
  }

  cql_result = _cql_client->execute_statement(CREATE_TOKENS_TABLE);

  return cql_result;
}

CqlResult TokensCqlManager::create_token(const int& school_id,
                                         const std::string& token,
                                         const CassUuid& user_id) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_insert_token.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_string(statement.get(), 1, token.c_str());
    cass_statement_bind_uuid(statement.get(), 2, user_id);
    cass_statement_bind_int32(statement.get(), 3, _time_to_live);

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

std::pair<CqlResult, CassUuid> TokensCqlManager::get_user_from_token(
    const int& school_id, const std::string& token) {
  std::vector<CassUuid> users_id;
  // Define the result handler that pre-allocates the memory for the users_id
  // vector
  auto reserve_users_id = [&users_id](size_t num_rows) {
    // Reserve capacity for all returned users_id
    users_id.reserve(num_rows);
  };

  // Define the row handler that will store the users_id
  auto copy_user_id_details = [&users_id](const CassRow* row) {
    users_id.emplace_back();
    CassUuid& user_id = users_id.back();

    CqlResult cql_result = get_uuid_value(row, 0, user_id);
    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
                << cql_result.error();
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_get_user_from_token.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);
  cass_statement_bind_string(statement.get(), 1, token.c_str());

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_users_id, copy_user_id_details);

  if (cql_result.code() != ResultCode::OK) {
    CassUuid temp_uuid;
    return {cql_result, temp_uuid};
  }

  if (users_id.size() != 1) {
    CassUuid temp_uuid;
    return {CqlResult(ResultCode::NOT_FOUND), temp_uuid};
  }

  return {cql_result, users_id.at(0)};
}

CqlResult TokensCqlManager::delete_token(const int& school_id,
                                         const std::string& token) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_delete_token.get()));
    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_string(statement.get(), 1, token.c_str());

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