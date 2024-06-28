/**
 * @file questions_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the question object
 * @version 0.1
 * @date 2023-01-07
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "cql_helpers/questions_cql_manager.hpp"

CqlResult QuestionsCqlManager::configure(bool init_db_schema) {
  CqlResult cql_result = ResultCode::OK;

  if (init_db_schema) {
    cql_result = init_schema();

    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << "Failed to initialise questions table: "
                << cql_result.str_code() << cql_result.error();
      return cql_result;
    }
  }

  cql_result = init_prepare_statements();

  return cql_result;
}

CqlResult QuestionsCqlManager::init_prepare_statements() {
  CqlResult cql_result = _cql_client->prepare_statement(
      _prepared_insert_question, INSERT_QUESTION);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise insert question statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_get_question, SELECT_QUESTION);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select questions by question prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_update_question,
                                              UPDATE_QUESTION);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select questions by question prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_delete_question,
                                              DELETE_QUESTION);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise delete question prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();
  }

  return cql_result;
}

CqlResult QuestionsCqlManager::init_schema() {
  CqlResult cql_result = _cql_client->execute_statement(CREATE_SCHOOL_KEYSPACE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise school keyspace: "
              << cql_result.str_code() << cql_result.error();
    return cql_result;
  }

  cql_result = _cql_client->execute_statement(CREATE_QUESTIONS_TABLE);

  return cql_result;
}

CqlResult QuestionsCqlManager::create_question(
    const QuestionObject& question_data) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_insert_question.get()));

    cass_statement_bind_int32(statement.get(), 0, question_data._school_id);
    cass_statement_bind_uuid(statement.get(), 1, question_data._question_id);
    cass_statement_bind_string(statement.get(), 2, question_data._text.c_str());
    cass_statement_bind_int64(statement.get(), 3,
                              question_data._time_added * 1000);
    cass_statement_bind_uuid(statement.get(), 4,
                             question_data._added_by_user_id);

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

std::pair<CqlResult, QuestionObject> QuestionsCqlManager::get_question_by_id(
    const int& school_id, const CassUuid& question_id) {
  std::vector<QuestionObject> questions;
  // Define the result handler that pre-allocates the memory for the questions
  // vector
  auto reserve_questions = [&questions](size_t num_rows) {
    // Reserve capacity for all returned questions
    questions.reserve(num_rows);
  };

  // Define the row handler that will store the questions
  auto copy_question_details = [&questions](const CassRow* row) {
    questions.emplace_back();
    QuestionObject& question = questions.back();

    CqlResult cql_result = map_row_to_question(row, question);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(cass_prepared_bind(_prepared_get_question.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);
  cass_statement_bind_uuid(statement.get(), 1, question_id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_questions, copy_question_details);

  if (cql_result.code() != ResultCode::OK) {
    return {cql_result, QuestionObject()};
  }

  if (questions.size() != 1) {
    return {CqlResult(ResultCode::NOT_FOUND), QuestionObject()};
  }

  return {cql_result, questions.at(0)};
}

CqlResult QuestionsCqlManager::update_question(
    const int& school_id, const CassUuid& question_id, const std::string& text,
    const time_t& time_changed, const CassUuid& changed_by_user) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_update_question.get()));

    cass_statement_bind_int32(statement.get(), 3, school_id);
    cass_statement_bind_uuid(statement.get(), 4, question_id);
    cass_statement_bind_string(statement.get(), 0, text.c_str());
    cass_statement_bind_int64(statement.get(), 1, time_changed * 1000);
    cass_statement_bind_uuid(statement.get(), 2, changed_by_user);

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

CqlResult QuestionsCqlManager::delete_question(const int& school_id,
                                               const CassUuid& question_id) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_delete_question.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, question_id);

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

CqlResult map_row_to_question(const CassRow* row, QuestionObject& question) {
  CqlResult cql_result = get_int_value(row, 0, question._school_id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_uuid_value(row, 1, question._question_id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_text_value(row, 2, question._text);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_long_value(row, 3, question._time_added);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }
  question._time_added /= 1000;

  cql_result = get_uuid_value(row, 4, question._added_by_user_id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  return CqlResult(ResultCode::OK);
}
