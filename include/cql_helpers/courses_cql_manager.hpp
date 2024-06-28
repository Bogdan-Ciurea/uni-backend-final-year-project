/**
 * @file courses_cql_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the course object
 * @version 0.1
 * @date 2023-01-02
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef COURSES_CQL_MANAGER_H
#define COURSES_CQL_MANAGER_H

#include "../database_objects/course_object.hpp"
#include "cql_client.hpp"

class CoursesCqlManager {
 public:
  /**
   * @brief Construct a new Courses Cql Manager object
   *
   * @param cql_client - The cassandra cql client
   */
  CoursesCqlManager(CqlClient* cql_client) : _cql_client(cql_client) {}

  /**
   * @brief Destroy the Courses Cql Manager object
   *
   */
  ~CoursesCqlManager() {}

  /**
   * @brief Will prepare the database to be operated on
   *
   * @param init_db_schema - If you want to also create the keyspace and table
   *
   * @return CqlResult     - The result of the operation
   */
  CqlResult configure(bool init_db_schema);

  /**
   * @brief Adds a course entry on the database
   *
   * @param course_data - The course object we want to add
   *
   * @return CqlResult  - The result of the operation
   */
  CqlResult create_course(const CourseObject& course_data);

  /**
   * @brief Get the course object
   *
   * @param school_id  - The id of the school
   * @param id         - The uuid of the course
   *
   * @return std::pair<CqlResult, CourseObject>
   */
  std::pair<CqlResult, CourseObject> get_course_by_id(const int& school_id,
                                                      const CassUuid& id);

  /**
   * @brief Get all courses from a school
   *
   * @param school_id  - The id of the school
   *
   * @return std::pair<CqlResult, std::vector<CourseObject>>
   */
  std::pair<CqlResult, std::vector<CourseObject>> get_courses_by_school(
      const int& school_id);

  /**
   * @brief Updates a course
   *
   * @param school_id   - The id of the school
   * @param id          - The uuid of the course
   * @param name        - The new name of the course
   * @param thumbnail   - The new thumbnail of the course
   * @param updated_at  - The new updated at of the course
   * @param start_date  - The new start date of the course
   * @param end_date    - The new end date of the course
   * @param files       - The new files of the course
   */
  CqlResult update_course(const int& school_id, const CassUuid& id,
                          const std::string& name, const std::string& thumbnail,
                          const time_t& updated_at, const time_t& start_date,
                          const time_t& end_date,
                          const std::vector<CassUuid>& files);

  /**
   * @brief Deletes a course by id
   *
   * @param school_id   - The id of the school
   * @param id          - The uuid of the course
   * @return CqlResult  - The result of the operation
   */
  CqlResult delete_course_by_id(const int& school_id, const CassUuid& id);

 private:
  CqlClient* _cql_client;

  CassPreparedPtr _prepared_insert_course;

  CassPreparedPtr _prepared_get_course;

  CassPreparedPtr _prepared_get_courses_by_school;

  CassPreparedPtr _prepared_update_course;

  CassPreparedPtr _prepared_delete_course;

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

  const char* CREATE_COURSES_TABLE =
      "CREATE TABLE IF NOT EXISTS schools.courses ( "
      "school int, "
      "id uuid, "
      "name varchar, "
      "course_thumbnail varchar, "
      "created_at timestamp, "
      "start_date timestamp, "
      "end_date timestamp, "
      "files list <uuid>, "
      "PRIMARY KEY ((school, id)));";

  const char* INSERT_COURSE =
      "INSERT INTO schools.courses (school, id, name, course_thumbnail, "
      "created_at, start_date, end_date, files) VALUES (?, ?, ?, ?, ?, ?, ?, "
      "?) IF NOT EXISTS;";

  const char* SELECT_COURSE =
      "SELECT school, id, name, course_thumbnail, created_at, start_date, "
      "end_date, files FROM schools.courses WHERE school = ? AND id = ?;";

  const char* SELECT_COURSES_BY_SCHOOL =
      "SELECT school, id, name, course_thumbnail, created_at, start_date, "
      "end_date, files FROM schools.courses WHERE school = ? ALLOW FILTERING;";

  const char* UPDATE_COURSE =
      "UPDATE schools.courses SET name = ?, course_thumbnail = ?, created_at = "
      "?, start_date = "
      "?, end_date = ?, files = ? WHERE school = ? AND id = ? IF EXISTS;";

  const char* DELETE_COURSE =
      "DELETE FROM schools.courses WHERE school = ? AND id = ? IF EXISTS;";
};

CqlResult map_row_to_course(const CassRow* row, CourseObject& course);

#endif  // COURSES_CQL_MANAGER_H