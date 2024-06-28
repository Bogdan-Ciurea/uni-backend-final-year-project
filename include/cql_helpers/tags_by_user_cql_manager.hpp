/**
 * @file tags_by_user_cql_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the relationship between tags and users
 * @version 0.1
 * @date 2023-01-04
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef TAGS_BY_USER_CQL_MANAGER_H
#define TAGS_BY_USER_CQL_MANAGER_H

#include "cql_client.hpp"

class TagsByUserCqlManager {
 public:
  TagsByUserCqlManager(CqlClient* cql_client) : _cql_client(cql_client) {}
  ~TagsByUserCqlManager() {}

  /**
   * @brief Configures the class
   *
   * @param init_db_schema - If true, the keyspace and table will be created
   * @return CqlResult - The result of the operation
   */
  CqlResult configure(bool init_db_schema);

  /**
   * @brief Creates a relationship between a tag and a user
   *
   * @param school_id - The id of the school
   * @param user_id - The id of the user
   * @param tag_id - The id of the tag
   * @return CqlResult - The result of the operation
   */
  CqlResult create_relationship(const int school_id, const CassUuid& user_id,
                                const CassUuid& tag_id);

  /**
   * @brief Get the tags by user
   *
   * @param school_id - The id of the school
   * @param user_id - The id of the user
   * @return std::pair<CqlResult, std::vector<CassUuid>> - The result of the
   * operation and the vector of tag ids
   */
  std::pair<CqlResult, std::vector<CassUuid>> get_tags_by_user(
      const int school_id, const CassUuid& user_id);

  /**
   * @brief Deletes a relationship between a tag and a user
   *
   * @param school_id - The id of the school
   * @param user_id - The id of the user
   * @param tag_id - The id of the tag
   * @return CqlResult - The result of the operation
   */
  CqlResult delete_relationship(const int school_id, const CassUuid& user_id,
                                const CassUuid& tag_id);

  /**
   * @brief Deletes all relationships between multiple tags and one user
   *
   * @param school_id - The id of the school
   * @param user_id - The id of the user
   *
   * @return CqlResult - The result of the operation
   */
  CqlResult delete_relationships_by_user(const int school_id,
                                         const CassUuid& user_id);

 private:
  CqlClient* _cql_client;

  CassPreparedPtr _prepared_insert_relationship;

  CassPreparedPtr _prepared_get_tags_by_user;

  CassPreparedPtr _prepared_delete_relationship;

  CassPreparedPtr _prepared_delete_relationships_by_user;

  /**
   * @brief Initialize the prepared statements used by this class
   *
   * @return CqlResult - The result of the operation
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

  const char* CREATE_TAGS_BY_USER_TABLE =
      "CREATE TABLE IF NOT EXISTS schools.tags_by_user ( "
      "school int, "
      "user_id uuid, "
      "tag_id uuid, "
      "PRIMARY KEY ((school, user_id), tag_id));";

  const char* INSERT_RELATIONSHIP =
      "INSERT INTO schools.tags_by_user (school, user_id, tag_id ) VALUES (?, "
      "?, ?) IF NOT EXISTS;";

  const char* SELECT_TAGS =
      "SELECT tag_id  FROM schools.tags_by_user WHERE school = ? AND user_id = "
      "?;";

  // WIll delete a specific relationship between a tag and a user
  // Would be used when deleting a tag or a user is assigned to a tag
  const char* DELETE_RELATIONSHIP =
      "DELETE FROM schools.tags_by_user WHERE school = ? AND user_id = ? AND "
      "tag_id = ? IF EXISTS;";

  // Will delete all relationships between a user and all tags
  // Would be used when deleting a user
  const char* DELETE_RELATIONSHIPS_BY_USER =
      "DELETE FROM schools.tags_by_user WHERE school = ? AND user_id = ?;";
};

#endif