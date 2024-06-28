/**
 * @file tokens_cql_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the users tokens
 * @version 0.1
 * @date 2023-01-06
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef TOKENS_CQL_MANAGER_H
#define TOKENS_CQL_MANAGER_H

#include "cql_client.hpp"

class TokensCqlManager {
 public:
  /**
   * @brief Construct a new Tokens Cql Manager object
   *
   * @param cql_client - The cassandra cql client
   */
  TokensCqlManager(CqlClient* cql_client, const int ttl = 2592000)
      : _cql_client(cql_client), _time_to_live(ttl) {}

  /**
   * @brief Destroy the Tokens Cql Manager object
   *
   */
  ~TokensCqlManager(){};

  /**
   * @brief Will prepare the database to be operated on
   *
   * @param init_db_schema - If you want to also create the keyspace and table
   *
   * @return CqlResult     - The result of the operation
   */
  CqlResult configure(bool init_db_schema);

  /**
   * @brief Adds a token entry on the database
   *
   * @param school_id - The id of the school
   * @param token     - The token we want to add
   * @param user_id   - The id of the user
   *
   * @return CqlResult - The result of the operation
   */
  CqlResult create_token(const int& school_id, const std::string& token,
                         const CassUuid& user_id);

  /**
   * @brief Get user from token
   *
   * @param school_id - The id of the school
   * @param token     - The token we want to add
   *
   * @return std::pair<CqlResult, CassUuid> - The result of the operation and
   * the user id
   */
  std::pair<CqlResult, CassUuid> get_user_from_token(const int& school_id,
                                                     const std::string& token);

  /**
   * @brief Deletes a token entry on the database
   *
   * @param school_id - The id of the school
   * @param token     - The token we want to add
   *
   * @return CqlResult - The result of the operation
   */
  CqlResult delete_token(const int& school_id, const std::string& token);

 private:
  // The time to live for the token
  int _time_to_live;

  CqlClient* _cql_client;

  CassPreparedPtr _prepared_insert_token;

  CassPreparedPtr _prepared_get_user_from_token;

  CassPreparedPtr _prepared_delete_token;

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

  const char* CREATE_TOKENS_TABLE =
      "CREATE TABLE IF NOT EXISTS schools.tokens ( "
      "school int, "
      "value varchar, "
      "user_id uuid, "
      "PRIMARY KEY ((school, value)));";

  const char* INSERT_TOKEN =
      "INSERT INTO schools.tokens (school, value, user_id ) VALUES (?, ?, ?) "
      "IF NOT EXISTS USING TTL ?;";

  const char* GET_USER_FROM_TOKEN =
      "SELECT user_id FROM schools.tokens WHERE school = ? AND value = ?;";

  const char* DELETE_TOKEN =
      "DELETE FROM schools.tokens WHERE school = ? AND value = ? IF EXISTS;";
};

#endif