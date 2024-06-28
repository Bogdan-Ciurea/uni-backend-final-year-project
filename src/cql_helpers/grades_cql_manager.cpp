/**
 * @grade grades_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This grade is responsible for communication between the backend and
 * database and manages the grade object
 * @version 0.1
 * @date 2023-01-03
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "cql_helpers/grades_cql_manager.hpp"

CqlResult GradesCqlManager::configure(bool init_db_schema) {
  CqlResult cql_result = ResultCode::OK;

  if (init_db_schema) {
    cql_result = init_schema();

    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << "Failed to initialise grades table: "
                << cql_result.str_code() << cql_result.error();
      return cql_result;
    }
  }

  cql_result = init_prepare_statements();

  return cql_result;
}

CqlResult GradesCqlManager::init_prepare_statements() {
  CqlResult cql_result =
      _cql_client->prepare_statement(_prepared_insert_grade, INSERT_GRADE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise insert grade statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_get_grade_by_id,
                                              SELECT_GRADE_BY_ID);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select grade prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_get_grades_by_evaluated,
                                              SELECT_GRADES_BY_STUDENT);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select grades by student prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_get_grades_by_evaluator,
                                              SELECT_GRADES_BY_EVALUATOR);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select grades by evaluator prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_get_grades_by_course,
                                              SELECT_GRADES_BY_COURSE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select grades by course prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(
      _prepared_get_grades_by_evaluated_and_course,
      SELECT_GRADES_BY_EVALUATED_AND_COURSE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select grades by evaluated and course "
                 "prepared statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_update_grade, UPDATE_GRADE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise update grade prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_delete_grade, DELETE_GRADE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise delete grade prepared statement: "
              << cql_result.str_code() << cql_result.error();
  }

  return cql_result;
}

CqlResult GradesCqlManager::init_schema() {
  CqlResult cql_result = _cql_client->execute_statement(CREATE_SCHOOL_KEYSPACE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise school keyspace: "
              << cql_result.str_code() << cql_result.error();
    return cql_result;
  }

  cql_result = _cql_client->execute_statement(CREATE_GRADES_TABLE);

  return cql_result;
}

CqlResult GradesCqlManager::create_grade(const GradeObject& grade_data) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_insert_grade.get()));

    cass_statement_bind_int32(statement.get(), 0, grade_data._school_id);
    cass_statement_bind_uuid(statement.get(), 1, grade_data._id);
    cass_statement_bind_uuid(statement.get(), 2, grade_data._evaluated_id);
    cass_statement_bind_uuid(statement.get(), 3, grade_data._evaluator_id);
    cass_statement_bind_uuid(statement.get(), 4, grade_data._course_id);
    cass_statement_bind_int32(statement.get(), 5, grade_data._grade);
    cass_statement_bind_int32(statement.get(), 6, grade_data._out_of);
    cass_statement_bind_int64(statement.get(), 7,
                              grade_data._created_at * 1000);
    cass_statement_bind_float(statement.get(), 8, grade_data._weight);

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

std::pair<CqlResult, GradeObject> GradesCqlManager::get_grade_by_id(
    const int& school_id, const CassUuid& id) {
  std::vector<GradeObject> grades;
  // Define the result handler that pre-allocates the memory for the grades
  // vector
  auto reserve_grades = [&grades](size_t num_rows) {
    // Reserve capacity for all returned grades
    grades.reserve(num_rows);
  };

  // Define the row handler that will store the grades
  auto copy_grade_details = [&grades](const CassRow* row) {
    grades.emplace_back();
    GradeObject& grade = grades.back();

    CqlResult cql_result = map_row_to_grade(row, grade);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_get_grade_by_id.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);
  cass_statement_bind_uuid(statement.get(), 1, id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_grades, copy_grade_details);

  if (cql_result.code() != ResultCode::OK) {
    return {cql_result, GradeObject()};
  }

  if (grades.size() != 1) {
    return {{ResultCode::NOT_FOUND, "No entries found"}, GradeObject()};
  }

  return {cql_result, grades.at(0)};
}

std::pair<CqlResult, std::vector<GradeObject>>
GradesCqlManager::get_grades_by_student_id(const int& school_id,
                                           const CassUuid& student_id) {
  std::vector<GradeObject> grades;
  // Define the result handler that pre-allocates the memory for the grades
  // vector
  auto reserve_grades = [&grades](size_t num_rows) {
    // Reserve capacity for all returned grades
    grades.reserve(num_rows);
  };

  // Define the row handler that will store the grades
  auto copy_grade_details = [&grades](const CassRow* row) {
    grades.emplace_back();
    GradeObject& grade = grades.back();

    CqlResult cql_result = map_row_to_grade(row, grade);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_get_grades_by_evaluated.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);
  cass_statement_bind_uuid(statement.get(), 1, student_id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_grades, copy_grade_details);

  if (cql_result.code() != ResultCode::OK) {
    return {cql_result, std::vector<GradeObject>()};
  }

  return {cql_result, grades};
}

std::pair<CqlResult, std::vector<GradeObject>>
GradesCqlManager::get_grades_by_evaluator_id(const int& school_id,
                                             const CassUuid& evaluator_id) {
  std::vector<GradeObject> grades;
  // Define the result handler that pre-allocates the memory for the grades
  // vector
  auto reserve_grades = [&grades](size_t num_rows) {
    // Reserve capacity for all returned grades
    grades.reserve(num_rows);
  };

  // Define the row handler that will store the grades
  auto copy_grade_details = [&grades](const CassRow* row) {
    grades.emplace_back();
    GradeObject& grade = grades.back();

    CqlResult cql_result = map_row_to_grade(row, grade);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_get_grades_by_evaluator.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);
  cass_statement_bind_uuid(statement.get(), 1, evaluator_id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_grades, copy_grade_details);

  if (cql_result.code() != ResultCode::OK) {
    return {cql_result, std::vector<GradeObject>()};
  }

  return {cql_result, grades};
}

std::pair<CqlResult, std::vector<GradeObject>>
GradesCqlManager::get_grades_by_course_id(const int& school_id,
                                          const CassUuid& course_id) {
  std::vector<GradeObject> grades;
  // Define the result handler that pre-allocates the memory for the grades
  // vector
  auto reserve_grades = [&grades](size_t num_rows) {
    // Reserve capacity for all returned grades
    grades.reserve(num_rows);
  };

  // Define the row handler that will store the grades
  auto copy_grade_details = [&grades](const CassRow* row) {
    grades.emplace_back();
    GradeObject& grade = grades.back();

    CqlResult cql_result = map_row_to_grade(row, grade);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_get_grades_by_course.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);
  cass_statement_bind_uuid(statement.get(), 1, course_id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_grades, copy_grade_details);

  if (cql_result.code() != ResultCode::OK) {
    return {cql_result, std::vector<GradeObject>()};
  }

  return {cql_result, grades};
}

std::pair<CqlResult, std::vector<GradeObject>>
GradesCqlManager::get_grades_by_user_and_course(const int& school_id,
                                                const CassUuid& student_id,
                                                const CassUuid& course_id) {
  std::vector<GradeObject> grades;
  // Define the result handler that pre-allocates the memory for the grades
  // vector
  auto reserve_grades = [&grades](size_t num_rows) {
    // Reserve capacity for all returned grades
    grades.reserve(num_rows);
  };

  // Define the row handler that will store the grades
  auto copy_grade_details = [&grades](const CassRow* row) {
    grades.emplace_back();
    GradeObject& grade = grades.back();

    CqlResult cql_result = map_row_to_grade(row, grade);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_get_grades_by_evaluated_and_course.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);
  cass_statement_bind_uuid(statement.get(), 1, student_id);
  cass_statement_bind_uuid(statement.get(), 2, course_id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_grades, copy_grade_details);

  if (cql_result.code() != ResultCode::OK) {
    return {cql_result, std::vector<GradeObject>()};
  }

  return {cql_result, grades};
}

CqlResult GradesCqlManager::update_grade(const int& school_id,
                                         const CassUuid& id, const int& value,
                                         const int& out_of, const time_t& date,
                                         const float& weight) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_update_grade.get()));

    cass_statement_bind_int32(statement.get(), 0, value);
    cass_statement_bind_int32(statement.get(), 1, out_of);
    cass_statement_bind_int64(statement.get(), 2, date * 1000);
    cass_statement_bind_float(statement.get(), 3, weight);

    cass_statement_bind_int32(statement.get(), 4, school_id);
    cass_statement_bind_uuid(statement.get(), 5, id);

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

CqlResult GradesCqlManager::delete_grade(const int& school_id,
                                         const CassUuid& id) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_delete_grade.get()));

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

CqlResult map_row_to_grade(const CassRow* row, GradeObject& grade_object) {
  CqlResult cql_result = get_int_value(row, 0, grade_object._school_id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_uuid_value(row, 1, grade_object._id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_uuid_value(row, 2, grade_object._evaluated_id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_uuid_value(row, 3, grade_object._evaluator_id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_uuid_value(row, 4, grade_object._course_id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_int_value(row, 5, grade_object._grade);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_int_value(row, 6, grade_object._out_of);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_long_value(row, 7, grade_object._created_at);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }
  grade_object._created_at /= 1000;

  cql_result = get_float_value(row, 8, grade_object._weight);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
  }

  return cql_result;
}
