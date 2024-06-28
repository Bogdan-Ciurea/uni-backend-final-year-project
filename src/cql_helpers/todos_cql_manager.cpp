/**
 * @todo todos_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This todo is responsible for communication between the backend and
 * database and manages the todos object
 * @version 0.1
 * @date 2023-01-07
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "cql_helpers/todos_cql_manager.hpp"

CqlResult TodosCqlManager::configure(bool init_db_schema) {
  CqlResult cql_result = ResultCode::OK;

  if (init_db_schema) {
    cql_result = init_schema();

    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << "Failed to initialise todos table: " << cql_result.str_code()
                << cql_result.error();
      return cql_result;
    }
  }

  cql_result = init_prepare_statements();

  return cql_result;
}

CqlResult TodosCqlManager::init_prepare_statements() {
  CqlResult cql_result =
      _cql_client->prepare_statement(_prepared_insert_todo, INSERT_TODO);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise insert todo statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_get_todo_by_id,
                                              SELECT_TODO_BY_ID);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select todos prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_update_todo, UPDATE_TODO);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise update todos prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_delete_todo, DELETE_TODO);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise delete todo prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  return cql_result;
}

CqlResult TodosCqlManager::init_schema() {
  CqlResult cql_result = _cql_client->execute_statement(CREATE_SCHOOL_KEYSPACE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise school keyspace: "
              << cql_result.str_code() << cql_result.error();
    return cql_result;
  }

  cql_result = _cql_client->execute_statement(CREATE_TODOS_TABLE);

  return cql_result;
}

CqlResult TodosCqlManager::create_todo(const TodoObject& todo_data) {
  std::string error_message;

  try {
    CassStatementPtr statement(cass_prepared_bind(_prepared_insert_todo.get()));

    cass_statement_bind_int32(statement.get(), 0, todo_data._school_id);
    cass_statement_bind_uuid(statement.get(), 1, todo_data._todo_id);
    cass_statement_bind_string(statement.get(), 2, todo_data._text.c_str());
    cass_statement_bind_int32(statement.get(), 3,
                              static_cast<int>(todo_data._type));

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

std::pair<CqlResult, TodoObject> TodosCqlManager::get_todo_by_id(
    const int school_id, const CassUuid& todo_id) {
  std::vector<TodoObject> todos;
  // Define the result handler that pre-allocates the memory for the todos
  // vector
  auto reserve_todos = [&todos](size_t num_rows) {
    // Reserve capacity for all returned todos
    todos.reserve(num_rows);
  };

  // Define the row handler that will store the todos
  auto copy_todo_details = [&todos](const CassRow* row) {
    todos.emplace_back();
    TodoObject& todo = todos.back();

    CqlResult cql_result = map_row_to_todo(row, todo);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_get_todo_by_id.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);
  cass_statement_bind_uuid(statement.get(), 1, todo_id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_todos, copy_todo_details);

  if (cql_result.code() != ResultCode::OK) {
    return {cql_result, TodoObject()};
  }

  if (todos.size() != 1) {
    return {{ResultCode::NOT_FOUND, "No entries found"}, TodoObject()};
  }

  return {cql_result, todos.at(0)};
}

CqlResult TodosCqlManager::update_todo(const int school_id,
                                       const CassUuid& todo_id,
                                       const std::string& text,
                                       const TodoType type) {
  std::string error_message;

  try {
    CassStatementPtr statement(cass_prepared_bind(_prepared_update_todo.get()));

    cass_statement_bind_int32(statement.get(), 2, school_id);
    cass_statement_bind_uuid(statement.get(), 3, todo_id);
    cass_statement_bind_string(statement.get(), 0, text.c_str());
    cass_statement_bind_int32(statement.get(), 1, static_cast<int>(type));

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

CqlResult TodosCqlManager::delete_todo(const int school_id,
                                       const CassUuid& todo_id) {
  std::string error_message;

  try {
    CassStatementPtr statement(cass_prepared_bind(_prepared_delete_todo.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, todo_id);

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

CqlResult map_row_to_todo(const CassRow* row, TodoObject& todo) {
  CqlResult cql_result = get_int_value(row, 0, todo._school_id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_uuid_value(row, 1, todo._todo_id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_text_value(row, 2, todo._text);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  int type;
  cql_result = get_int_value(row, 3, type);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }
  if (type < 0 || type > 2) {
    return {ResultCode::UNKNOWN_ERROR, "Invalid todo type"};
  }

  if (type == 0) {
    todo._type = TodoType::NOT_STARTED;
  } else if (type == 1) {
    todo._type = TodoType::IN_PROGRESS;
  } else if (type == 2) {
    todo._type = TodoType::DONE;
  }

  return CqlResult(ResultCode::OK);
}
