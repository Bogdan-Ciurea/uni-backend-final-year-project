/**
 * @file todos_by_user_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the relationship between todos and users
 * @version 0.1
 * @date 2023-01-07
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "cql_helpers/todos_by_user_cql_manager.hpp"

CqlResult TodosByUserCqlManager::configure(bool init_db_schema) {
  CqlResult cql_result = ResultCode::OK;

  if (init_db_schema) {
    cql_result = init_schema();

    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << "Failed to initialise todos by user table: "
                << cql_result.str_code() << cql_result.error();
      return cql_result;
    }
  }

  cql_result = init_prepare_statements();

  return cql_result;
}

CqlResult TodosByUserCqlManager::init_prepare_statements() {
  CqlResult cql_result = _cql_client->prepare_statement(
      _prepared_insert_relationship, INSERT_RELATIONSHIP);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise insert todo by user statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_get_todos_by_user, SELECT_TODOS);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select todos by user prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_delete_relationship,
                                              DELETE_RELATIONSHIP);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise delete todo by user prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(
      _prepared_delete_relationships_by_user, DELETE_RELATIONSHIPS_BY_USER);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise delete todo by user prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();
  }

  return cql_result;
}

CqlResult TodosByUserCqlManager::init_schema() {
  CqlResult cql_result = _cql_client->execute_statement(CREATE_SCHOOL_KEYSPACE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise school keyspace: "
              << cql_result.str_code() << cql_result.error();
    return cql_result;
  }

  cql_result = _cql_client->execute_statement(CREATE_TODOS_BY_USER_TABLE);

  return cql_result;
}

CqlResult TodosByUserCqlManager::create_relationship(const int school_id,
                                                     const CassUuid& user_id,
                                                     const CassUuid& todo_id) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_insert_relationship.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, user_id);
    cass_statement_bind_uuid(statement.get(), 2, todo_id);

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
TodosByUserCqlManager::get_todos_by_user(const int school_id,
                                         const CassUuid& user_id) {
  std::vector<CassUuid> todos_id;
  // Define the result handler that pre-allocates the memory for the
  // todos_id vector
  auto reserve_todos_id = [&todos_id](size_t num_rows) {
    // Reserve capacity for all returned todos_id
    todos_id.reserve(num_rows);
  };

  // Define the row handler that will store the todos_id
  auto copy_todo_id_details = [&todos_id](const CassRow* row) {
    todos_id.emplace_back();
    CassUuid& todo_id = todos_id.back();

    CqlResult cql_result = get_uuid_value(row, 0, todo_id);
    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
                << cql_result.error();
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_get_todos_by_user.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);
  cass_statement_bind_uuid(statement.get(), 1, user_id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_todos_id, copy_todo_id_details);

  if (cql_result.code() != ResultCode::OK) {
    return {cql_result, std::vector<CassUuid>()};
  }

  return {cql_result, todos_id};
}

CqlResult TodosByUserCqlManager::delete_relationship(const int school_id,
                                                     const CassUuid& user_id,
                                                     const CassUuid& todo_id) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_delete_relationship.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, user_id);
    cass_statement_bind_uuid(statement.get(), 2, todo_id);

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

CqlResult TodosByUserCqlManager::delete_relationships_by_user(
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
