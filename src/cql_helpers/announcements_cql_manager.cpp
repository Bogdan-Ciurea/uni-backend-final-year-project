/**
 * @file announcements_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the announcements object
 * @version 0.1
 * @date 2022-12-16
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "cql_helpers/announcements_cql_manager.hpp"

CqlResult AnnouncementsCqlManager::configure(bool init_db_schema) {
  CqlResult cql_result = ResultCode::OK;

  if (init_db_schema) {
    cql_result = init_schema();

    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << "Failed to initialise announcements table: "
                << cql_result.str_code() << cql_result.error();
      return cql_result;
    }
  }

  cql_result = init_prepare_statements();

  return cql_result;
}

CqlResult AnnouncementsCqlManager::init_prepare_statements() {
  CqlResult cql_result = _cql_client->prepare_statement(
      _prepared_insert_announcement, INSERT_ANNOUNCEMENT);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise insert announcement prepared statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_get_announcement,
                                              SELECT_ANNOUNCEMENT);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select specific announcement prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_get_announcements_by_school_id,
                                     SELECT_ANNOUNCEMENTS_BY_SCHOOL_ID);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR
        << "Failed to initialise select all school announcements prepared "
           "statement: "
        << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_get_announcement_by_creator_id,
                                     SELECT_ANNOUNCEMENT_BY_CREATOR_ID);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR
        << "Failed to initialise select announcements by creator id prepared "
           "statement: "
        << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_update_announcement,
                                              UPDATE_ANNOUNCEMENT);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise update specific announcement prepared "
                 "statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_delete_announcement,
                                              DELETE_ANNOUNCEMENT);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise delete announcement prepared statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  return cql_result;
}

CqlResult AnnouncementsCqlManager::init_schema() {
  CqlResult cql_result = _cql_client->execute_statement(CREATE_SCHOOL_KEYSPACE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise school keyspace: "
              << cql_result.str_code() << cql_result.error();
    return cql_result;
  }

  cql_result = _cql_client->execute_statement(CREATE_ANNOUNCEMENTS_TABLE);

  return cql_result;
}

CqlResult AnnouncementsCqlManager::create_announcement(
    const AnnouncementObject& announcement_data) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_insert_announcement.get()));

    cass_statement_bind_int32(statement.get(), 0, announcement_data._school_id);
    cass_statement_bind_uuid(statement.get(), 1, announcement_data._id);
    cass_statement_bind_int64(statement.get(), 2,
                              announcement_data._created_at * 1000);
    cass_statement_bind_uuid(statement.get(), 3, announcement_data._created_by);
    cass_statement_bind_string(statement.get(), 4,
                               announcement_data._title.c_str());
    cass_statement_bind_string(statement.get(), 5,
                               announcement_data._content.c_str());
    cass_statement_bind_bool(
        statement.get(), 6,
        announcement_data._allow_answers == true ? cass_true : cass_false);

    // Generate the list of uuids
    CassCollectionPtr list(cass_collection_new(
        CASS_COLLECTION_TYPE_LIST, announcement_data._files.size()));

    // Append the uuids to the list. Add them back to front
    // because the cassandra driver adds them in reverse order
    for (auto& file : announcement_data._files) {
      cass_collection_append_uuid(list.get(), file);
    }

    if (cass_statement_bind_collection(statement.get(), 7, list.get()) !=
        CASS_OK) {
      LOG_ERROR << "Failed to bind collection of files to announcement";
      return {ResultCode::UNKNOWN_ERROR,
              "Failed to bind collection of files to announcement"};
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

std::pair<CqlResult, AnnouncementObject>
AnnouncementsCqlManager::get_announcement_by_id(const int& school_id,
                                                const CassUuid& id) {
  std::vector<AnnouncementObject> announcements;
  // Define the result handler that pre-allocate space in announcements vector
  auto reserve_announcements = [&announcements](size_t num_rows) {
    // Reserve capacity for all returned announcements
    announcements.reserve(num_rows);
  };

  // Define the row handler that will store the announcements
  auto copy_announcement_details = [&announcements](const CassRow* row) {
    announcements.emplace_back();
    AnnouncementObject& announcement = announcements.back();

    CqlResult cql_result = map_row_to_announcement(row, announcement);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_get_announcement.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);
  cass_statement_bind_uuid(statement.get(), 1, id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_announcements, copy_announcement_details);

  if (cql_result.code() != ResultCode::OK)
    return {cql_result, AnnouncementObject()};

  if (announcements.size() != 1)
    return {{ResultCode::NOT_FOUND, "No entries found"}, AnnouncementObject()};

  return {cql_result, announcements.at(0)};
}

std::pair<CqlResult, std::vector<AnnouncementObject>>
AnnouncementsCqlManager::get_announcement_school_id(const int& school_id) {
  std::vector<AnnouncementObject> announcements;
  // Define the result handler that pre-allocate space in announcements vector
  auto reserve_announcements = [&announcements](size_t num_rows) {
    // Reserve capacity for all returned announcements
    announcements.reserve(num_rows);
  };

  // Define the row handler that will store the announcements
  auto copy_announcement_details = [&announcements](const CassRow* row) {
    announcements.emplace_back();
    AnnouncementObject& announcement = announcements.back();

    CqlResult cql_result = map_row_to_announcement(row, announcement);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_get_announcements_by_school_id.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_announcements, copy_announcement_details);

  if (cql_result.code() != ResultCode::OK) return {cql_result, {}};

  return {cql_result, announcements};
}

std::pair<CqlResult, std::vector<AnnouncementObject>>
AnnouncementsCqlManager::get_announcements_by_creator_id(
    const int& school_id, const CassUuid& creator_id) {
  std::vector<AnnouncementObject> announcements;

  // Define the result handler that pre-allocate space in announcements vector
  auto reserve_announcements = [&announcements](size_t num_rows) {
    // Reserve capacity for all returned announcements
    announcements.reserve(num_rows);
  };

  // Define the row handler that will store the announcements
  auto copy_announcement_details = [&announcements](const CassRow* row) {
    announcements.emplace_back();
    AnnouncementObject& announcement = announcements.back();

    CqlResult cql_result = map_row_to_announcement(row, announcement);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_get_announcement_by_creator_id.get()));
  cass_statement_bind_int32(statement.get(), 0, school_id);
  cass_statement_bind_uuid(statement.get(), 1, creator_id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_announcements, copy_announcement_details);

  if (cql_result.code() != ResultCode::OK) return {cql_result, {}};

  return {cql_result, announcements};
}

CqlResult AnnouncementsCqlManager::update_announcement(
    const int& school_id, const CassUuid& id, const time_t& created_at,
    const CassUuid& created_by, const std::string& title,
    const std::string& contents, const bool& allow_answers,
    const std::vector<CassUuid>& files) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_update_announcement.get()));

    cass_statement_bind_uuid(statement.get(), 0, created_by);
    cass_statement_bind_string(statement.get(), 1, title.c_str());
    cass_statement_bind_string(statement.get(), 2, contents.c_str());
    cass_statement_bind_bool(statement.get(), 3,
                             allow_answers == true ? cass_true : cass_false);

    // Generate the list of uuids
    CassCollection* list =
        cass_collection_new(CASS_COLLECTION_TYPE_LIST, files.size());

    // Append the uuids to the list
    for (const auto& file : files) {
      cass_collection_append_uuid(list, file);
    }

    cass_statement_bind_collection(statement.get(), 4, list);
    cass_statement_bind_int32(statement.get(), 5, school_id);
    cass_statement_bind_uuid(statement.get(), 6, id);
    cass_statement_bind_int64(statement.get(), 7, created_at * 1000);

    cass_collection_free(list);

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

CqlResult AnnouncementsCqlManager::delete_announcement_by_id(
    const int& school_id, const CassUuid& id, const time_t& created_at) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_delete_announcement.get()));

    cass_statement_bind_int32(statement.get(), 0, school_id);
    cass_statement_bind_uuid(statement.get(), 1, id);
    cass_statement_bind_int64(statement.get(), 2, created_at * 1000);

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

CqlResult map_row_to_announcement(const CassRow* row,
                                  AnnouncementObject& announcement) {
  CqlResult cql_result = get_int_value(row, 0, announcement._school_id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_uuid_value(row, 1, announcement._id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_long_value(row, 2, announcement._created_at);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }
  announcement._created_at /= 1000;

  cql_result = get_uuid_value(row, 3, announcement._created_by);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_text_value(row, 4, announcement._title);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_text_value(row, 5, announcement._content);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_bool_value(row, 6, announcement._allow_answers);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_array_uuids_value(row, 7, announcement._files);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  return {ResultCode::OK};
}