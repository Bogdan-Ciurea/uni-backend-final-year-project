/**
 * @file files_cql_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the file object
 * @version 0.1
 * @date 2023-01-03
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "cql_helpers/files_cql_manager.hpp"

CqlResult FilesCqlManager::configure(bool init_db_schema) {
  CqlResult cql_result = ResultCode::OK;

  if (init_db_schema) {
    cql_result = init_schema();

    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << "Failed to initialise files table: " << cql_result.str_code()
                << cql_result.error();
      return cql_result;
    }
  }

  cql_result = init_prepare_statements();

  return cql_result;
}

CqlResult FilesCqlManager::init_prepare_statements() {
  CqlResult cql_result =
      _cql_client->prepare_statement(_prepared_insert_file, INSERT_FILE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise insert file statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_get_file, SELECT_FILE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select file prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_update_file, UPDATE_FILE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise update file prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_delete_file, DELETE_FILE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise delete file prepared statement: "
              << cql_result.str_code() << cql_result.error();
  }

  return cql_result;
}

CqlResult FilesCqlManager::init_schema() {
  CqlResult cql_result = _cql_client->execute_statement(CREATE_SCHOOL_KEYSPACE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise school keyspace: "
              << cql_result.str_code() << cql_result.error();
    return cql_result;
  }

  cql_result = _cql_client->execute_statement(CREATE_FILES_TABLE);

  return cql_result;
}

CqlResult FilesCqlManager::create_file(const FileObject& file) {
  std::string error_message;

  try {
    CassStatementPtr statement(cass_prepared_bind(_prepared_insert_file.get()));

    cass_statement_bind_int32(statement.get(), 0, file._school_id);
    cass_statement_bind_uuid(statement.get(), 1, file._id);
    cass_statement_bind_int32(statement.get(), 2, static_cast<int>(file._type));

    // Generate the list of uuids
    CassCollectionPtr list(
        cass_collection_new(CASS_COLLECTION_TYPE_LIST, file._files.size()));

    // Append the uuids to the list. Add them back to front
    // because the cassandra driver adds them in reverse order
    for (auto& file : file._files) {
      cass_collection_append_uuid(list.get(), file);
    }

    if (cass_statement_bind_collection(statement.get(), 3, list.get()) !=
        CASS_OK) {
      LOG_ERROR << "Failed to bind collection of files to file_data";
      return {ResultCode::UNKNOWN_ERROR,
              "Failed to bind collection of files to file_data"};
    }

    cass_statement_bind_string(statement.get(), 4, file._name.c_str());
    cass_statement_bind_string(statement.get(), 5, file._path_to_file.c_str());
    cass_statement_bind_int32(statement.get(), 6, file._size);
    cass_statement_bind_uuid(statement.get(), 7, file._added_by_user);
    cass_statement_bind_bool(
        statement.get(), 8,
        file._visible_to_students == true ? cass_true : cass_false);
    cass_statement_bind_bool(
        statement.get(), 9,
        file._students_can_add == true ? cass_true : cass_false);

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

std::pair<CqlResult, FileObject> FilesCqlManager::get_file_by_id(
    const int& school_id, const CassUuid& id) {
  std::vector<FileObject> files;
  // Define the result handler that pre-allocates the memory for the files
  // vector
  auto reserve_files = [&files](size_t num_rows) {
    // Reserve capacity for all returned files
    files.reserve(num_rows);
  };

  // Define the row handler that will store the files
  auto copy_file_details = [&files](const CassRow* row) {
    files.emplace_back();
    FileObject& file = files.back();

    CqlResult cql_result = map_row_to_file(row, file);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(cass_prepared_bind(_prepared_get_file.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);
  cass_statement_bind_uuid(statement.get(), 1, id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_files, copy_file_details);

  if (cql_result.code() != ResultCode::OK) {
    return {cql_result, FileObject()};
  }

  if (files.size() != 1) {
    return {{ResultCode::NOT_FOUND, "No entries found"}, FileObject()};
  }

  return {cql_result, files.at(0)};
}

CqlResult FilesCqlManager::update_file(
    const int& school_id, const CassUuid& id, const CustomFileType& type,
    const std::string& name, const std::vector<CassUuid>& files,
    const std::string& path_to_file, const int& size,
    const CassUuid& added_by_user, const bool& visible_to_students,
    const bool& students_can_add) {
  std::string error_message;

  try {
    CassStatementPtr statement(cass_prepared_bind(_prepared_update_file.get()));

    cass_statement_bind_int32(statement.get(), 0, (int)type);

    // Generate the list of uuids
    CassCollectionPtr list(
        cass_collection_new(CASS_COLLECTION_TYPE_LIST, files.size()));

    // Append the uuids to the list. Add them back to front
    // because the cassandra driver adds them in reverse order
    for (auto& file : files) {
      cass_collection_append_uuid(list.get(), file);
    }

    if (cass_statement_bind_collection(statement.get(), 1, list.get()) !=
        CASS_OK) {
      LOG_ERROR << "Failed to bind collection of files to file_data";
      return {ResultCode::UNKNOWN_ERROR,
              "Failed to bind collection of files to file_data"};
    }

    cass_statement_bind_string(statement.get(), 2, name.c_str());
    cass_statement_bind_string(statement.get(), 3, path_to_file.c_str());
    cass_statement_bind_int32(statement.get(), 4, size);
    cass_statement_bind_uuid(statement.get(), 5, added_by_user);
    cass_statement_bind_bool(
        statement.get(), 6,
        visible_to_students == true ? cass_true : cass_false);
    cass_statement_bind_bool(statement.get(), 7,
                             students_can_add == true ? cass_true : cass_false);
    cass_statement_bind_int32(statement.get(), 8, school_id);
    cass_statement_bind_uuid(statement.get(), 9, id);

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

CqlResult FilesCqlManager::delete_file(const int& school_id,
                                       const CassUuid& id) {
  std::string error_message;

  try {
    CassStatementPtr statement(cass_prepared_bind(_prepared_delete_file.get()));

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

CqlResult map_row_to_file(const CassRow* row, FileObject& file) {
  CqlResult cql_result = get_int_value(row, 0, file._school_id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_uuid_value(row, 1, file._id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  int temp_int;
  cql_result = get_int_value(row, 2, temp_int);
  if (cql_result.code() != ResultCode::OK || temp_int < 0 || temp_int >= 2) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }
  if (temp_int == 0) {
    file._type = CustomFileType::FILE;
  } else {
    file._type = CustomFileType::FOLDER;
  }

  cql_result = get_array_uuids_value(row, 3, file._files);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_text_value(row, 4, file._name);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_text_value(row, 5, file._path_to_file);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_int_value(row, 6, file._size);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_uuid_value(row, 7, file._added_by_user);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_bool_value(row, 8, file._visible_to_students);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_bool_value(row, 9, file._students_can_add);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  return CqlResult(ResultCode::OK);
}