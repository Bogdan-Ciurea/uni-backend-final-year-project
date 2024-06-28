/**
 * @file announcements_cql_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the announcements object
 * @version 0.1
 * @date 2022-12-16
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef ANNOUNCEMENTS_CQL_MANAGER_H
#define ANNOUNCEMENTS_CQL_MANAGER_H

#include "cql_client.hpp"
#include "database_objects/announcements_object.hpp"

class AnnouncementsCqlManager {
 public:
  /**
   * @brief Construct a new Announcement Cql Manager object
   *
   * @param cql_client - The cassandra cql client
   */
  AnnouncementsCqlManager(CqlClient* cql_client) : _cql_client(cql_client) {}

  /**
   * @brief Destroy the Announcement Cql Manager object
   *
   */
  ~AnnouncementsCqlManager() {}

  /**
   * @brief Will prepare the database to be operated on
   *
   * @param init_db_schema - If you want to also create the keyspace and table
   *
   * @return CqlResult     - The result of the operation
   */
  CqlResult configure(bool init_db_schema);

  /**
   * @brief Adds a announcement entry on the database
   *
   * @param announcement_data - The announcement object we want to add
   *
   * @return CqlResult   - The result of the operation
   */
  CqlResult create_announcement(const AnnouncementObject& announcement_data);

  /**
   * @brief Get the announcement object
   *
   * @param school_id  - The id of the school
   * @param id         - The uuid of the announcement
   *
   * @return std::pair<CqlResult, AnnouncementObject>
   */
  std::pair<CqlResult, AnnouncementObject> get_announcement_by_id(
      const int& school_id, const CassUuid& id);

  /**
   * @brief Get all announcements by school id
   *
   * @param school_id  - The id of the school
   *
   * @return std::pair<CqlResult, std::vector<AnnouncementObject>>
   */
  std::pair<CqlResult, std::vector<AnnouncementObject>>
  get_announcement_school_id(const int& school_id);

  /**
   * @brief Get the announcements by creator id object
   *
   * @param school_id  - The id of the school
   * @param id         - The uuid of the creator
   * @return std::pair<CqlResult, std::vector<AnnouncementObject>>
   */
  std::pair<CqlResult, std::vector<AnnouncementObject>>
  get_announcements_by_creator_id(const int& school_id,
                                  const CassUuid& creator_id);

  /**
   * @brief Update the announcement object
   *
   * @param school_id     - The id of the school
   * @param id            - The uuid of the announcement
   * @param created_at    - The creation time of the announcement
   * @param created_by    - The creator of the announcement
   * @param title         - The title of the announcement
   * @param contents      - The contents of the announcement
   * @param allow_answers - If the announcement allows answers
   * @param files         - The files linked to this announcement
   *
   * @return CqlResult - The result of the operation
   */
  CqlResult update_announcement(const int& school_id, const CassUuid& id,
                                const time_t& created_at,
                                const CassUuid& created_by,
                                const std::string& title,
                                const std::string& contents,
                                const bool& allow_answers,
                                const std::vector<CassUuid>& files);

  /**
   * @brief Delete all of the announcement object based on the id
   *
   * @param school_id  - The id of the school
   * @param id         - The uuid of the announcement
   * @param created_at - The creation time of the announcement
   *
   * @return CqlResult - The result of the operation
   */
  CqlResult delete_announcement_by_id(const int& school_id, const CassUuid& id,
                                      const time_t& created_at);

 private:
  // Pointer to the cassandra cql client
  CqlClient* _cql_client;

  CassPreparedPtr _prepared_insert_announcement;

  CassPreparedPtr _prepared_get_announcement;

  CassPreparedPtr _prepared_get_announcement_by_creator_id;

  CassPreparedPtr _prepared_get_announcements_by_school_id;

  CassPreparedPtr _prepared_update_announcement;

  CassPreparedPtr _prepared_delete_announcement;

  /**
   * @brief Initialize the prepared statements used by this class
   *
   * @return CqlResult The result of the operation
   */
  CqlResult init_prepare_statements();

  /**
   * @brief Creates the keyspace and the table
   *
   * @return CqlResult The result of the operation
   */
  CqlResult init_schema();

  const char* CREATE_SCHOOL_KEYSPACE =
      "CREATE KEYSPACE IF NOT EXISTS schools WITH REPLICATION = { 'class' : "
      "'SimpleStrategy', 'replication_factor' : 3 };";

  const char* CREATE_ANNOUNCEMENTS_TABLE =
      "CREATE TABLE IF NOT EXISTS schools.announcements ( "
      "school int, "
      "id uuid, "
      "created_at timestamp, "
      "created_by uuid, "
      "title varchar, "
      "content text, "
      "allow_answers boolean, "
      "files list <uuid>, "
      "PRIMARY KEY ((school, id), created_at)) "
      "WITH CLUSTERING ORDER BY (created_at DESC);";

  const char* INSERT_ANNOUNCEMENT =
      "INSERT INTO schools.announcements (school, id, created_at, created_by, "
      "title, content, allow_answers, files) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?) IF NOT EXISTS;";

  const char* SELECT_ANNOUNCEMENT =
      "SELECT school, id, created_at, created_by, title, content, "
      "allow_answers, files "
      "FROM schools.announcements WHERE school = ? AND id = ?;";

  const char* SELECT_ANNOUNCEMENTS_BY_SCHOOL_ID =
      "SELECT school, id, created_at, created_by, title, content, "
      "allow_answers, files "
      "FROM schools.announcements WHERE school = ? ALLOW FILTERING;";

  const char* SELECT_ANNOUNCEMENT_BY_CREATOR_ID =
      "SELECT school, id, created_at, created_by, title, content, "
      "allow_answers, files "
      "FROM schools.announcements WHERE school = ? AND created_by = ? ALLOW "
      "FILTERING;";

  const char* UPDATE_ANNOUNCEMENT =
      "UPDATE schools.announcements SET created_by = ?, title = ?, content = "
      "?, allow_answers = ?, files = ? WHERE school = ? AND id = ? AND "
      "created_at = ? IF "
      "EXISTS;";

  const char* DELETE_ANNOUNCEMENT =
      "DELETE FROM schools.announcements WHERE school = ? AND id = ? AND "
      "created_at = ? IF EXISTS;";
};

CqlResult map_row_to_announcement(const CassRow* row,
                                  AnnouncementObject& announcement);

#endif  // ANNOUNCEMENTS_CQL_MANAGER_H
