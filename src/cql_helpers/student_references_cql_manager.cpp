/**
 * @reference student_references_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This reference is responsible for communication between the backend
 * and database and manages the school object
 * @version 0.1
 * @date 2023-01-06
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "cql_helpers/student_references_cql_manager.hpp"

CqlResult StudentReferencesCqlManager::configure(bool init_db_schema) {
  CqlResult cql_result = ResultCode::OK;

  if (init_db_schema) {
    cql_result = init_schema();

    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << "Failed to initialise references table: "
                << cql_result.str_code() << cql_result.error();
      return cql_result;
    }
  }

  cql_result = init_prepare_statements();

  return cql_result;
}

CqlResult StudentReferencesCqlManager::init_prepare_statements() {
  CqlResult cql_result = _cql_client->prepare_statement(
      _prepared_insert_reference, INSERT_REFERENCE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise insert reference statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(
      _prepared_select_references_by_student, SELECT_REFERENCES_BY_STUDENT);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select references prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_delete_reference,
                                              DELETE_REFERENCE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise delete specific reference prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(
      _prepared_delete_references_by_student, DELETE_REFERENCES_BY_STUDENT);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR
        << "Failed to initialise delete references by student id statement: "
        << cql_result.str_code() << cql_result.error();
  }

  return cql_result;
}

CqlResult StudentReferencesCqlManager::init_schema() {
  CqlResult cql_result = _cql_client->execute_statement(CREATE_SCHOOL_KEYSPACE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise school keyspace: "
              << cql_result.str_code() << cql_result.error();
    return cql_result;
  }

  cql_result = _cql_client->execute_statement(CREATE_STUDENT_REFERENCES_TABLE);

  return cql_result;
}

CqlResult StudentReferencesCqlManager::create_student_reference(
    const StudentReferenceObject& student_reference_data) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_insert_reference.get()));

    cass_statement_bind_int32(statement.get(), 0,
                              student_reference_data._school_id);
    cass_statement_bind_uuid(statement.get(), 1,
                             student_reference_data._student_id);
    cass_statement_bind_string(statement.get(), 2,
                               student_reference_data._reference.c_str());
    cass_statement_bind_int32(statement.get(), 3,
                              static_cast<int>(student_reference_data._type));

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

std::pair<CqlResult, std::vector<StudentReferenceObject>>
StudentReferencesCqlManager::get_student_references(
    const int& school_id, const CassUuid& student_id) {
  std::vector<StudentReferenceObject> references;
  // Define the result handler that pre-allocates the memory for the references
  // vector
  auto reserve_references = [&references](size_t num_rows) {
    // Reserve capacity for all returned references
    references.reserve(num_rows);
  };

  // Define the row handler that will store the references
  auto copy_reference_details = [&references](const CassRow* row) {
    references.emplace_back();
    StudentReferenceObject& reference = references.back();

    CqlResult cql_result = map_row_to_reference(row, reference);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_select_references_by_student.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);
  cass_statement_bind_uuid(statement.get(), 1, student_id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_references, copy_reference_details);

  if (cql_result.code() != ResultCode::OK) {
    return {cql_result, std::vector<StudentReferenceObject>()};
  }

  if (references.size() == 0) {
    return {{ResultCode::NOT_FOUND, "No entries found"},
            std::vector<StudentReferenceObject>()};
  }

  return {cql_result, references};
}

CqlResult StudentReferencesCqlManager::update_student_reference(
    const int& school_id, const CassUuid& student_id,
    const std::string& old_reference, const std::string& new_reference,
    const ReferenceType& type) {
  StudentReferenceObject reference_data(school_id, student_id, new_reference,
                                        type);

  CqlResult cql_result =
      this->delete_student_reference(school_id, student_id, old_reference);
  if (cql_result.code() != ResultCode::OK) {
    return cql_result;
  }

  return this->create_student_reference(reference_data);
}

CqlResult StudentReferencesCqlManager::delete_student_reference(
    const int& school_id, const CassUuid& student_id,
    const std::string& reference) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_delete_reference.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, student_id);
    cass_statement_bind_string(statement.get(), 2, reference.c_str());

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

CqlResult StudentReferencesCqlManager::delete_student_references(
    const int& school_id, const CassUuid& student_id) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_delete_references_by_student.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, student_id);

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

CqlResult map_row_to_reference(const CassRow* row,
                               StudentReferenceObject& reference) {
  CqlResult cql_result = get_int_value(row, 0, reference._school_id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_uuid_value(row, 1, reference._student_id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_text_value(row, 2, reference._reference);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  int temp_int;
  cql_result = get_int_value(row, 3, temp_int);
  if (cql_result.code() != ResultCode::OK || temp_int < 0 || temp_int >= 2) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }
  if (temp_int == 0) {
    reference._type = ReferenceType::EMAIL;
  } else {
    reference._type = ReferenceType::PHONE_NUMBER;
  }

  return CqlResult(ResultCode::OK);
}