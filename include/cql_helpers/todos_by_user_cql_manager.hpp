/**
 * @file todos_by_user_cql_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the relationship between todos and users
 * @version 0.1
 * @date 2023-01-07
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef TODOS_BY_USER_HPP
#define TODOS_BY_USER_HPP

#include "cql_client.hpp"

class TodosByUserCqlManager {
 public:
  /**
   * @brief Construct a new Todos By User object
   *
   * @param cql_client
   */
  TodosByUserCqlManager(CqlClient* cql_client) : _cql_client(cql_client) {}

  /**
   * @brief Destroy the Todos By User object
   *
   */
  ~TodosByUserCqlManager() {}

  /**
   * @brief Configures the class
   *
   * @param init_db_schema - If true, the keyspace and table will be created
   * @return CqlResult - The result of the operation
   */
  CqlResult configure(bool init_db_schema);

  /**
   * @brief Creates a relationship between a user and a todo
   *
   * @param school_id    - The id of the school
   * @param user_id     - The id of the user
   * @param todo_id     - The id of the todo
   * @return CqlResult  - The result of the operation
   */
  CqlResult create_relationship(const int school_id, const CassUuid& user_id,
                                const CassUuid& todo_id);

  /**
   * @brief Get the todos by user
   *
   * @param school_id    - The id of the school
   * @param user_id      - The id of the user
   *
   * @return std::pair<CqlResult, std::vector<CassUuid>> - The result of the
   * operation and the vector of todo ids
   */
  std::pair<CqlResult, std::vector<CassUuid>> get_todos_by_user(
      const int school_id, const CassUuid& user_id);

  /**
   * @brief Deletes a relationship between an user and a todo
   *
   * @param school_id       - The id of the school
   * @param user_id         - The id of the user
   * @param todo_id         - The id of the todo
   *
   * @return CqlResult      - The result of the operation
   */
  CqlResult delete_relationship(const int school_id, const CassUuid& user_id,
                                const CassUuid& todo_id);

  /**
   * @brief Deletes all relationships of an user
   *
   * @param school_id  - The id of the school
   * @param user_id    - The id of the user
   * @return CqlResult - The result of the operation
   */
  CqlResult delete_relationships_by_user(const int school_id,
                                         const CassUuid& user_id);

 private:
  CqlClient* _cql_client;

  CassPreparedPtr _prepared_insert_relationship;

  CassPreparedPtr _prepared_get_todos_by_user;

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

  const char* CREATE_TODOS_BY_USER_TABLE =
      "CREATE TABLE IF NOT EXISTS schools.todos_by_user ( "
      "school int, "
      "user_id uuid, "
      "todo_id uuid, "
      "PRIMARY KEY ((school, user_id), todo_id));";

  const char* INSERT_RELATIONSHIP =
      "INSERT INTO schools.todos_by_user (school, user_id, todo_id) VALUES (?, "
      "?, ?) IF NOT EXISTS;";

  const char* SELECT_TODOS =
      "SELECT todo_id FROM schools.todos_by_user WHERE school = ? AND user_id "
      "= ?;";

  const char* DELETE_RELATIONSHIP =
      "DELETE FROM schools.todos_by_user WHERE school = ? AND user_id = ? AND "
      "todo_id "
      "= ? IF EXISTS;";

  const char* DELETE_RELATIONSHIPS_BY_USER =
      "DELETE FROM schools.todos_by_user WHERE school = ? AND user_id = ?;";
};

#endif