/**
 * @file questions_by_course_cql_manager.CPP
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the relationship between questions and courses
 * @version 0.1
 * @date 2023-01-07
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "cql_helpers/questions_by_course_cql_manager.hpp"

CqlResult QuestionsByCourseCqlManager::configure(bool init_db_schema) {
  CqlResult cql_result = ResultCode::OK;

  if (init_db_schema) {
    cql_result = init_schema();

    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << "Failed to initialise questions_by_course table: "
                << cql_result.str_code() << cql_result.error();
      return cql_result;
    }
  }

  cql_result = init_prepare_statements();

  return cql_result;
}

CqlResult QuestionsByCourseCqlManager::init_prepare_statements() {
  CqlResult cql_result = _cql_client->prepare_statement(
      _prepared_insert_relationship, INSERT_RELATIONSHIP);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise insert relationship prepared statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_get_questions_by_course,
                                              SELECT_QUESTIONS_BY_COURSE);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select questions by course prepared "
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
      _prepared_delete_relationships_by_course, DELETE_RELATIONSHIPS_BY_COURSE);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise delete relationships by courses "
                 "prepared statement: "
              << cql_result.str_code() << cql_result.error();
  }

  return cql_result;
}

CqlResult QuestionsByCourseCqlManager::init_schema() {
  CqlResult cql_result = _cql_client->execute_statement(CREATE_SCHOOL_KEYSPACE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise school keyspace: "
              << cql_result.str_code() << cql_result.error();
    return cql_result;
  }

  cql_result = _cql_client->execute_statement(CREATE_QUESTIONS_BY_COURSE_TABLE);

  return cql_result;
}

CqlResult QuestionsByCourseCqlManager::create_relationship(
    const int school_id, const CassUuid& course_id,
    const CassUuid& question_id) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_insert_relationship.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, course_id);
    cass_statement_bind_uuid(statement.get(), 2, question_id);

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
QuestionsByCourseCqlManager::get_questions_by_course(
    const int school_id, const CassUuid& course_id) {
  std::vector<CassUuid> questions_id;
  // Define the result handler that pre-allocates the memory for the
  // questions_id vector
  auto reserve_questions_id = [&questions_id](size_t num_rows) {
    // Reserve capacity for all returned questions_id
    questions_id.reserve(num_rows);
  };

  // Define the row handler that will store the questions_id
  auto copy_question_id_details = [&questions_id](const CassRow* row) {
    questions_id.emplace_back();
    CassUuid& question_id = questions_id.back();

    CqlResult cql_result = get_uuid_value(row, 0, question_id);
    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
                << cql_result.error();
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_get_questions_by_course.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);
  cass_statement_bind_uuid(statement.get(), 1, course_id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_questions_id, copy_question_id_details);

  if (cql_result.code() != ResultCode::OK) {
    return {cql_result, std::vector<CassUuid>()};
  }

  return {cql_result, questions_id};
}

CqlResult QuestionsByCourseCqlManager::delete_relationship(
    const int school_id, const CassUuid& course_id,
    const CassUuid& question_id) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_delete_relationship.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, course_id);
    cass_statement_bind_uuid(statement.get(), 2, question_id);

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

CqlResult QuestionsByCourseCqlManager::delete_relationships_by_course(
    const int school_id, const CassUuid& course_id) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_delete_relationships_by_course.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, course_id);

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
