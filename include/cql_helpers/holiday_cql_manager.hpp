/**
 * @file holiday_cql_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the holiday object
 * @version 0.1
 * @date 2022-11-25
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef HOLIDAY_CQL_MANAGER_H
#define HOLIDAY_CQL_MANAGER_H

#include "../database_objects/holiday_object.hpp"
#include "cql_client.hpp"

class HolidayCqlManager {
 public:
  /**
   * @brief Construct a new Holiday Cql Manager object
   *
   * @param cql_client - The cassandra cql client
   */
  HolidayCqlManager(CqlClient* cql_client) : _cql_client(cql_client) {}

  /**
   * @brief Destroy the Holiday Cql Manager object
   *
   */
  ~HolidayCqlManager() {}

  /**
   * @brief Will prepare the database to be operated on
   *
   * @param init_db_schema - If you want to also create the keyspace and table
   * @return CqlResult     - The result of the operation
   */
  CqlResult configure(bool init_db_schema);

  /**
   * @brief Adds a holiday entry on the database
   *
   * @param holiday    - The holiday object we want to add
   * @return CqlResult - The result of the operation
   */
  CqlResult create_holiday(const HolidayObject& holiday_data);

  /**
   * @brief Get the specific holiday object
   *
   * @param school_or_country_id - The id of the school or country
   * @param type                 - 0 if this is a national holiday
   *                             - 1 if this is a custom school holiday
   * @param timestamp            - The timestamp of the holiday
   * @return                     - Result of the operation and the records from
   * holiday table
   */
  std::pair<CqlResult, HolidayObject> get_specific_holiday(
      const int school_or_country_id, const HolidayType type,
      const time_t timestamp);

  /**
   * @brief Get the holidays objects
   *
   * @param school_or_country_id - The id of the school or country
   * @param type                 - 0 if this is a national holiday
   *                             - 1 if this is a custom school holiday
   * @return                     - Result of the operation and the records from
   * holiday table
   */
  std::pair<CqlResult, std::vector<HolidayObject>> get_holidays(
      const int school_or_country_id, const HolidayType type);

  /**
   * @brief Will update the information of a holiday.
   * Unfortunately, this operation will use the delete function,
   * followed by the create function.
   *
   * @param new_holiday - The new version of the holiday
   * @param old_holiday - The holiday we want to change
   * @return CqlResult  - The result of the operation
   */
  CqlResult update_holiday(const HolidayObject& new_holiday_data,
                           const HolidayObject& old_holiday_data);

  /**
   * @brief Deletes a specific holiday
   *
   * @param holiday    - The holiday we want to delete
   * @return CqlResult - The result of the operation
   */
  CqlResult delete_specific_holiday(const HolidayObject& holiday_data);

  /**
   * @brief Deletes holidays by country or school id and type
   *
   * @param school_or_country_id - The id of the school or country
   * @param type                 - 0 if this is a national holiday
   *                               1 if this is a custom school holiday
   * @return                     - Result of the operation
   */
  CqlResult delete_holidays(const int school_or_country_id,
                            const HolidayType type);

 private:
  CqlClient* _cql_client;

  // Prepared statements to create a new holiday
  CassPreparedPtr _prepared_insert_holiday;

  // Prepared statements to get a specific holiday
  CassPreparedPtr _prepared_select_holiday;

  // Prepared statement to get all holidays for by id and type
  CassPreparedPtr _prepared_select_holidays_by_id_and_type;

  // Prepared statements to delete a specific holiday
  CassPreparedPtr _prepared_delete_holiday;

  // Prepared statements to delete all holidays for by id and type
  CassPreparedPtr _prepared_delete_holidays_by_id_and_type;

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

  const char* CREATE_TABLE_HOLIDAY =
      "CREATE TABLE IF NOT EXISTS environment.holidays_by_country_or_school ("
      "country_or_school_id int, "
      "type int, "
      "date timestamp, "
      "name varchar, "
      "PRIMARY key ((country_or_school_id, type), date));";

  const char* INSERT_HOLIDAY =
      "INSERT INTO environment.holidays_by_country_or_school ( "
      "country_or_school_id, type, date, name) "
      "VALUES (?, ?, ?, ?) IF NOT EXISTS;";

  const char* SELECT_HOLIDAY =
      "SELECT country_or_school_id, type, date, name "
      "FROM environment.holidays_by_country_or_school "
      "WHERE country_or_school_id = ? AND type = ? AND "
      "date = ?;";

  const char* SELECT_HOLIDAYS_BY_ID_AND_TYPE =
      "SELECT country_or_school_id, type, date, name "
      "FROM environment.holidays_by_country_or_school "
      "WHERE country_or_school_id = ? AND type = ?;";

  const char* DELETE_HOLIDAY =
      "DELETE FROM environment.holidays_by_country_or_school "
      "WHERE country_or_school_id = ? AND type = ? AND date = ? IF EXISTS;";

  const char* DELETE_HOLIDAYS_BY_ID_AND_TYPE =
      "DELETE FROM environment.holidays_by_country_or_school "
      "WHERE country_or_school_id = ? AND type = ?;";
};

/**
 * @brief
 *
 * @param row        - The cassandra row returned from the query
 * @param holiday    - The retuned holiday
 * @return CqlResult - The result of the operation
 */
CqlResult map_row_to_holiday(const CassRow* row, HolidayObject& holiday);

#endif  // HOLIDAY_CQL_MANAGER_H