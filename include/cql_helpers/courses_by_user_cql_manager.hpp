/**
 * @file courses_by_user_cql_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the courses by user relationship
 * @version 0.1
 * @date 2023-01-03
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef COURSES_BY_USER_CQL_MANAGER_H
#define COURSES_BY_USER_CQL_MANAGER_H

#include "cql_client.hpp"

class CoursesByUserCqlManager {
 public:
  CoursesByUserCqlManager(CqlClient* cql_client) : _cql_client(cql_client){};
  ~CoursesByUserCqlManager(){};

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
   * @param user_id   - The id of the user
   * @param course_id - The id of the course
   * @return CqlResult - The result of the operation
   */
  CqlResult create_relationship(const int school_id, const CassUuid& user_id,
                                const CassUuid& course_id);

  /**
   * @brief Get the courses by user
   *
   * @param school_id - The id of the school
   * @param user_id   - The id of the user
   * @return std::pair<CqlResult, std::vector<CassUuid>> - The result of the
   * operation and the vector of course ids
   */
  std::pair<CqlResult, std::vector<CassUuid>> get_courses_by_user(
      const int school_id, const CassUuid& user_id);

  /**
   * @brief Deletes all relationships between an user and all courses that he is
   * assigned to
   *
   * @param school_id - The id of the school
   * @param user_id   - The id of the user
   *
   * @return CqlResult - The result of the operation
   *
   */
  CqlResult delete_all_relationships_of_user(const int& school_id,
                                             const CassUuid& user_id);

  /**
   * @brief Deletes a relationship between an user and a course
   *
   * @param school_id - The id of the school
   * @param user_id   - The id of the user
   * @param course_id - The id of the course
   *
   * @return CqlResult - The result of the operation
   */
  CqlResult delete_relationship(const int& school_id, const CassUuid& user_id,
                                const CassUuid& course_id);

 private:
  CqlClient* _cql_client;

  CassPreparedPtr _prepared_insert_relationship;

  CassPreparedPtr _prepared_get_courses_by_user;

  CassPreparedPtr _prepared_delete_all_relationships_of_user;

  CassPreparedPtr _prepared_delete_relationship;

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

  const char* CREATE_COURSES_BY_USER_TABLE =
      "CREATE TABLE IF NOT EXISTS schools.courses_by_user ( "
      "school int, "
      "user_id uuid, "
      "course_id uuid, "
      "PRIMARY KEY ((school, user_id), course_id));";

  const char* INSERT_RELATIONSHIP =
      "INSERT INTO schools.courses_by_user (school, user_id, course_id ) "
      "VALUES (?, ?, ?) IF NOT EXISTS;";

  const char* SELECT_COURSES_BY_USER =
      "SELECT course_id  FROM schools.courses_by_user WHERE school = ? AND "
      "user_id = ?;";

  const char* DELETE_ALL_RELATIONSHIPS_OF_USER =
      "DELETE FROM schools.courses_by_user WHERE school = ? AND user_id = ?;";

  const char* DELETE_RELATIONSHIP =
      "DELETE FROM schools.courses_by_user WHERE school = ? AND user_id = ? "
      "AND course_id = ? IF EXISTS;";
};

#endif