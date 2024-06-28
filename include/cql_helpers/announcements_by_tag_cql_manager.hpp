/**
 * @file announcements_by_tag_cql_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the relationship between announcements and tags
 * @version 0.1
 * @date 2023-01-01
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef ANNOUNCEMENTS_BY_TAG_CQL_MANAGER_H
#define ANNOUNCEMENTS_BY_TAG_CQL_MANAGER_H

#include "cql_client.hpp"

class AnnouncementsByTagCqlManager {
 public:
  AnnouncementsByTagCqlManager(CqlClient* cql_client)
      : _cql_client(cql_client) {}
  ~AnnouncementsByTagCqlManager() {}

  /**
   * @brief Configures the class
   *
   * @param init_db_schema - If true, the keyspace and table will be created
   * @return CqlResult - The result of the operation
   */
  CqlResult configure(bool init_db_schema);

  /**
   * @brief Creates a relationship between an announcement and a tag
   *
   * @param school_id - The id of the school
   * @param tag_id - The id of the tag
   * @param announcement_id - The id of the announcement
   * @return CqlResult - The result of the operation
   */
  CqlResult create_relationship(const int school_id, const CassUuid& tag_id,
                                const CassUuid& announcement_id);

  /**
   * @brief Get the announcements by tag
   *
   * @param school_id - The id of the school
   * @param tag_id - The id of the tag
   * @return std::pair<CqlResult, std::vector<CassUuid>> - The result of the
   * operation and the vector of announcement ids
   */
  std::pair<CqlResult, std::vector<CassUuid>> get_announcements_by_tag(
      const int school_id, const CassUuid& tag_id);

  /**
   * @brief Get the tags by announcement
   *
   * @param school_id - The id of the school
   * @param announcement_id - The id of the announcement
   * @return std::pair<CqlResult, std::vector<CassUuid>> - The result of the
   * operation and the vector of tags ids
   */
  std::pair<CqlResult, std::vector<CassUuid>> get_tags_by_announcement(
      const int school_id, const CassUuid& announcement_id);

  /**
   * @brief Deletes a relationship between an announcement and a tag
   *
   * @param school_id - The id of the school
   * @param tag_id - The id of the tag
   * @param announcement_id - The id of the announcement
   * @return CqlResult - The result of the operation
   */
  CqlResult delete_relationship(const int school_id, const CassUuid& tag_id,
                                const CassUuid& announcement_id);

  /**
   * @brief Deletes all relationships of a tag
   *
   * @param school_id - The id of the school
   * @param tag_id - The id of the tag
   * @return CqlResult - The result of the operation
   */
  CqlResult delete_relationships_by_tag(const int school_id,
                                        const CassUuid& tag_id);

 private:
  CqlClient* _cql_client;

  CassPreparedPtr _prepared_insert_relationship;

  CassPreparedPtr _prepared_get_announcements_by_tag;

  CassPreparedPtr _prepared_get_tags_by_announcement;

  CassPreparedPtr _prepared_delete_relationship;

  CassPreparedPtr _prepared_delete_relationships_by_tag;

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

  const char* CREATE_ANNOUNCEMENTS_BY_TAG_TABLE =
      "CREATE TABLE IF NOT EXISTS schools.announcements_by_tag ( "
      "school int, "
      "tag_id uuid, "
      "announcement_id uuid, "
      "PRIMARY KEY ((school, tag_id), announcement_id));";

  const char* INSERT_RELATIONSHIP =
      "INSERT INTO schools.announcements_by_tag (school, tag_id , "
      "announcement_id) VALUES (?, ?, ?) IF NOT EXISTS;";

  const char* SELECT_ANNOUNCEMENTS =
      "SELECT announcement_id FROM schools.announcements_by_tag WHERE school = "
      "? AND tag_id = ?;";

  const char* SELECT_TAGS_BY_ANNOUNCEMENT =
      "SELECT tag_id FROM schools.announcements_by_tag WHERE school = ? AND "
      "announcement_id = ? ALLOW FILTERING;";

  // Will delete a specific relationship. Would be used if you delete an
  // announcement
  const char* DELETE_RELATIONSHIP =
      "DELETE FROM schools.announcements_by_tag WHERE school = ? AND tag_id = "
      "? AND announcement_id = ? IF EXISTS;";

  // Will delete all relationships related to a tag. Would be used if you delete
  // a tag
  const char* DELETE_RELATIONSHiPS_BY_TAG =
      "DELETE FROM schools.announcements_by_tag WHERE school = ? AND tag_id = "
      "?;";
};

#endif  // ANNOUNCEMENTS_BY_TAG_CQL_MANAGER_H
