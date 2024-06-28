/**
 * @file answers_by_announcement_or_question_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the relationship between answers and announcements or
 * questions
 * @version 0.1
 * @date 2023-01-02
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "cql_helpers/answers_by_announcement_or_question_cql_manager.hpp"

CqlResult AnswersByAnnouncementOrQuestionCqlManager::configure(
    bool init_db_schema) {
  CqlResult cql_result = ResultCode::OK;

  if (init_db_schema) {
    cql_result = init_schema();

    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << "Failed to initialise answers_id table: "
                << cql_result.str_code() << cql_result.error();
      return cql_result;
    }
  }

  cql_result = init_prepare_statements();

  return cql_result;
}

CqlResult AnswersByAnnouncementOrQuestionCqlManager::init_prepare_statements() {
  CqlResult cql_result = _cql_client->prepare_statement(
      _prepared_insert_relationship, INSERT_RELATIONSHIP);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise insert relationship prepared statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(
      _prepared_get_answers_by_announcement_or_question, SELECT_ANSWERS);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select announcements by tag prepared "
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
      _prepared_delete_relationships_by_announcement_or_question,
      DELETE_RELATIONSHIPS_BY_ANNOUNCEMENT_OR_QUESTION);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise delete relationship prepared statement: "
              << cql_result.str_code() << cql_result.error();
  }

  return cql_result;
}

CqlResult AnswersByAnnouncementOrQuestionCqlManager::init_schema() {
  CqlResult cql_result = _cql_client->execute_statement(CREATE_SCHOOL_KEYSPACE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise school keyspace: "
              << cql_result.str_code() << cql_result.error();
    return cql_result;
  }

  cql_result = _cql_client->execute_statement(
      CREATE_ANSWERS_BY_ANNOUNCEMENT_OR_QUESTION_TABLE);

  return cql_result;
}

CqlResult AnswersByAnnouncementOrQuestionCqlManager::create_relationship(
    const int school_id, const CassUuid& announcement_or_question_id,
    const int type, const CassUuid& answer_id) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_insert_relationship.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, announcement_or_question_id);
    cass_statement_bind_int32(statement.get(), 2, type);
    cass_statement_bind_uuid(statement.get(), 3, answer_id);

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
AnswersByAnnouncementOrQuestionCqlManager::
    get_answers_by_announcement_or_question(
        const int school_id, const CassUuid& announcement_or_question_id,
        const int type) {
  std::vector<CassUuid> answers_id;
  // Define the result handler that pre-allocates the memory for the
  // answers_id vector
  auto reserve_answers_id = [&answers_id](size_t num_rows) {
    // Reserve capacity for all returned answers_id
    answers_id.reserve(num_rows);
  };

  // Define the row handler that will store the answers_id
  auto copy_answer_id_details = [&answers_id](const CassRow* row) {
    answers_id.emplace_back();
    CassUuid& answer_id = answers_id.back();

    CqlResult cql_result = get_uuid_value(row, 0, answer_id);
    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
                << cql_result.error();
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(cass_prepared_bind(
      _prepared_get_answers_by_announcement_or_question.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);
  cass_statement_bind_uuid(statement.get(), 1, announcement_or_question_id);
  cass_statement_bind_int32(statement.get(), 2, type);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_answers_id, copy_answer_id_details);

  if (cql_result.code() != ResultCode::OK) {
    return {cql_result, std::vector<CassUuid>()};
  }

  return {cql_result, answers_id};
}

CqlResult AnswersByAnnouncementOrQuestionCqlManager::delete_relationship(
    const int school_id, const CassUuid& announcement_or_question_id,
    const int type, const CassUuid& answer_id) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_delete_relationship.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, announcement_or_question_id);
    cass_statement_bind_int32(statement.get(), 2, type);
    cass_statement_bind_uuid(statement.get(), 3, answer_id);

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

CqlResult AnswersByAnnouncementOrQuestionCqlManager::
    delete_relationships_by_announcement_or_question(
        const int school_id, const CassUuid& announcement_or_question_id,
        const int type) {
  std::string error_message;

  try {
    CassStatementPtr statement(cass_prepared_bind(
        _prepared_delete_relationships_by_announcement_or_question.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, announcement_or_question_id);
    cass_statement_bind_int32(statement.get(), 2, type);

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
