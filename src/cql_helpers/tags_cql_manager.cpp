/**
 * @file tags_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the tag object
 * @version 0.1
 * @date 2023-01-04
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "cql_helpers/tags_cql_manager.hpp"

CqlResult TagsCqlManager::configure(bool init_db_schema) {
  CqlResult cql_result = ResultCode::OK;

  if (init_db_schema) {
    cql_result = init_schema();

    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << "Failed to initialise tags table: " << cql_result.str_code()
                << cql_result.error();
      return cql_result;
    }
  }

  cql_result = init_prepare_statements();

  return cql_result;
}

CqlResult TagsCqlManager::init_prepare_statements() {
  CqlResult cql_result =
      _cql_client->prepare_statement(_prepared_insert_tag, INSERT_TAG);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise insert tag statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_get_tag_by_id, SELECT_TAG_BY_ID);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select tag by id prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_get_tags_by_school_id,
                                              SELECT_TAGS_BY_SCHOOL_ID);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select tags by school id prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_update_tag, UPDATE_TAG);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise update tag prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_delete_tag, DELETE_TAG);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise delete tag prepared statement: "
              << cql_result.str_code() << cql_result.error();
  }

  return cql_result;
}

CqlResult TagsCqlManager::init_schema() {
  CqlResult cql_result = _cql_client->execute_statement(CREATE_SCHOOL_KEYSPACE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise school keyspace: "
              << cql_result.str_code() << cql_result.error();
    return cql_result;
  }

  cql_result = _cql_client->execute_statement(CREATE_TAGS_TABLE);

  return cql_result;
}

CqlResult TagsCqlManager::create_tag(const TagObject& tag_data) {
  std::string error_message;

  try {
    CassStatementPtr statement(cass_prepared_bind(_prepared_insert_tag.get()));

    cass_statement_bind_int32(statement.get(), 0, tag_data._school_id);
    cass_statement_bind_uuid(statement.get(), 1, tag_data._id);
    cass_statement_bind_string(statement.get(), 2, tag_data._name.c_str());
    cass_statement_bind_string(statement.get(), 3, tag_data._colour.c_str());

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

std::pair<CqlResult, TagObject> TagsCqlManager::get_tag_by_id(
    const int& school_id, const CassUuid& id) {
  std::vector<TagObject> tags;
  // Define the result handler that pre-allocates the memory for the tags
  // vector
  auto reserve_tags = [&tags](size_t num_rows) {
    // Reserve capacity for all returned tags
    tags.reserve(num_rows);
  };

  // Define the row handler that will store the tags
  auto copy_tag_details = [&tags](const CassRow* row) {
    tags.emplace_back();
    TagObject& tag = tags.back();

    CqlResult cql_result = map_row_to_tag(row, tag);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(cass_prepared_bind(_prepared_get_tag_by_id.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);
  cass_statement_bind_uuid(statement.get(), 1, id);

  CqlResult cql_result =
      _cql_client->select_rows(statement.get(), reserve_tags, copy_tag_details);

  if (cql_result.code() != ResultCode::OK) {
    return {cql_result, TagObject()};
  }

  if (tags.size() != 1) {
    return {{ResultCode::NOT_FOUND, "No entries found"}, TagObject()};
  }

  return {cql_result, tags.at(0)};
}

std::pair<CqlResult, std::vector<TagObject>>
TagsCqlManager::get_tags_by_school_id(const int& school_id) {
  std::vector<TagObject> tags;
  // Define the result handler that pre-allocates the memory for the tags
  // vector
  auto reserve_tags = [&tags](size_t num_rows) {
    // Reserve capacity for all returned tags
    tags.reserve(num_rows);
  };

  // Define the row handler that will store the tags
  auto copy_tag_details = [&tags](const CassRow* row) {
    tags.emplace_back();
    TagObject& tag = tags.back();

    CqlResult cql_result = map_row_to_tag(row, tag);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_get_tags_by_school_id.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);

  CqlResult cql_result =
      _cql_client->select_rows(statement.get(), reserve_tags, copy_tag_details);

  if (cql_result.code() != ResultCode::OK) {
    return {cql_result, std::vector<TagObject>()};
  }

  return {cql_result, tags};
}

CqlResult TagsCqlManager::update_tag(const int& school_id, const CassUuid& id,
                                     const std::string& name,
                                     const std::string& colour) {
  std::string error_message;

  try {
    CassStatementPtr statement(cass_prepared_bind(_prepared_update_tag.get()));

    cass_statement_bind_string(statement.get(), 0, name.c_str());
    cass_statement_bind_string(statement.get(), 1, colour.c_str());
    cass_statement_bind_int32(statement.get(), 2, school_id);
    cass_statement_bind_uuid(statement.get(), 3, id);

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

CqlResult TagsCqlManager::delete_tag(const int& school_id, const CassUuid& id) {
  std::string error_message;

  try {
    CassStatementPtr statement(cass_prepared_bind(_prepared_delete_tag.get()));

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

CqlResult map_row_to_tag(const CassRow* row, TagObject& tag_object) {
  CqlResult cql_result = get_int_value(row, 0, tag_object._school_id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_uuid_value(row, 1, tag_object._id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_text_value(row, 2, tag_object._name);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_text_value(row, 3, tag_object._colour);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
  }

  return cql_result;
}