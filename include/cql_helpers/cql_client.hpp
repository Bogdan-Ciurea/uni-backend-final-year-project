/**
 * @file cql_client.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief Implementation of the client that will interact with the cassandra
 * server.
 * @version 0.1
 * @date 2022-11-23
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef CQL_STORE_HPP
#define CQL_STORE_HPP

#include <stdio.h>

#include <memory>
#include <string>
#include <vector>

#include "Logger.h"
#include "cassandra.h"

// Custom deleter for cassandra statement
struct StatementDeleter {
  void operator()(CassStatement* statement) { cass_statement_free(statement); };
};

// Custom deleter for cassandra prepared statement
struct PreparedDeleter {
  void operator()(const CassPrepared* prepared) {
    cass_prepared_free(prepared);
  };
};

// Custom deleter for cassandra future
struct FutureDeleter {
  void operator()(CassFuture* future) { cass_future_free(future); };
};

// Custom deleter for cassandra result
struct ResultDeleter {
  void operator()(const CassResult* result) { cass_result_free(result); };
};

// Custom deleter for cassandra iterator
struct IteratorDeleter {
  void operator()(CassIterator* iterator) { cass_iterator_free(iterator); };
};

struct CollectionDeleter {
  void operator()(CassCollection* collection) {
    cass_collection_free(collection);
  };
};

// Smart pointer for cassandra statement
using CassStatementPtr = std::unique_ptr<CassStatement, StatementDeleter>;

// Smart pointer for cassandra prepared statement
using CassPreparedPtr = std::unique_ptr<const CassPrepared, PreparedDeleter>;

// Smart pointer for cassandra future
using CassFuturePtr = std::unique_ptr<CassFuture, FutureDeleter>;

// Smart pointer for cassandra result
using CassResultPtr = std::unique_ptr<const CassResult, ResultDeleter>;

// Smart pointer for cassandra iterator
using CassIteratorPtr = std::unique_ptr<CassIterator, IteratorDeleter>;

using CassCollectionPtr = std::unique_ptr<CassCollection, CollectionDeleter>;

/**
 * @brief Enumerator of the result codes that we can get from cassandra.
 *
 */
enum class ResultCode {
  OK = 0,
  INVALID_REQUEST = 1,
  NOT_FOUND = 2,
  CONNECTION_ERROR = 3,
  RESOURCE_ERROR = 4,
  UNKNOWN_ERROR = 5,
  UNAVAILABLE = 6,
  TIMEOUT = 7,
  NOT_APPLIED = 8
};

/**
 * @brief The result of an interaction with Cassandra.
 * It contains the result code and the error description (if the operation
 * failed).
 *
 */
class CqlResult {
 public:
  CqlResult(ResultCode code, std::string error = "")
      : _code(code), _error(std::move(error)){};

  // Getter function for the _code
  ResultCode code() const { return _code; };

  // Getter function for the _error
  std::string error() const { return _error; };

  // Get the code as a string
  std::string str_code();

 private:
  ResultCode _code;
  std::string _error;
};

/**
 * @brief Wrapper class around the Cassandra C++ driver.
 * It manages the connection between the client and Cassandra.
 * Supports low level functions to execute CQL commands.
 * The application should build only one instance of this class and share
 * it between multiple other classes.
 *
 */
class CqlClient {
 public:
  /**
   * @brief Construct a new Cql Client object
   *
   * @param cass_hostname - The hostname that we want to connect to
   * @param cass_port     - The port of the hostname that we want to connect to
   */
  CqlClient(std::string cass_hostname, unsigned short cass_port)
      : _cass_hostname(std::move(cass_hostname)), _cass_port(cass_port){};

  /**
   * @brief Destructor for the Cql Client object
   *
   */
  virtual ~CqlClient();

  /**
   * @brief Will connect to the Cassandra cluster
   *
   * @return Result code of the operation
   */
  CqlResult connect();

  /**
   * @brief Will execute a CQL command.
   *
   * @param cql_statement the CQL command in a string form
   * @return Result of the execution
   */
  CqlResult execute_statement(const char* cql_statement);

  /**
   * @brief Execute a CQL command
   *
   * @param statement The statement that we want to execute
   * @return The result of the operation
   */
  std::pair<CqlResult, CassResultPtr> execute_statement(
      CassStatement* statement);

  /**
   * @brief Will execute a statement and process records returned by Cassandra
   *
   * @param statement        - The statement that we want to execute
   * @param result_allocator - Function object called with the number of records
   *                           returned by the statement. Can be used to
   * pre-allocate space to store the object
   * @param row_handler      - Function object called with each record (CassRow)
   *                           returned by the statement. Used to process the
   *                           the records returned from Cassandra.
   * @return                 - Result of the execution
   */
  template <typename FAlloc, typename FRow>
  CqlResult select_rows(CassStatement* statement, FAlloc result_allocator,
                        FRow row_handler);

  /**
   * @brief Prepare a statement that can be used multiple times with different
   * parameters
   *
   * @param prepared_statement - Out parameter containing the prepared statement
   * @param query              - Statement to be prepared as a null terminated
   * string
   * @return                   - Result of statement
   */
  CqlResult prepare_statement(CassPreparedPtr& prepared_statement,
                              const char* query);

