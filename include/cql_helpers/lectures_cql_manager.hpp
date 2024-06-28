/**
 * @file lectures_cql_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the lecture object
 * @version 0.1
 * @date 2023-01-06
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef LECTURES_CQL_MANAGER_H
#define LECTURES_CQL_MANAGER_H

#include "cql_client.hpp"
#include "database_objects/lecture_object.hpp"

class LecturesCqlManager {
 public:
  /**
   * @brief Construct a new Lectures Cql Manager object
   *
   * @param cql_client - The cassandra cql client
   */
  LecturesCqlManager(CqlClient* cql_client) : _cql_client(cql_client) {}

  /**
   * @brief Destroy the Lectures Cql Manager object
   *
   */
  ~LecturesCqlManager(){};

  /**
   * @brief Will prepare the database to be operated on
   *
   * @param init_db_schema - If you want to also create the keyspace and table
   *
   * @return CqlResult     - The result of the operation
   */
  CqlResult configure(bool init_db_schema);

  /**
   * @brief Adds a lecture entry on the database
   *
   * @param lecture_data - The lecture object we want to add
   *
   * @return CqlResult   - The result of the operation
   */
  CqlResult create_lecture(const LectureObject& lecture_data);

  /**
   * @brief Get the lectures object related to a course
   *
   * @param school_id  - The id of the school
   * @param course_id  - The id of the course
   *
   * @return std::pair<CqlResult, std::vector<LectureObject>>
   */
  std::pair<CqlResult, std::vector<LectureObject>> get_lectures_by_course(
      const int& school_id, const CassUuid& course_id);

  /**
   * @brief Updates a lecture entry on the database.
   * Considering that we cannot update a primary key, we will delete the old
   * entry and add a new one
   *
   * @param school_id - The id of the school
   * @param course_id - The id of the course
   * @param original_starting_time - The original starting time of the lecture
   * @param new_starting_time - The new starting time of the lecture
   * @param duration - The duration of the lecture
   * @param location - The location of the lecture
   *
   * @return CqlResult   - The result of the operation
   */
  CqlResult update_lecture(const int& school_id, const CassUuid& course_id,
                           const time_t& original_starting_time,
                           const time_t& new_starting_time, const int& duration,
                           const std::string& location);

  /**
   * @brief Deletes a lecture entry on the database
   *
   * @param school_id - The id of the school
   * @param course_id - The id of the course
   * @param starting_time - The starting time of the lecture
   *
   * @return CqlResult   - The result of the operation
   */
  CqlResult delete_lecture(const int& school_id, const CassUuid& course_id,
                           const time_t& starting_time);

  /**
   * @brief Deletes all the lectures related to a course
   *
   * @param school_id - The id of the school
   * @param course_id - The id of the course
   *
   * @return CqlResult   - The result of the operation
   */
  CqlResult delete_lectures_by_course(const int& school_id,
                                      const CassUuid& course_id);

 private:
  CqlClient* _cql_client;

  CassPreparedPtr _prepared_insert_lecture;

  CassPreparedPtr _prepared_get_lectures_by_course;

  CassPreparedPtr _prepared_delete_lecture;

  CassPreparedPtr _prepared_delete_lectures_by_course;

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

  const char* CREATE_LECTURES_TABLE =
      "CREATE TABLE IF NOT EXISTS schools.lectures ( "
      "school int, "
      "course_id uuid, "
      "starting_time timestamp, "
      "duration int, "
      "location text, "
      "PRIMARY KEY ((school, course_id), starting_time));";

  const char* INSERT_LECTURE =
      "INSERT INTO schools.lectures (school, course_id, starting_time, "
      "duration, location) VALUES (?, ?, ?, ?, ?) IF NOT EXISTS;";

  const char* SELECT_LECTURES_BY_COURSE =
      "SELECT school, course_id, starting_time, duration, location FROM "
      "schools.lectures WHERE school = ? AND course_id = ?;";

  const char* DELETE_LECTURE =
      "DELETE FROM schools.lectures WHERE school = ? AND course_id = ? AND "
      "starting_time = ? IF EXISTS;";

  const char* DELETE_LECTURES_BY_COURSE =
      "DELETE FROM schools.lectures WHERE school = ? AND course_id = ?;";
};

CqlResult map_row_to_lecture(const CassRow* row, LectureObject& lecture);

#endif