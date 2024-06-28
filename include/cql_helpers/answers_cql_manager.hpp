/**
 * @file answers_cql_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the answer object
 * @version 0.1
 * @date 2022-12-31
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef ANSWERS_CQL_MANAGER_H
#define ANSWERS_CQL_MANAGER_H

#include "cql_client.hpp"
#include "database_objects/answer_object.hpp"

class AnswersCqlManager {
 public:
  /**
   * @brief Construct a new Answers Cql Manager object
   *
   * @param cql_client - The cassandra cql client
   */
  AnswersCqlManager(CqlClient* cql_client) : _cql_client(cql_client) {}

  /**
   * @brief Destroy the Answers Cql Manager object
   *
   */
  ~AnswersCqlManager() {}

  /**
   * @brief Will prepare the database to be operated on
   *
   * @param init_db_schema - If you want to also create the keyspace and table
   *
   * @return CqlResult     - The result of the operation
   */
  CqlResult configure(bool init_db_schema);

  /**
   * @brief Adds a answer entry on the database
   *
   * @param answer_data - The answer object we want to add
   *
   * @return CqlResult   - The result of the operation
   */
  CqlResult create_answer(const AnswerObject& answer_data);

  /**
   * @brief Get the answer object
   *
   * @param school_id  - The id of the school
   * @param id         - The uuid of the answer
   *
   * @return std::pair<CqlResult, AnswerObject>
   */
  std::pair<CqlResult, AnswerObject> get_answer_by_id(const int school_id,
                                                      const CassUuid& id);

  /**
   * @brief Updates the answer object
   *
   * @param school_id  - The id of the school
   * @param id         - The uuid of the answer
   * @param created_at - The new created at value
   * @param updated_by - The new user that updated the answer
   * @param content    - The new content value
   */
  CqlResult update_answer(const int school_id, const CassUuid& id,
                          const time_t& created_at, const CassUuid& updated_by,
                          const std::string& content);

  /**
   * @brief Deletes the answer object
   *
   * @param school_id  - The id of the school
   * @param id         - The uuid of the answer
   * @param created_at - The created at value
   *
   */
  CqlResult delete_answer(const int school_id, const CassUuid& id,
                          const time_t& created_at);

 private:
  CqlClient* _cql_client;

  CassPreparedPtr _prepared_insert_answer;

  CassPreparedPtr _prepared_get_answer;

  CassPreparedPtr _prepared_update_answer;

  CassPreparedPtr _prepared_delete_answer;

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

  const char* CREATE_ANSWERS_TABLE =
      "CREATE TABLE IF NOT EXISTS schools.answers ( "
      "school int, "
      "id uuid, "
      "created_at timestamp, "
      "created_by uuid, "
      "content text, "
      "PRIMARY KEY ((school, id), created_at))"
      "WITH CLUSTERING ORDER BY (created_at DESC);";

  const char* INSERT_ANSWER =
      "INSERT INTO schools.answers (school, id, created_at, created_by, "
      "content ) VALUES (?, ?, ?, ?, ?) IF NOT EXISTS;";

  const char* SELECT_ANSWER =
      "SELECT school, id, created_at, created_by, content FROM schools.answers "
      "WHERE school = ? AND id = ?;";

  const char* UPDATE_ANSWER =
      "UPDATE schools.answers SET created_by = ?, content = ? WHERE school = ? "
      "AND id = ? AND created_at = ? IF EXISTS;";

  const char* DELETE_ANSWER =
      "DELETE FROM schools.answers WHERE school = ? AND id = ? AND created_at "
      "= ? IF EXISTS;";
};

CqlResult map_row_to_answer(const CassRow* row, AnswerObject& answer);

#endif  // ANSWERS_CQL_MANAGER_H