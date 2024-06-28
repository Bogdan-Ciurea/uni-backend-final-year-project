/**
 * @file grades_cql_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the grade object
 * @version 0.1
 * @date 2023-01-03
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef GRADES_CQL_MANAGER_H
#define GRADES_CQL_MANAGER_H

#include "../database_objects/grade_object.hpp"
#include "cql_client.hpp"

class GradesCqlManager {
 public:
  /**
   * @brief Construct a new Files Cql Manager object
   *
   * @param cql_client - The cassandra cql client
   */
  GradesCqlManager(CqlClient* cql_client) : _cql_client(cql_client) {}

  /**
   * @brief Destroy the Files Cql Manager object
   *
   */
  ~GradesCqlManager() {}

  /**
   * @brief Will prepare the database to be operated on
   *
   * @param init_db_schema - If you want to also create the keyspace and table
   *
   * @return CqlResult     - The result of the operation
   */
  CqlResult configure(bool init_db_schema);

  /**
   * @brief Adds a grade entry on the database
   *
   * @param grade_data - The grade object we want to add
   *
   * @return CqlResult   - The result of the operation
   */
  CqlResult create_grade(const GradeObject& grade_data);

  /**
   * @brief Get the grade object
   *
   * @param school_id  - The id of the school
   * @param id         - The uuid of the grade
   *
   * @return std::pair<CqlResult, GradeObject>
   */
  std::pair<CqlResult, GradeObject> get_grade_by_id(const int& school_id,
                                                    const CassUuid& id);

  /**
   * @brief Get all grades of an evaluated user (student)
   *
   * @param school_id  - The id of the school
   * @param student_id - The uuid of the student
   *
   * @return std::pair<CqlResult, std::vector<GradeObject>>
   */
  std::pair<CqlResult, std::vector<GradeObject>> get_grades_by_student_id(
      const int& school_id, const CassUuid& student_id);

  /**
   * @brief Get all grades assigned by an evaluator
   *
   * @param school_id    - The id of the school
   * @param evaluator_id - The uuid of the evaluator
   *
   * @return std::pair<CqlResult, std::vector<GradeObject>>
   */
  std::pair<CqlResult, std::vector<GradeObject>> get_grades_by_evaluator_id(
      const int& school_id, const CassUuid& evaluator_id);

  /**
   * @brief Get all grades by subject
   *
   * @param school_id  - The id of the school
   * @param course_id  - The uuid of the course
   *
   * @return std::pair<CqlResult, std::vector<GradeObject>>
   */
  std::pair<CqlResult, std::vector<GradeObject>> get_grades_by_course_id(
      const int& school_id, const CassUuid& course_id);

  /**
   * @brief Get all grades by evaluated and course
   *
   * @param school_id  - The id of the school
   * @param student_id - The uuid of the student
   * @param course_id  - The uuid of the course
   *
   * @return std::pair<CqlResult, std::vector<GradeObject>>
   */
  std::pair<CqlResult, std::vector<GradeObject>> get_grades_by_user_and_course(
      const int& school_id, const CassUuid& student_id,
      const CassUuid& course_id);

  /**
   * @brief Updates the grade object
   *
   * @param school_id  - The id of the school
   * @param id         - The uuid of the grade
   * @param value      - The new value of the grade
   * @param out_of     - The new out of value of the grade
   * @param date       - The new date of the grade
   * @param weight     - The new weight of the grade
   *
   * @return CqlResult   - The result of the operation
   */
  CqlResult update_grade(const int& school_id, const CassUuid& id,
                         const int& value, const int& out_of,
                         const time_t& date, const float& weight);

  /**
   * @brief Deletes a grade entry from the database
   *
   * @param school_id  - The id of the school
   * @param id         - The uuid of the grade
   *
   * @return CqlResult   - The result of the operation
   */
  CqlResult delete_grade(const int& school_id, const CassUuid& id);

 private:
  CqlClient* _cql_client;

  CassPreparedPtr _prepared_insert_grade;

  CassPreparedPtr _prepared_get_grade_by_id;

  CassPreparedPtr _prepared_get_grades_by_evaluated;

  CassPreparedPtr _prepared_get_grades_by_evaluator;

  CassPreparedPtr _prepared_get_grades_by_course;

  CassPreparedPtr _prepared_get_grades_by_evaluated_and_course;

  CassPreparedPtr _prepared_update_grade;

  CassPreparedPtr _prepared_delete_grade;

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

  const char* CREATE_GRADES_TABLE =
      "CREATE TABLE IF NOT EXISTS schools.grades ( "
      "school int, "
      "id uuid, "
      "evaluated_id uuid, "
      "evaluator_id uuid, "
      "course_id uuid, "
      "value int, "
      "out_of int, "
      "created_at timestamp, "
      "weight float, "
      "PRIMARY KEY ((school, id)));";

  const char* INSERT_GRADE =
      "INSERT INTO schools.grades (school, id, evaluated_id, evaluator_id, "
      "course_id, value, out_of, created_at, weight) VALUES (?, ?, ?, ?, ?, ?, "
      "?, ?, ?) IF NOT EXISTS;";

  const char* SELECT_GRADE_BY_ID =
      "SELECT school, id, evaluated_id, evaluator_id, course_id, value, "
      "out_of, created_at, weight FROM schools.grades WHERE school = ? AND id "
      "= ?;";

  const char* SELECT_GRADES_BY_STUDENT =
      "SELECT school, id, evaluated_id, evaluator_id, course_id, value, "
      "out_of, created_at, weight FROM schools.grades WHERE school = ? AND "
      "evaluated_id "
      "= ? ALLOW FILTERING;";

  const char* SELECT_GRADES_BY_EVALUATOR =
      "SELECT school, id, evaluated_id, evaluator_id, course_id, value, "
      "out_of, created_at, weight FROM schools.grades WHERE school = ? AND "
      "evaluator_id "
      "= ? ALLOW FILTERING;";

  const char* SELECT_GRADES_BY_COURSE =
      "SELECT school, id, evaluated_id, evaluator_id, course_id, value, "
      "out_of, created_at, weight FROM schools.grades WHERE school = ? AND "
      "course_id "
      "= ? ALLOW FILTERING;";

  const char* SELECT_GRADES_BY_EVALUATED_AND_COURSE =
      "SELECT school, id, evaluated_id, evaluator_id, course_id, value, "
      "out_of, created_at, weight FROM schools.grades WHERE school = ? AND "
      "evaluated_id = ? AND course_id = ? ALLOW FILTERING;";

  const char* UPDATE_GRADE =
      "UPDATE schools.grades SET value = ?, out_of = ?, created_at = ?, weight "
      "= ? WHERE school = ? AND id = ? IF EXISTS;";

  const char* DELETE_GRADE =
      "DELETE FROM schools.grades WHERE school = ? AND id = ? IF EXISTS;";
};

CqlResult map_row_to_grade(const CassRow* row, GradeObject& grade_object);

#endif