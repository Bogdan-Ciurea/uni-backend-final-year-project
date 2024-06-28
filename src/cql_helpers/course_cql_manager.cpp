/**
 * @file courses_cql_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the course object
 * @version 0.1
 * @date 2023-01-02
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "cql_helpers/courses_cql_manager.hpp"

CqlResult CoursesCqlManager::configure(bool init_db_schema) {
  CqlResult cql_result = ResultCode::OK;

  if (init_db_schema) {
    cql_result = init_schema();

    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << "Failed to initialise courses table: "
                << cql_result.str_code() << cql_result.error();
      return cql_result;
    }
  }

  cql_result = init_prepare_statements();

  return cql_result;
}

CqlResult CoursesCqlManager::init_prepare_statements() {
  CqlResult cql_result =
      _cql_client->prepare_statement(_prepared_insert_course, INSERT_COURSE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise insert course prepared statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_get_course, SELECT_COURSE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select course prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_get_courses_by_school,
                                              SELECT_COURSES_BY_SCHOOL);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select courses by school prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_update_course, UPDATE_COURSE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise update course prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_delete_course, DELETE_COURSE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise delete course prepared statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  return cql_result;
}

CqlResult CoursesCqlManager::init_schema() {
  CqlResult cql_result = _cql_client->execute_statement(CREATE_SCHOOL_KEYSPACE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise school keyspace: "
              << cql_result.str_code() << cql_result.error();
    return cql_result;
  }

  cql_result = _cql_client->execute_statement(CREATE_COURSES_TABLE);

  return cql_result;
}

CqlResult CoursesCqlManager::create_course(const CourseObject& course_data) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_insert_course.get()));

    cass_statement_bind_int32(statement.get(), 0, course_data._school_id);
    cass_statement_bind_uuid(statement.get(), 1, course_data._id);
    cass_statement_bind_string(statement.get(), 2, course_data._name.c_str());
    cass_statement_bind_string(statement.get(), 3,
                               course_data._course_thumbnail.c_str());
    cass_statement_bind_int64(statement.get(), 4,
                              course_data._created_at * 1000);
    cass_statement_bind_int64(statement.get(), 5,
                              course_data._start_date * 1000);
    cass_statement_bind_int64(statement.get(), 6, course_data._end_date * 1000);

    // Generate the list of uuids
    CassCollectionPtr list(cass_collection_new(CASS_COLLECTION_TYPE_LIST,
                                               course_data._files.size()));

    // Append the uuids to the list. Add them back to front
    // because the cassandra driver adds them in reverse order
    for (auto& file : course_data._files) {
      if (CASS_OK != cass_collection_append_uuid(list.get(), file))
        LOG_DEBUG << "Error when adding file";
    }

    if (cass_statement_bind_collection(statement.get(), 7, list.get()) !=
        CASS_OK) {
      LOG_ERROR << "Failed to bind collection of files to course_data";
      return {ResultCode::UNKNOWN_ERROR,
              "Failed to bind collection of files to course_data"};
    }

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

std::pair<CqlResult, CourseObject> CoursesCqlManager::get_course_by_id(
    const int& school_id, const CassUuid& id) {
  std::vector<CourseObject> courses;
  // Define the result handler that pre-allocates the memory for the courses
  // vector
  auto reserve_courses = [&courses](size_t num_rows) {
    // Reserve capacity for all returned courses
    courses.reserve(num_rows);
  };

  // Define the row handler that will store the courses
  auto copy_course_details = [&courses](const CassRow* row) {
    courses.emplace_back();
    CourseObject& course = courses.back();

    CqlResult cql_result = map_row_to_course(row, course);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(cass_prepared_bind(_prepared_get_course.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);
  cass_statement_bind_uuid(statement.get(), 1, id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_courses, copy_course_details);

  if (cql_result.code() != ResultCode::OK) {
    return {cql_result, CourseObject()};
  }

  if (courses.size() != 1) {
    return {{ResultCode::NOT_FOUND, "No entries found"}, CourseObject()};
  }

  return {cql_result, courses.at(0)};
}

std::pair<CqlResult, std::vector<CourseObject>>
CoursesCqlManager::get_courses_by_school(const int& school_id) {
  std::vector<CourseObject> courses;
  // Define the result handler that pre-allocates the memory for the courses
  // vector
  auto reserve_courses = [&courses](size_t num_rows) {
    // Reserve capacity for all returned courses
    courses.reserve(num_rows);
  };

  // Define the row handler that will store the courses
  auto copy_course_details = [&courses](const CassRow* row) {
    courses.emplace_back();
    CourseObject& course = courses.back();

    CqlResult cql_result = map_row_to_course(row, course);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_get_courses_by_school.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_courses, copy_course_details);

  if (cql_result.code() != ResultCode::OK) {
    return {cql_result, {}};
  }

  return {cql_result, courses};
}

CqlResult CoursesCqlManager::update_course(
    const int& school_id, const CassUuid& id, const std::string& name,
    const std::string& thumbnail, const time_t& updated_at,
    const time_t& start_date, const time_t& end_date,
    const std::vector<CassUuid>& files) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_update_course.get()));

    cass_statement_bind_string(statement.get(), 0, name.c_str());
    cass_statement_bind_string(statement.get(), 1, thumbnail.c_str());
    cass_statement_bind_int64(statement.get(), 2, updated_at * 1000);
    cass_statement_bind_int64(statement.get(), 3, start_date * 1000);
    cass_statement_bind_int64(statement.get(), 4, end_date * 1000);

    // Generate the list of uuids
    CassCollectionPtr list(
        cass_collection_new(CASS_COLLECTION_TYPE_LIST, files.size()));

    // Append the uuids to the list. Add them back to front
    // because the cassandra driver adds them in reverse order
    for (auto& file : files) {
      cass_collection_append_uuid(list.get(), file);
    }

    if (cass_statement_bind_collection(statement.get(), 5, list.get()) !=
        CASS_OK) {
      LOG_ERROR << "Failed to bind collection of files to course_data";
      return {ResultCode::UNKNOWN_ERROR,
              "Failed to bind collection of files to course_data"};
    }

    cass_statement_bind_int32(statement.get(), 6, school_id);
    cass_statement_bind_uuid(statement.get(), 7, id);

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

CqlResult CoursesCqlManager::delete_course_by_id(const int& school_id,
                                                 const CassUuid& id) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_delete_course.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, id);

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

CqlResult map_row_to_course(const CassRow* row, CourseObject& course) {
  CqlResult cql_result = get_int_value(row, 0, course._school_id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_uuid_value(row, 1, course._id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_text_value(row, 2, course._name);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_text_value(row, 3, course._course_thumbnail);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    LOG_ERROR << "Failed to get course thumbnail. This will not stop the "
                 "information to be read.";
  }

  cql_result = get_long_value(row, 4, course._created_at);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }
  course._created_at /= 1000;

  cql_result = get_long_value(row, 5, course._start_date);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }
  course._start_date /= 1000;

  cql_result = get_long_value(row, 6, course._end_date);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }
  course._end_date /= 1000;

  cql_result = get_array_uuids_value(row, 7, course._files);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  return CqlResult(ResultCode::OK);
}
