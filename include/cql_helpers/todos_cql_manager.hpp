/**
 * @file todos_cql_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the todos object
 * @version 0.1
 * @date 2023-01-07
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef TODOS_CQL_MANAGER_H
#define TODOS_CQL_MANAGER_H

#include "cql_client.hpp"
#include "database_objects/todo_object.hpp"

class TodosCqlManager {
 public:
  /**
   * @brief Construct a new Todos Cql Manager object
   *
   * @param cql_client - The cassandra cql client
   */
  TodosCqlManager(CqlClient* cql_client) : _cql_client(cql_client) {}

  /**
   * @brief Destroy the Todos Cql Manager object
   *
   */
  ~TodosCqlManager(){};

  /**
   * @brief Will prepare the database to be operated on
   *
   * @param init_db_schema - If you want to also create the keyspace and table
   *
   * @return CqlResult     - The result of the operation
   */
  CqlResult configure(bool init_db_schema);

  /**
   * @brief Adds a todo entry on the database
   *
   * @param todo_data   - The todo object we want to add
   *
   * @return CqlResult  - The result of the operation
   */
  CqlResult create_todo(const TodoObject& todo_data);

  /**
   * @brief Get a todo object by an id
   *
   * @param school_id  - The id of the school
   * @param todo_id    - The id of the todo
   *
   * @return std::pair<CqlResult, TodoObject> - The result of the operation and
   * the todo object
   */
  std::pair<CqlResult, TodoObject> get_todo_by_id(const int school_id,
                                                  const CassUuid& todo_id);

  /**
   * @brief Updates a todo object
   *
   * @param school_id  - The id of the school
   * @param todo_id    - The id of the todo
   * @param text       - The new text of the todo
   * @param type       - The new status of the todo
   *
   * @return CqlResult - The result of the operation
   */
  CqlResult update_todo(const int school_id, const CassUuid& todo_id,
                        const std::string& text, const TodoType type);

  /**
   * @brief Deletes a todo object
   *
   * @param school_id  - The id of the school
   * @param todo_id    - The id of the todo
   *
   * @return CqlResult - The result of the operation
   */
  CqlResult delete_todo(const int school_id, const CassUuid& todo_id);

 private:
  CqlClient* _cql_client;

  CassPreparedPtr _prepared_insert_todo;

  CassPreparedPtr _prepared_get_todo_by_id;

  CassPreparedPtr _prepared_update_todo;

  CassPreparedPtr _prepared_delete_todo;

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

  const char* CREATE_TODOS_TABLE =
      "CREATE TABLE IF NOT EXISTS schools.todos( "
      "school int, "
      "id uuid, "
      "text text, "
      "type int, "
      "PRIMARY KEY ((school, id)));";

  const char* INSERT_TODO =
      "INSERT INTO schools.todos (school, id, text, type) VALUES (?, ?, ?, ?) "
      "IF NOT EXISTS ;";

  const char* SELECT_TODO_BY_ID =
      "SELECT school, id, text, type FROM schools.todos WHERE school = ? AND "
      "id = ?;";

  const char* UPDATE_TODO =
      "UPDATE schools.todos SET text = ?, type = ? WHERE school = ? AND id = ? "
      "IF EXISTS;";

  const char* DELETE_TODO =
      "DELETE FROM schools.todos WHERE school = ? AND id = ? IF EXISTS;";
};

CqlResult map_row_to_todo(const CassRow* row, TodoObject& todo);

#endif