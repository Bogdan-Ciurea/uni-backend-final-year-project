/**
 * @file users_cql_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the user object
 * @version 0.1
 * @date 2023-01-06
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef USERS_CQL_MANAGER_H
#define USERS_CQL_MANAGER_H

#include "../database_objects/user_object.hpp"
#include "cql_client.hpp"

class UsersCqlManager {
 public:
  /**
   * @brief Construct a new Users Cql Manager object
   *
   * @param cql_client - The cassandra cql client
   */
  UsersCqlManager(CqlClient* cql_client) : _cql_client(cql_client) {}

  /**
   * @brief Destroy the Users Cql Manager object
   *
   */
  ~UsersCqlManager(){};

  /**
   * @brief Will prepare the database to be operated on
   *
   * @param init_db_schema - If you want to also create the keyspace and table
   *
   * @return CqlResult     - The result of the operation
   */
  CqlResult configure(bool init_db_schema);

  /**
   * @brief Adds a user entry on the database
   *
   * @param user_data - The user object we want to add
   *
   * @return CqlResult   - The result of the operation
   */
  CqlResult create_user(const UserObject& user_data);

  /**
   * @brief Get the user object related to a user id
   *
   * @param school_id  - The id of the school
   * @param user_id    - The id of the user
   *
   * @return std::pair<CqlResult, UserObject>
   */
  std::pair<CqlResult, UserObject> get_user(const int school_id,
                                            const CassUuid& user_id);

  /**
   * @brief Get the users by school object
   *
   * @param school_id  - The id of the school
   * @return std::pair<CqlResult, std::vector<UserObject>>
   */
  std::pair<CqlResult, std::vector<UserObject>> get_users_by_school(
      const int school_id);

  /**
   * @brief Get the user object related to a user email and password
   *
   * @param school_id  - The id of the school
   * @param email      - The email of the user
   * @param password   - The password of the user
   *
   * @return std::pair<CqlResult, UserObject>
   */
  std::pair<CqlResult, UserObject> get_user_by_email(const int school_id,
                                                     const std::string& email);

  /**
   * @brief Update an existing user entry on the database
   *
   * @param school_id    - The id of the school
   * @param user_id      - The id of the user
   * @param email        - The new email
   * @param password     - The new password
   * @param user_type    - The new user type
   * @param changed_password - If the password was changed
   * @param first_name         - The new first_name
   * @param last_name      - The new last_name
   * @param phone_number - The new phone number
   * @param last_time_online - The new last time online
   *
   * @return CqlResult   - The result of the operation
   */
  CqlResult update_user(const int school_id, const CassUuid& user_id,
                        const std::string& email, const std::string& password,
                        const UserType& user_type, const bool& changed_password,
                        const std::string& first_name,
                        const std::string& last_name,
                        const std::string& phone_number,
                        const time_t& last_time_online);

  /**
   * @brief Delete an existing user entry on the database
   *
   * @param school_id    - The id of the school
   * @param user_id      - The id of the user
   */
  CqlResult delete_user(const int school_id, const CassUuid& user_id);

 private:
  CqlClient* _cql_client;

  CassPreparedPtr _prepared_insert_user;

  CassPreparedPtr _prepared_get_user;

  CassPreparedPtr _prepared_get_users_by_school;

  CassPreparedPtr _prepared_get_user_by_email;

  CassPreparedPtr _prepared_update_user;

  CassPreparedPtr _prepared_delete_user;

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

  const char* CREATE_USERS_TABLE =
      "CREATE TABLE IF NOT EXISTS schools.users ( "
      "school int, "
      "id uuid, "
      "email varchar, "
      "password varchar, "
      "type int, "
      "changed_password boolean, "
      "first_name varchar, "
      "last_name varchar, "
      "phone_nr varchar, "
      "last_time_online timestamp, "
      "PRIMARY KEY ((school, id)));";

  const char* INSERT_USER =
      "INSERT INTO schools.users (school, id, email, password, type, "
      "changed_password, first_name, last_name, phone_nr , last_time_online ) "
      "VALUES "
      "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?) IF NOT EXISTS;";

  const char* GET_USER =
      "SELECT school, id, email, password, type, changed_password, first_name, "
      "last_name, "
      "phone_nr, last_time_online FROM schools.users WHERE school = ? AND id = "
      "?;";

  const char* GET_USERS_BY_SCHOOL =
      "SELECT school, id, email, password, type, changed_password, first_name, "
      "last_name, "
      "phone_nr, last_time_online FROM schools.users WHERE school = ? "
      "ALLOW FILTERING;";

  const char* GET_USER_BY_EMAIL =
      "SELECT school, id, email, password, type, changed_password, first_name, "
      "last_name, "
      "phone_nr, last_time_online FROM schools.users WHERE school = ? AND "
      "email = ? ALLOW FILTERING ;";

  const char* UPDATE_USER =
      "UPDATE schools.users SET email = ?, password = ?, type = ?, "
      "changed_password = ?, first_name = ?, last_name = ?, phone_nr = ?, "
      "last_time_online = ? WHERE school = ? AND id = ? IF EXISTS;";

  const char* DELETE_USER =
      "DELETE FROM schools.users WHERE school = ? AND id = ? IF EXISTS;";
};

CqlResult map_row_to_user(const CassRow* row, UserObject& user);

#endif