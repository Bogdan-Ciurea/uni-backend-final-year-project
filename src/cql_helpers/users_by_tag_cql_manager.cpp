/**
 * @file users_by_tag_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the relationship a tag and multiple users
 * @version 0.1
 * @date 2023-01-07
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "cql_helpers/users_by_tag_cql_manager.hpp"

CqlResult UsersByTagCqlManager::configure(bool init_db_schema) {
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

CqlResult UsersByTagCqlManager::init_prepare_statements() {
  CqlResult cql_result = _cql_client->prepare_statement(
      _prepared_insert_relationship, INSERT_RELATIONSHIP);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise insert relationship prepared statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_get_users_by_tag,
                                              GET_USERS_BY_TAG);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select users by tag prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_delete_relationship,
                                              DELETE_RELATIONSHIP);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise delete relationship prepared statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(
      _prepared_delete_relationships_by_tag, DELETE_RELATIONSHIPS_BY_TAG);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise delete all relationship of a tag "
                 "prepared statement: "
              << cql_result.str_code() << cql_result.error();
  }

  return cql_result;
}

CqlResult UsersByTagCqlManager::init_schema() {
  CqlResult cql_result = _cql_client->execute_statement(CREATE_SCHOOL_KEYSPACE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise school keyspace: "
              << cql_result.str_code() << cql_result.error();
    return cql_result;
  }

  cql_result = _cql_client->execute_statement(CREATE_USERS_BY_TAG_TABLE);

  return cql_result;
}

CqlResult UsersByTagCqlManager::create_relationship(const int school_id,
                                                    const CassUuid& tag_id,
                                                    const CassUuid& user_id) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_insert_relationship.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, tag_id);
    cass_statement_bind_uuid(statement.get(), 2, user_id);

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

std::pair<CqlResult, std::vector<CassUuid>>
UsersByTagCqlManager::get_users_by_tag(const int school_id,
                                       const CassUuid& tag_id) {
  std::vector<CassUuid> users_id;
  // Define the result handler that pre-allocates the memory for the
  // users_id vector
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
      cass_prepared_bind(_prepared_get_users_by_tag.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);
  cass_statement_bind_uuid(statement.get(), 1, tag_id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_users_id, copy_user_id_details);

  if (cql_result.code() != ResultCode::OK) {
    return {cql_result, std::vector<CassUuid>()};
  }

  return {cql_result, users_id};
}

CqlResult UsersByTagCqlManager::delete_relationship(const int school_id,
                                                    const CassUuid& tag_id,
                                                    const CassUuid& user_id) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_delete_relationship.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, tag_id);
    cass_statement_bind_uuid(statement.get(), 2, user_id);

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

CqlResult UsersByTagCqlManager::delete_relationships_by_tag(
    const int school_id, const CassUuid& tag_id) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_delete_relationships_by_tag.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, tag_id);

    auto [cql_result, cass_result] =
        _cql_client->execute_statement(statement.get());

    return cql_result;
  } catch (std::exception& ex) {
    error_message = ex.what();
  } catch (...) {
    error_message = "unknown exception";
  }

  return {ResultCode::UNKNOWN_ERROR, error_message};
}