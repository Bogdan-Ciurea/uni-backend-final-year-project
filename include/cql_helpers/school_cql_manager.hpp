/**
 * @file school_cql_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the school object
 * @version 0.1
 * @date 2022-12-09
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef SCHOOL_CQL_MANAGER_H
#define SCHOOL_CQL_MANAGER_H

#include "../database_objects/school_object.hpp"
#include "cql_client.hpp"

class SchoolCqlManager {
 public:
  /**
   * @brief Construct a new School Cql Manager object
   *
   * @param cql_client - The cassandra cql client
   */
  SchoolCqlManager(CqlClient* cql_client) : _cql_client(cql_client) {}

  /**
   * @brief Destroy the School Cql Manager object
   *
   */
  ~SchoolCqlManager() {}

  /**
   * @brief Will prepare the database to be operated on
   *
   * @param init_db_schema - If you want to also create the keyspace and table
   * @return CqlResult     - The result of the operation
   */
  CqlResult configure(bool init_db_schema);

  /**
   * @brief Adds a school entry on the database
   *
   * @param school_data - The school object we want to add
   * @return CqlResult  - The result of the operation
   */
  CqlResult create_school(const SchoolObject& school_data);

  /**
   * @brief Get a specific school object
   *
   * @param id - The id of the school we want to get
   *
   * @return Result of the operation and the record from
   * school table
   */
  std::pair<CqlResult, SchoolObject> get_school(const int id);

  /**
   * @brief Get all the school objects
   *
   * @return Result of the operation and the records from
   * school table
   */
  std::pair<CqlResult, std::vector<SchoolObject>> get_all_schools();

  /**
   * @brief Updates the values of a school
   *
   * @param id         - The id of the entry that we want to edit
   * @param name       - The new name of the entry
   * @param country_id - The new country id of the entry
   * @param image_path - The new image path of the entry
   * @return CqlResult - The result of the operation
   */
  CqlResult update_school(const int id, const std::string name,
                          const int country_id, std::string image_path);

  /**
   * @brief Deletes an entry by its id
   *
   * @param id         - The id of the entry that we want to delete
   * @return CqlResult - The result of the operation
   */
  CqlResult delete_school(const int id);

 private:
  // Pointer to the CqlClient
  CqlClient* _cql_client;

  // Prepared statement to create e new school
  CassPreparedPtr _prepared_insert_school;

  // Prepared statement to select a specific school
  CassPreparedPtr _prepared_select_school;

  // Prepared statement to select all schools
  CassPreparedPtr _prepared_select_all_schools;

  // Prepared statement to update a school
  CassPreparedPtr _prepared_update_school;

  // Prepared statement to delete a school
  CassPreparedPtr _prepared_delete_school;

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

  const char* CREATE_ENVIRONMENT_KEYSPACE =
      "CREATE KEYSPACE IF NOT EXISTS environment "
      "WITH REPLICATION = { 'class' : 'SimpleStrategy', "
      "'replication_factor' : 3 };";

  const char* CREATE_TABLE_SCHOOLS =
      "CREATE TABLE IF NOT EXISTS environment.schools ( "
      "id int, name varchar, country int,"
      "image_path varchar, PRIMARY KEY (id));";

  const char* INSERT_SCHOOL =
      "INSERT INTO environment.schools (id, name, country, image_path) "
      "VALUES (?, ?, ?, ?) IF NOT EXISTS;";

  const char* SELECT_SCHOOL =
      "SELECT id, name, country, image_path FROM environment.schools "
      "WHERE id = ?;";

  const char* SELECT_ALL_SCHOOLS =
      "SELECT id, name, country, image_path FROM environment.schools;";

  const char* UPDATE_SCHOOL =
      "UPDATE environment.schools SET name = ?, country = ?, "
      "image_path = ? WHERE id = ? IF EXISTS;";

  const char* DELETE_SCHOOL =
      "DELETE FROM environment.schools WHERE id = ? IF EXISTS;";
};

CqlResult map_row_to_school(const CassRow* row, SchoolObject& school);

#endif  // SCHOOL_CQL_MANAGER_H