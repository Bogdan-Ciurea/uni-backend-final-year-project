/**
 * @lecture lectures_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This lecture is responsible for communication between the backend and
 * database and manages the lecture object
 * @version 0.1
 * @date 2023-01-06
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "cql_helpers/lectures_cql_manager.hpp"

CqlResult LecturesCqlManager::configure(bool init_db_schema) {
  CqlResult cql_result = ResultCode::OK;

  if (init_db_schema) {
    cql_result = init_schema();

    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << "Failed to initialise lectures table: "
                << cql_result.str_code() << cql_result.error();
      return cql_result;
    }
  }

  cql_result = init_prepare_statements();

  return cql_result;
}

CqlResult LecturesCqlManager::init_prepare_statements() {
  CqlResult cql_result =
      _cql_client->prepare_statement(_prepared_insert_lecture, INSERT_LECTURE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise insert lecture statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_get_lectures_by_course,
                                              SELECT_LECTURES_BY_COURSE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select lectures by course prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_delete_lecture, DELETE_LECTURE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise delete lecture prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(
      _prepared_delete_lectures_by_course, DELETE_LECTURES_BY_COURSE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise delete lecture prepared statement: "
              << cql_result.str_code() << cql_result.error();
  }

  return cql_result;
}

CqlResult LecturesCqlManager::init_schema() {
  CqlResult cql_result = _cql_client->execute_statement(CREATE_SCHOOL_KEYSPACE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise school keyspace: "
              << cql_result.str_code() << cql_result.error();
    return cql_result;
  }

  cql_result = _cql_client->execute_statement(CREATE_LECTURES_TABLE);

  return cql_result;
}

CqlResult LecturesCqlManager::create_lecture(
    const LectureObject& lecture_data) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_insert_lecture.get()));

    cass_statement_bind_int32(statement.get(), 0, lecture_data._school_id);
    cass_statement_bind_uuid(statement.get(), 1, lecture_data._course_id);
    cass_statement_bind_int64(statement.get(), 2,
                              lecture_data._starting_time * 1000);
    cass_statement_bind_int32(statement.get(), 3, lecture_data._duration);
    cass_statement_bind_string(statement.get(), 4,
                               lecture_data._location.c_str());

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

std::pair<CqlResult, std::vector<LectureObject>>
LecturesCqlManager::get_lectures_by_course(const int& school_id,
                                           const CassUuid& course_id) {
  std::vector<LectureObject> lectures;
  // Define the result handler that pre-allocates the memory for the lectures
  // vector
  auto reserve_lectures = [&lectures](size_t num_rows) {
    // Reserve capacity for all returned lectures
    lectures.reserve(num_rows);
  };

  // Define the row handler that will store the lectures
  auto copy_lecture_details = [&lectures](const CassRow* row) {
    lectures.emplace_back();
    LectureObject& lecture = lectures.back();

    CqlResult cql_result = map_row_to_lecture(row, lecture);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_get_lectures_by_course.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);
  cass_statement_bind_uuid(statement.get(), 1, course_id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_lectures, copy_lecture_details);

  if (cql_result.code() != ResultCode::OK) {
    return {cql_result, std::vector<LectureObject>()};
  }

  return {cql_result, lectures};
}

CqlResult LecturesCqlManager::update_lecture(
    const int& school_id, const CassUuid& course_id,
    const time_t& original_starting_time, const time_t& new_starting_time,
    const int& duration, const std::string& location) {
  LectureObject lecture_data(school_id, course_id, new_starting_time, duration,
                             location);

  CqlResult cql_result =
      this->delete_lecture(school_id, course_id, original_starting_time);
  if (cql_result.code() != ResultCode::OK) {
    return cql_result;
  }

  return this->create_lecture(lecture_data);
}

CqlResult LecturesCqlManager::delete_lecture(const int& school_id,
                                             const CassUuid& course_id,
                                             const time_t& starting_time) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_delete_lecture.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, course_id);
    cass_statement_bind_int64(statement.get(), 2, starting_time * 1000);

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

CqlResult LecturesCqlManager::delete_lectures_by_course(
    const int& school_id, const CassUuid& course_id) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_delete_lectures_by_course.get()));

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

CqlResult map_row_to_lecture(const CassRow* row, LectureObject& lecture) {
  CqlResult cql_result = get_int_value(row, 0, lecture._school_id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_uuid_value(row, 1, lecture._course_id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_long_value(row, 2, lecture._starting_time);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }
  lecture._starting_time /= 1000;

  cql_result = get_int_value(row, 3, lecture._duration);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_text_value(row, 4, lecture._location);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  return CqlResult(ResultCode::OK);
}