 private:
  std::string _cass_hostname;
  unsigned short _cass_port;
  CassCluster* _cluster = nullptr;
  CassSession* _session = nullptr;
};

/**
 * @brief Get the cql result object
 *
 * @param future  - Cassandra future obtained after executing an operation.
 * @return        - Result with the error code and message extracted from
 * future.
 */
CqlResult get_cql_result(CassFuture* future);

/**
 * @brief Get the result code object
 *
 * @param cass_result_code - Cassandra result code.
 * @return ResultCode      - CQL Client result code.
 */
ResultCode get_result_code(CassError cass_result_code);

/**
 * @brief Tests if an operation was executed when LWT is used
 *
 * @param result    - Cassandra result returned by a query.
 * @return          - True, if the operation was executed.
 */
CqlResult was_applied(const CassResultPtr& result);

/**
 * @brief Get the bool value object
 *
 * @param row           - Cassandra row returned by a query.
 * @param column_index  - Index of the column from where to read the value.
 * @param value         - Output parameter where the value is returned.
 * @return              - Result of the action.
 */
CqlResult get_bool_value(const CassRow* row, size_t column_index, bool& value);

/**
 * @brief Get the int value object
 *
 * @param row           - Cassandra row returned by a query.
 * @param column_index  - Index of the column from where to read the value.
 * @param value         - Output parameter where the value is returned.
 * @return              - Result of the action.
 */
CqlResult get_int_value(const CassRow* row, size_t column_index, int& value);

/**
 * @brief Get the float value object
 *
 * @param row           - Cassandra row returned by a query.
 * @param column_index  - Index of the column from where to read the value.
 * @param value         - Output parameter where the value is returned.
 * @return              - Result of the action.
 */
CqlResult get_float_value(const CassRow* row, size_t column_index,
                          float& value);

/**
 * @brief Get the long value object
 *
 * @param row           - Cassandra row returned by a query.
 * @param column_index  - Index of the column from where to read the value.
 * @param value         - Output parameter where the value is returned.
 * @return              - Result of the action.
 */
CqlResult get_long_value(const CassRow* row, size_t column_index, long& value);

/**
 * @brief Get the text value object
 *
 * @param row           - Cassandra row returned by a query.
 * @param column_index  - Index of the column from where to read the value.
 * @param value         - Output parameter where the value is returned.
 * @return              - Result of the action.
 */
CqlResult get_text_value(const CassRow* row, size_t column_index,
                         std::string& value);

/**
 * @brief Get the uuid value object
 *
 * @param row           - Cassandra row returned by a query.
 * @param column_index  - Index of the column from where to read the value.
 * @param value         - Output parameter where the value is returned.
 * @return              - Result of the action.
 */
CqlResult get_uuid_value(const CassRow* row, size_t column_index,
                         CassUuid& uuid);

/**
 * @brief Get the array uuids value object
 *
 * @param row           - Cassandra row returned by a query.
 * @param column_index  - Index of the column from where to read the value.
 * @param value         - Output parameter where the value is returned.
 * @return              - Result of the action.
 */
CqlResult get_array_uuids_value(const CassRow* row, size_t column_index,
                                std::vector<CassUuid>& uuids);

/**
 * @brief Create uuid object with the current time
 *
 * @return CassUuid of the current time
 */
CassUuid create_current_uuid();

/**
 * @brief Create a uuid with at specific time
 *
 * @param time       - The time we want to create the uuid on
 * @return CassUuid  - The UUID created on the time
 */
CassUuid create_uuid_on_time(time_t time);

/**
 * @brief Get the uuid string object
 *
 * @param uuid          - The uuid we want to convert to string
 * @return std::string  - The string representation of the uuid
 */
std::string get_uuid_string(CassUuid uuid);

template <typename FAlloc, typename FRow>
CqlResult CqlClient::select_rows(CassStatement* statement,
                                 FAlloc result_allocator, FRow row_handler) {
  ResultCode result_code = ResultCode::OK;
  std::string result_error;

  try {
    auto [cql_result, cass_results] = execute_statement(statement);

    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    size_t num_rows = cass_result_row_count(cass_results.get());

    if (num_rows == 0) {
      return {ResultCode::NOT_FOUND, "No entries found with this data!"};
    }

    // Let the caller allocate memory to store teh rows
    result_allocator(num_rows);

    CassIteratorPtr row_iterator(cass_iterator_from_result(cass_results.get()));

    while (cass_iterator_next(row_iterator.get())) {
      const CassRow* row = cass_iterator_get_row(row_iterator.get());

      if (row == nullptr) {
        return {ResultCode::UNKNOWN_ERROR, "Failed to get row from iterator"};
      }

      // Let the caller decides what to do with the row
      row_handler(row);
    }
  } catch (std::exception& ex) {
    result_code = ResultCode::UNKNOWN_ERROR;
    result_error =
        std::string("Exception while processing selected rows: ") + ex.what();
  } catch (...) {
    result_code = ResultCode::UNKNOWN_ERROR;
    result_error =
        std::string("Unknown exception while processing selected rows");
  }

  return {result_code, result_error};
}

#endif