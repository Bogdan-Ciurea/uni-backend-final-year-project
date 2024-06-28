/**
 * @file tags_cql_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the tag object
 * @version 0.1
 * @date 2023-01-04
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef TAGS_CQL_MANAGER_H
#define TAGS_CQL_MANAGER_H

#include "cql_client.hpp"
#include "database_objects/tag_object.hpp"

class TagsCqlManager {
 public:
  /**
   * @brief Construct a new Tags Cql Manager object
   *
   * @param cql_client - The cassandra cql client
   */
  TagsCqlManager(CqlClient* cql_client) : _cql_client(cql_client) {}

  /**
   * @brief Destroy the Tags Cql Manager object
   *
   */
  ~TagsCqlManager() {}

  /**
   * @brief Will prepare the database to be operated on
   *
   * @param init_db_schema - If you want to also create the keyspace and table
   *
   * @return CqlResult     - The result of the operation
   */
  CqlResult configure(bool init_db_schema);

  /**
   * @brief Adds a tag entry on the database
   *
   * @param tag_data - The tag object we want to add
   *
   * @return CqlResult   - The result of the operation
   */
  CqlResult create_tag(const TagObject& tag_data);

  /**
   * @brief Get the tag object
   *
   * @param school_id  - The id of the school
   * @param id         - The uuid of the tag
   *
   * @return std::pair<CqlResult, TagObject>
   */
  std::pair<CqlResult, TagObject> get_tag_by_id(const int& school_id,
                                                const CassUuid& id);

  /**
   * @brief Get all the tags for a given school
   *
   * @param school_id  - The id of the school
   *
   * @return std::pair<CqlResult, std::vector<TagObject>>
   */
  std::pair<CqlResult, std::vector<TagObject>> get_tags_by_school_id(
      const int& school_id);

  /**
   * @brief Update a tag entry on the database
   *
   * @param school_id - The id of the school
   * @param id        - The uuid of the tag
   * @param name      - The new name of the tag
   * @param colour    - The new colour of the tag
   *
   * @return CqlResult   - The result of the operation
   */
  CqlResult update_tag(const int& school_id, const CassUuid& id,
                       const std::string& name, const std::string& colour);

  /**
   * @brief Delete a tag entry on the database
   *
   * @param school_id - The id of the school
   * @param id        - The uuid of the tag
   *
   * @return CqlResult   - The result of the operation
   */
  CqlResult delete_tag(const int& school_id, const CassUuid& id);

 private:
  CqlClient* _cql_client;

  CassPreparedPtr _prepared_insert_tag;

  CassPreparedPtr _prepared_get_tag_by_id;

  CassPreparedPtr _prepared_get_tags_by_school_id;

  CassPreparedPtr _prepared_update_tag;

  CassPreparedPtr _prepared_delete_tag;

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

  const char* CREATE_TAGS_TABLE =
      "CREATE TABLE IF NOT EXISTS schools.tags ( "
      "school int, "
      "id uuid, "
      "value varchar, "
      "colour varchar, "
      "PRIMARY KEY ((school, id)));";

  const char* INSERT_TAG =
      "INSERT INTO schools.tags (school, id, value, colour) VALUES (?, ?, ?, "
      "?) IF NOT EXISTS ;";

  const char* SELECT_TAG_BY_ID =
      "SELECT school, id, value, colour FROM schools.tags WHERE school = ? AND "
      "id = ?;";

  const char* SELECT_TAGS_BY_SCHOOL_ID =
      "SELECT school, id, value, colour FROM schools.tags WHERE school = ? "
      "ALLOW "
      "FILTERING;";

  const char* UPDATE_TAG =
      "UPDATE schools.tags SET value = ?, colour = ? WHERE school = ? AND id = "
      "? IF EXISTS;";

  const char* DELETE_TAG =
      "DELETE FROM schools.tags WHERE school = ? AND id = ? IF EXISTS;";
};

CqlResult map_row_to_tag(const CassRow* row, TagObject& tag_object);

#endif