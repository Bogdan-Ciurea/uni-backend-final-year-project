/**
 * @file questions_by_course_cql_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the relationship between questions and courses
 * @version 0.1
 * @date 2023-01-07
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef QUESTIONS_BY_COURSE_CQL_MANAGER_H
#define QUESTIONS_BY_COURSE_CQL_MANAGER_H

#include "cql_client.hpp"

class QuestionsByCourseCqlManager {
 public:
  /**
   * @brief Construct a new Questions By Tag Cql Manager object
   *
   * @param cql_client - The CqlClient object
   */
  QuestionsByCourseCqlManager(CqlClient* cql_client)
      : _cql_client(cql_client) {}

  /**
   * @brief Destroy the Questions By Tag Cql Manager object
   *
   */
  ~QuestionsByCourseCqlManager(){};

  /**
   * @brief Configures the class
   *
   * @param init_db_schema - If true, the keyspace and table will be created
   * @return CqlResult - The result of the operation
   */
  CqlResult configure(bool init_db_schema);

  /**
   * @brief Creates a relationship between a question and a course
   *
   * @param school_id   - The id of the school
   * @param course_id   - The id of the tag
   * @param question_id - The id of the question
   *
   * @return CqlResult  - The result of the operation
   */
  CqlResult create_relationship(const int school_id, const CassUuid& tag_id,
                                const CassUuid& question_id);

  /**
   * @brief Get the questions by course
   *
   * @param school_id - The id of the school
   * @param course_id - The id of the course
   *
   * @return std::pair<CqlResult, std::vector<CassUuid>> - The result of the
   * operation and the vector of question ids
   */
  std::pair<CqlResult, std::vector<CassUuid>> get_questions_by_course(
      const int school_id, const CassUuid& course_id);

  /**
   * @brief Deletes a relationship between a question and a course
   *
   * @param school_id   - The id of the school
   * @param course_id   - The id of the course
   * @param question_id - The id of the question
   *
   * @return CqlResult  - The result of the operation
   */
  CqlResult delete_relationship(const int school_id, const CassUuid& tag_id,
                                const CassUuid& question_id);

  /**
   * @brief Deletes all relationships of a course
   *
   * @param school_id - The id of the school
   * @param course_id - The id of the course
   *
   * @return CqlResult - The result of the operation
   */
  CqlResult delete_relationships_by_course(const int school_id,
                                           const CassUuid& tag_id);

 private:
  CqlClient* _cql_client;

  CassPreparedPtr _prepared_insert_relationship;

  CassPreparedPtr _prepared_get_questions_by_course;

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

  const char* CREATE_QUESTIONS_BY_COURSE_TABLE =
      "CREATE TABLE IF NOT EXISTS schools.questions_by_course ( "
      "school int, "
      "course_id uuid, "
      "question_id uuid, "
      "PRIMARY KEY (school, course_id, question_id));";

  const char* INSERT_RELATIONSHIP =
      "INSERT INTO schools.questions_by_course (school, course_id, "
      "question_id) VALUES (?, ?, ?) IF NOT EXISTS;";

  const char* SELECT_QUESTIONS_BY_COURSE =
      "SELECT question_id FROM schools.questions_by_course WHERE "
      "school = ? AND course_id = ?;";

  const char* DELETE_RELATIONSHIP =
      "DELETE FROM schools.questions_by_course WHERE school = ? AND course_id "
      "= ? AND question_id = ? IF EXISTS;";

  const char* DELETE_RELATIONSHIPS_BY_COURSE =
      "DELETE FROM schools.questions_by_course WHERE school = ? AND course_id "
      "= ?;";
};

#endif