/**
 * @file questions_cql_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the question object
 * @version 0.1
 * @date 2023-01-07
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef QUESTIONS_CQL_MANAGER_H
#define QUESTIONS_CQL_MANAGER_H

#include "cql_client.hpp"
#include "database_objects/question_object.hpp"

class QuestionsCqlManager {
 public:
  /**
   * @brief Construct a new Questions Cql Manager object
   *
   * @param cql_client - The cassandra cql client
   */
  QuestionsCqlManager(CqlClient* cql_client) : _cql_client(cql_client) {}

  /**
   * @brief Destroy the Questions Cql Manager object
   *
   */
  ~QuestionsCqlManager(){};

  /**
   * @brief Will prepare the database to be operated on
   *
   * @param init_db_schema - If you want to also create the keyspace and table
   *
   * @return CqlResult     - The result of the operation
   */
  CqlResult configure(bool init_db_schema);

  /**
   * @brief Adds a question entry on the database
   *
   * @param question_data - The question object we want to add
   *
   * @return CqlResult    - The result of the operation
   */
  CqlResult create_question(const QuestionObject& question_data);

  /**
   * @brief Get the question object
   *
   * @param school_id   - The id of the school
   * @param question_id - The id of the question
   *
   * @return std::pair<CqlResult, QuestionObject>
   */
  std::pair<CqlResult, QuestionObject> get_question_by_id(
      const int& school_id, const CassUuid& question_id);

  /**
   * @brief Updates a question entry on the database
   *
   * @param school_id    - The id of the school
   * @param question_id  - The id of the question
   * @param text         - The new text of the question
   * @param time_changed - The new time of the question
   * @param changed_by_user - The user that changed the question
   *
   * @return CqlResult   - The result of the operation
   */
  CqlResult update_question(const int& school_id, const CassUuid& question_id,
                            const std::string& text, const time_t& time_changed,
                            const CassUuid& changed_by_user);

  /**
   * @brief Deletes a question entry on the database
   *
   * @param school_id   - The id of the school
   * @param question_id - The id of the question
   *
   * @return CqlResult  - The result of the operation
   */
  CqlResult delete_question(const int& school_id, const CassUuid& question_id);

 private:
  CqlClient* _cql_client;

  CassPreparedPtr _prepared_insert_question;

  CassPreparedPtr _prepared_get_question;

  CassPreparedPtr _prepared_update_question;

  CassPreparedPtr _prepared_delete_question;

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

  const char* CREATE_QUESTIONS_TABLE =
      "CREATE TABLE IF NOT EXISTS schools.questions ( "
      "school int, "
      "id uuid, "
      "text text, "
      "time_added timestamp, "
      "added_by_user_id uuid, "
      "PRIMARY KEY ((school, id)));";

  const char* INSERT_QUESTION =
      "INSERT INTO schools.questions (school, id, text, time_added, "
      "added_by_user_id) VALUES (?, ?, ?, ?, ?) IF NOT EXISTS ;";

  const char* SELECT_QUESTION =
      "SELECT school, id, text, time_added, added_by_user_id FROM  "
      "schools.questions "
      "WHERE school = ? AND id = ?;";

  const char* UPDATE_QUESTION =
      "UPDATE schools.questions SET text = ?, time_added = ?, added_by_user_id "
      "= ? WHERE school = ? AND id = ? IF EXISTS;";

  const char* DELETE_QUESTION =
      "DELETE FROM schools.questions WHERE school = ? AND id = ? IF EXISTS;";
};

CqlResult map_row_to_question(const CassRow* row, QuestionObject& question);

#endif