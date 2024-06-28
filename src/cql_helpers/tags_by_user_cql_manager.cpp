/**
 * @file tags_by_user_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the relationship between tags and users
 * @version 0.1
 * @date 2023-01-04
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "cql_helpers/tags_by_user_cql_manager.hpp"

CqlResult TagsByUserCqlManager::configure(bool init_db_schema) {
  CqlResult cql_result = ResultCode::OK;

  if (init_db_schema) {
    cql_result = init_schema();

    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << "Failed to initialise tags_by_user table: "
                << cql_result.str_code() << cql_result.error();
      return cql_result;
    }
  }

  cql_result = init_prepare_statements();

  return cql_result;
}

CqlResult TagsByUserCqlManager::init_prepare_statements() {
  CqlResult cql_result = _cql_client->prepare_statement(
      _prepared_insert_relationship, INSERT_RELATIONSHIP);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise insert relationship prepared statement: "
              << cql_result.str_code() << cql_result.error();
    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_get_tags_by_user, SELECT_TAGS);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select tags by user prepared "
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
      _prepared_delete_relationships_by_user, DELETE_RELATIONSHIPS_BY_USER);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise delete relationships by user prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();
  }

  return cql_result;
}

CqlResult TagsByUserCqlManager::init_schema() {
  CqlResult cql_result = _cql_client->execute_statement(CREATE_SCHOOL_KEYSPACE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise school keyspace: "
              << cql_result.str_code() << cql_result.error();
    return cql_result;
  }

  cql_result = _cql_client->execute_statement(CREATE_TAGS_BY_USER_TABLE);

  return cql_result;
}

CqlResult TagsByUserCqlManager::create_relationship(const int school_id,
                                                    const CassUuid& user_id,
                                                    const CassUuid& tag_id) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_insert_relationship.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, user_id);
    cass_statement_bind_uuid(statement.get(), 2, tag_id);

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
TagsByUserCqlManager::get_tags_by_user(const int school_id,
                                       const CassUuid& user_id) {
  std::vector<CassUuid> tags_id;
  // Define the result handler that pre-allocates the memory for the
  // tags_id vector
  auto reserve_tags_id = [&tags_id](size_t num_rows) {
    // Reserve capacity for all returned tags_id
    tags_id.reserve(num_rows);
  };

  // Define the row handler that will store the tags_id
  auto copy_tag_id_details = [&tags_id](const CassRow* row) {
    tags_id.emplace_back();
    CassUuid& tag_id = tags_id.back();

    CqlResult cql_result = get_uuid_value(row, 0, tag_id);
    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
                << cql_result.error();
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_get_tags_by_user.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);
  cass_statement_bind_uuid(statement.get(), 1, user_id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_tags_id, copy_tag_id_details);

  if (cql_result.code() != ResultCode::OK) {
    return {cql_result, std::vector<CassUuid>()};
  }

  return {cql_result, tags_id};
}

CqlResult TagsByUserCqlManager::delete_relationship(const int school_id,
                                                    const CassUuid& user_id,
                                                    const CassUuid& tag_id) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_delete_relationship.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, user_id);
    cass_statement_bind_uuid(statement.get(), 2, tag_id);

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

CqlResult TagsByUserCqlManager::delete_relationships_by_user(
    const int school_id, const CassUuid& user_id) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_delete_relationships_by_user.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, user_id);

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
