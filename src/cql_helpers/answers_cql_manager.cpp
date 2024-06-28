/**
 * @file answers_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the answer object
 * @version 0.1
 * @date 2022-12-31
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "cql_helpers/answers_cql_manager.hpp"

CqlResult AnswersCqlManager::configure(bool init_db_schema) {
  CqlResult cql_result = ResultCode::OK;

  if (init_db_schema) {
    cql_result = init_schema();

    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << "Failed to initialise answers table: "
                << cql_result.str_code() << cql_result.error();
      return cql_result;
    }
  }

  cql_result = init_prepare_statements();

  return cql_result;
}

CqlResult AnswersCqlManager::init_prepare_statements() {
  CqlResult cql_result =
      _cql_client->prepare_statement(_prepared_insert_answer, INSERT_ANSWER);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise insert answer prepared statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_get_answer, SELECT_ANSWER);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select answer prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_update_answer, UPDATE_ANSWER);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise update answer prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_delete_answer, DELETE_ANSWER);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise delete answer prepared statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  return cql_result;
}

CqlResult AnswersCqlManager::init_schema() {
  CqlResult cql_result = _cql_client->execute_statement(CREATE_SCHOOL_KEYSPACE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise school keyspace: "
              << cql_result.str_code() << cql_result.error();
    return cql_result;
  }

  cql_result = _cql_client->execute_statement(CREATE_ANSWERS_TABLE);

  return cql_result;
}

CqlResult AnswersCqlManager::create_answer(const AnswerObject& answer_data) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_insert_answer.get()));

    cass_statement_bind_int32(statement.get(), 0, answer_data._school_id);
    cass_statement_bind_uuid(statement.get(), 1, answer_data._id);
    cass_statement_bind_int64(statement.get(), 2,
                              answer_data._created_at * 1000);
    cass_statement_bind_uuid(statement.get(), 3, answer_data._created_by);
    cass_statement_bind_string(statement.get(), 4,
                               answer_data._content.c_str());

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

std::pair<CqlResult, AnswerObject> AnswersCqlManager::get_answer_by_id(
    const int school_id, const CassUuid& id) {
  std::vector<AnswerObject> answers;
  // Define the result handler that pre-allocates the memory for the answers
  // vector
  auto reserve_answers = [&answers](size_t num_rows) {
    // Reserve capacity for all returned answers
    answers.reserve(num_rows);
  };

  // Define the row handler that will store the answers
  auto copy_answer_details = [&answers](const CassRow* row) {
    answers.emplace_back();
    AnswerObject& answer = answers.back();

    CqlResult cql_result = map_row_to_answer(row, answer);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(cass_prepared_bind(_prepared_get_answer.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);
  cass_statement_bind_uuid(statement.get(), 1, id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_answers, copy_answer_details);

  if (cql_result.code() != ResultCode::OK) {
    return {cql_result, AnswerObject()};
  }

  if (answers.size() != 1) {
    return {{ResultCode::NOT_FOUND, "No entries found"}, AnswerObject()};
  }

  return {cql_result, answers.at(0)};
}

CqlResult AnswersCqlManager::update_answer(const int school_id,
                                           const CassUuid& id,
                                           const time_t& created_at,
                                           const CassUuid& created_by,
                                           const std::string& content) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_update_answer.get()));

    cass_statement_bind_int32(statement.get(), 2, school_id);
    cass_statement_bind_uuid(statement.get(), 3, id);
    cass_statement_bind_int64(statement.get(), 4, created_at);
    cass_statement_bind_uuid(statement.get(), 0, created_by);
    cass_statement_bind_string(statement.get(), 1, content.c_str());

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

CqlResult AnswersCqlManager::delete_answer(const int school_id,
                                           const CassUuid& id,
                                           const time_t& created_at) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_delete_answer.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, id);
    cass_statement_bind_int64(statement.get(), 2, created_at * 1000);

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

CqlResult map_row_to_answer(const CassRow* row, AnswerObject& answer) {
  CqlResult cql_result = get_int_value(row, 0, answer._school_id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_uuid_value(row, 1, answer._id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_long_value(row, 2, answer._created_at);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }
  answer._created_at /= 1000;

  cql_result = get_uuid_value(row, 3, answer._created_by);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_text_value(row, 4, answer._content);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  return CqlResult(ResultCode::OK);
}
