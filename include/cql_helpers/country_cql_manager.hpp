/**
 * @file country_cql_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the country object
 * @version 0.1
 * @date 2022-12-01
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef COUNTRY_CQL_MANAGER_H
#define COUNTRY_CQL_MANAGER_H

#include "../database_objects/country_object.hpp"
#include "cql_client.hpp"

class CountryCqlManager {
 public:
  /**
   * @brief Construct a new Holiday Cql Manager object
   *
   * @param cql_client - The cassandra cql client
   */
  CountryCqlManager(CqlClient* cql_client) : _cql_client(cql_client) {}

  /**
   * @brief Destroy the Holiday Cql Manager object
   *
   */
  ~CountryCqlManager() {}

  /**
   * @brief Will prepare the database to be operated on
   *
   * @param init_db_schema - If you want to also create the keyspace and table
   * @return CqlResult     - The result of the operation
   */
  CqlResult configure(bool init_db_schema);

  /**
   * @brief Adds a country entry on the database
   *
   * @param country_data - The country object we want to add
   * @return CqlResult   - The result of the operation
   */
  CqlResult create_country(const CountryObject& country_data);

  /**
   * @brief Get the country object
   *
   * @param id - The id of the country
   * @return std::pair<CqlResult, CountryObject>
   */
  std::pair<CqlResult, CountryObject> get_country(const int id);

  /**
   * @brief Get the all country objects
   *
   * @return Result of the operation and the records from
   * countries table
   */
  std::pair<CqlResult, std::vector<CountryObject>> get_all_countries();

  /**
   * @brief Updates the values of a country
   *
   * @param id         - The id of the entry that we want to edit
   * @param name       - The new name of the entry
   * @param code       - The new code of the entry
   * @return CqlResult - The result of the operation
   */
  CqlResult update_country(const int id, const std::string name,
                           const std::string code);

  /**
   * @brief Deletes an entry by its id
   *
   * @param id         - The id of the entry that we want to delete
   * @return CqlResult - The result of the operation
   */
  CqlResult delete_country(const int id);

 private:
  // Pointer to the CqlClient
  CqlClient* _cql_client;

  // Prepared statement to create e new country
  CassPreparedPtr _prepared_insert_country;

  // Prepared statement to select a specific country
  CassPreparedPtr _prepared_select_country;

  // Prepared statement to select all countries
  CassPreparedPtr _prepared_select_all_countries;

  // Prepared statement to update a country
  CassPreparedPtr _prepared_update_country;

  // Prepared statement to delete a country
  CassPreparedPtr _prepared_delete_country;

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

  const char* CREATE_TABLE_COUNTRIES =
      "CREATE TABLE IF NOT EXISTS environment.countries ( "
      "id int PRIMARY KEY, "
      "name varchar, "
      "code varchar );";

  const char* INSERT_COUNTRY =
      "INSERT INTO environment.countries (id, name, code) "
      "VALUES (?, ?, ?) IF NOT EXISTS;";

  const char* SELECT_COUNTRY =
      "SELECT id, name, code FROM environment.countries WHERE id = ?;";

  const char* SELECT_ALL_COUNTRIES =
      "SELECT id, name, code FROM environment.countries;";

  const char* UPDATE_COUNTRY =
      "UPDATE environment.countries SET name = ?, code = ? WHERE id = ? IF "
      "EXISTS;";

  const char* DELETE_COUNTRY =
      "DELETE FROM environment.countries WHERE id = ? IF EXISTS;";
};

CqlResult map_row_to_country(const CassRow* row, CountryObject& country);

#endif  // COUNTRY_CQL_MANAGER_H