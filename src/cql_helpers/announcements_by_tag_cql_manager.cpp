/**
 * @file announcements_by_tag_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the relationship between announcements and tags
 * @version 0.1
 * @date 2023-01-02
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "cql_helpers/announcements_by_tag_cql_manager.hpp"

CqlResult AnnouncementsByTagCqlManager::configure(bool init_db_schema) {
  CqlResult cql_result = ResultCode::OK;

  if (init_db_schema) {
    cql_result = init_schema();

    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << "Failed to initialise announcements_id table: "
                << cql_result.str_code() << cql_result.error();
      return cql_result;
    }
  }

  cql_result = init_prepare_statements();

  return cql_result;
}

CqlResult AnnouncementsByTagCqlManager::init_prepare_statements() {
  CqlResult cql_result = _cql_client->prepare_statement(
      _prepared_insert_relationship, INSERT_RELATIONSHIP);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise insert relationship prepared statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(
      _prepared_get_announcements_by_tag, SELECT_ANNOUNCEMENTS);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select announcements by tag prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(
      _prepared_get_tags_by_announcement, SELECT_TAGS_BY_ANNOUNCEMENT);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select tags by announcement prepared "
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
      _prepared_delete_relationships_by_tag, DELETE_RELATIONSHiPS_BY_TAG);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise delete all relationships of a tag "
                 "prepared statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  return cql_result;
}

CqlResult AnnouncementsByTagCqlManager::init_schema() {
  CqlResult cql_result = _cql_client->execute_statement(CREATE_SCHOOL_KEYSPACE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise school keyspace: "
              << cql_result.str_code() << cql_result.error();
    return cql_result;
  }

  cql_result =
      _cql_client->execute_statement(CREATE_ANNOUNCEMENTS_BY_TAG_TABLE);

  return cql_result;
}

CqlResult AnnouncementsByTagCqlManager::create_relationship(
    const int school_id, const CassUuid& tag_id,
    const CassUuid& announcement_id) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_insert_relationship.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, tag_id);
    cass_statement_bind_uuid(statement.get(), 2, announcement_id);

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
AnnouncementsByTagCqlManager::get_announcements_by_tag(const int school_id,
                                                       const CassUuid& tag_id) {
  std::vector<CassUuid> announcements_id;
  // Define the result handler that pre-allocates the memory for the
  // announcements_id vector
  auto reserve_announcements_id = [&announcements_id](size_t num_rows) {
    // Reserve capacity for all returned announcements_id
    announcements_id.reserve(num_rows);
  };

  // Define the row handler that will store the announcements_id
  auto copy_announcement_id_details = [&announcements_id](const CassRow* row) {
    announcements_id.emplace_back();
    CassUuid& announcement_id = announcements_id.back();

    CqlResult cql_result = get_uuid_value(row, 0, announcement_id);
    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
                << cql_result.error();
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_get_announcements_by_tag.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);
  cass_statement_bind_uuid(statement.get(), 1, tag_id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_announcements_id, copy_announcement_id_details);

  if (cql_result.code() != ResultCode::OK) {
    return {cql_result, std::vector<CassUuid>()};
  }

  return {cql_result, announcements_id};
}

std::pair<CqlResult, std::vector<CassUuid>>
AnnouncementsByTagCqlManager::get_tags_by_announcement(
    const int school_id, const CassUuid& announcement_id) {
  std::vector<CassUuid> tags_ids;
  // Define the result handler that pre-allocates the memory for the
  // tags_ids vector
  auto reserve_tags_ids = [&tags_ids](size_t num_rows) {
    // Reserve capacity for all returned tags_ids
    tags_ids.reserve(num_rows);
  };

  // Define the row handler that will store the tags_ids
  auto copy_tag_id_details = [&tags_ids](const CassRow* row) {
    tags_ids.emplace_back();
    CassUuid& tag_id = tags_ids.back();

    CqlResult cql_result = get_uuid_value(row, 0, tag_id);
    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
                << cql_result.error();
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_get_tags_by_announcement.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);
  cass_statement_bind_uuid(statement.get(), 1, announcement_id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_tags_ids, copy_tag_id_details);

  if (cql_result.code() != ResultCode::OK) {
    return {cql_result, std::vector<CassUuid>()};
  }

  return {cql_result, tags_ids};
}

CqlResult AnnouncementsByTagCqlManager::delete_relationship(
    const int school_id, const CassUuid& tag_id,
    const CassUuid& announcement_id) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_delete_relationship.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, tag_id);
    cass_statement_bind_uuid(statement.get(), 2, announcement_id);

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

CqlResult AnnouncementsByTagCqlManager::delete_relationships_by_tag(
    const int school_id, const CassUuid& tag_id) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_delete_relationships_by_tag.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, tag_id);

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
