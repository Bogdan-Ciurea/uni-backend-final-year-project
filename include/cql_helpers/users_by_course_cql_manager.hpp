/**
 * @file users_by_course_cql_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the relationship a course and multiple users
 * @version 0.1
 * @date 2023-01-06
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef USERS_BY_COURSE_CQL_MANAGER_H
#define USERS_BY_COURSE_CQL_MANAGER_H

#include "cql_client.hpp"

class UsersByCourseCqlManager {
 public:
  /**
   * @brief Construct a new Users By Course Cql Manager object
   *
   * @param cql_client - The CqlClient object
   */
  UsersByCourseCqlManager(CqlClient* cql_client) : _cql_client(cql_client) {}

  /**
   * @brief Destroy the Users By Course Cql Manager object
   *
   */
  ~UsersByCourseCqlManager() {}

  /**
   * @brief Configures the class
   *
   * @param init_db_schema - If true, the keyspace and table will be created
   * @return CqlResult - The result of the operation
   */
  CqlResult configure(bool init_db_schema);

  /**
   * @brief Creates a relationship between a course and a user
   *
   * @param school_id - The id of the school
   * @param course_id - The id of the course
   * @param user_id - The id of the user
   *
   * @return CqlResult - The result of the operation
   */
  CqlResult create_relationship(const int school_id, const CassUuid& course_id,
                                const CassUuid& user_id);

  /**
   * @brief Get the users by course
   *
   * @param school_id - The id of the school
   * @param course_id - The id of the course
   * @return std::pair<CqlResult, std::vector<CassUuid>> - The result of the
   * operation and the vector of user ids
   */
  std::pair<CqlResult, std::vector<CassUuid>> get_users_by_course(
      const int school_id, const CassUuid& course_id);

  /**
   * @brief Deletes a relationship between a course and a user
   *
   * @param school_id - The id of the school
   * @param course_id - The id of the course
   * @param user_id - The id of the user
   *
   * @return CqlResult - The result of the operation
   */
  CqlResult delete_relationship(const int school_id, const CassUuid& course_id,
                                const CassUuid& user_id);

  /**
   * @brief Deletes all relationships between a course and users
   *
   * @param school_id - The id of the school
   * @param course_id - The id of the course
   *
   * @return CqlResult - The result of the operation
   */
  CqlResult delete_relationships_by_course(const int school_id,
                                           const CassUuid& course_id);

 private:
  CqlClient* _cql_client;

  CassPreparedPtr _prepared_insert_relationship;

  CassPreparedPtr _prepared_get_users_by_course;

  CassPreparedPtr _prepared_delete_relationship;

  CassPreparedPtr _prepared_delete_relationships_by_course;

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

  const char* CREATE_USERS_BY_COURSE_TABLE =
      "CREATE TABLE IF NOT EXISTS schools.users_by_course ( "
      "school int, "
      "course_id uuid, "
      "user_id uuid, "
      "PRIMARY KEY ((school, course_id), user_id));";

  const char* INSERT_RELATIONSHIP =
      "INSERT INTO schools.users_by_course (school, course_id, user_id) VALUES "
      "(?, ?, ?) IF NOT EXISTS;";

  const char* GET_USERS_BY_COURSE =
      "SELECT user_id FROM schools.users_by_course WHERE school = ? AND "
      "course_id = ?;";

  const char* DELETE_RELATIONSHIP =
      "DELETE FROM schools.users_by_course WHERE school = ? AND course_id = ? "
      "AND user_id = ? IF EXISTS;";

  const char* DELETE_RELATIONSHIPS_BY_COURSE =
      "DELETE FROM schools.users_by_course WHERE school = ? AND course_id = ?;";
};

#endif