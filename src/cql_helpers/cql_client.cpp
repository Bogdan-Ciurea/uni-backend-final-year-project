#include "cql_helpers/cql_client.hpp"

std::string CqlResult::str_code() {
  switch (_code) {
    case ResultCode::OK:
      return "OK";
    case ResultCode::INVALID_REQUEST:
      return "INVALID_REQUEST";
    case ResultCode::NOT_FOUND:
      return "NOT_FOUND";
    case ResultCode::CONNECTION_ERROR:
      return "CONNECTION_ERROR";
    case ResultCode::RESOURCE_ERROR:
      return "RESOURCE_ERROR";
    case ResultCode::UNKNOWN_ERROR:
      return "UNKNOWN_ERROR";
    case ResultCode::UNAVAILABLE:
      return "UNAVAILABLE";
    case ResultCode::TIMEOUT:
      return "TIMEOUT";
    default:
      return "UNKNOWN_CODE";
  }
}

CqlClient::~CqlClient() {
  if (_session != nullptr) {
    CassFuturePtr close_future(cass_session_close(_session));

    cass_future_wait(close_future.get());

    if (_cluster != nullptr) {
      cass_cluster_free(_cluster);
    }

    cass_session_free(_session);
  }
}

CqlResult CqlClient::connect() {
  LOG_INFO << "Connecting to Cassandra cluster at: " + _cass_hostname + ":"
           << _cass_port;

  cass_log_set_level(CASS_LOG_ERROR);

  _cluster = cass_cluster_new();
  _session = cass_session_new();

  cass_cluster_set_contact_points(_cluster, _cass_hostname.c_str());
  cass_cluster_set_port(_cluster, _cass_port);

  CassFuturePtr future(cass_session_connect(_session, _cluster));
  cass_future_wait(future.get());

  CqlResult cql_result = get_cql_result(future.get());

  return cql_result;
}

CqlResult CqlClient::execute_statement(const char* cql_statement) {
  CassStatementPtr statement(cass_statement_new(cql_statement, 0));

  auto [cql_result, cass_results] = execute_statement(statement.get());

  return cql_result;
}

std::pair<CqlResult, CassResultPtr> CqlClient::execute_statement(
    CassStatement* statement) {
  std::string error_msg;

  try {
    CassFuturePtr future(cass_session_execute(_session, statement));

    cass_future_wait(future.get());

    CqlResult cql_result = get_cql_result(future.get());

    if (cql_result.code() != ResultCode::OK) {
      return {cql_result, CassResultPtr(nullptr)};
    }

    const CassResult* cass_result = cass_future_get_result(future.get());

    if (cass_result != nullptr) {
      return {CqlResult(ResultCode::OK), CassResultPtr(cass_result)};
    }

    error_msg = "NULL result from future";
  } catch (std::exception& ex) {
    error_msg = ex.what();
  } catch (...) {
    error_msg = "unknown exception";
  }

  // Got an error, build the error message
  std::string error = "Failed to execute statement: " + error_msg;

  return {CqlResult(ResultCode::UNKNOWN_ERROR, error), CassResultPtr(nullptr)};
}

CqlResult CqlClient::prepare_statement(CassPreparedPtr& prepared_statement,
                                       const char* query) {
  std::string error_msg;

  try {
    CassFuturePtr future(cass_session_prepare(_session, query));

    CqlResult cql_result = get_cql_result(future.get());

    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    const CassPrepared* ps = cass_future_get_prepared(future.get());

    if (ps != nullptr) {
      prepared_statement.reset(ps);

      return {ResultCode::OK};
    }

    error_msg = "NULL result from future";
  } catch (std::exception& ex) {
    error_msg = ex.what();
  } catch (...) {
    error_msg = "unknown exception";
  }

  // Got an error, build the error message
  std::string error =
      "Failed to prepare statement (" + std::string(query) + "): " + error_msg;

  return {ResultCode::UNKNOWN_ERROR, error};
}

CqlResult get_cql_result(CassFuture* future) {
  if (!future) {
    return {ResultCode::UNKNOWN_ERROR,
            "Failed to get result from NULL CassFuture"};
  }

  ResultCode result_code = get_result_code(cass_future_error_code(future));

  if (result_code == ResultCode::OK) {
    return {ResultCode::OK};
  }

  const char* message = nullptr;
  size_t message_length = 0;
  cass_future_error_message(future, &message, &message_length);

  return {result_code, std::string(message, message_length)};
}

ResultCode get_result_code(CassError cass_result_code) {
  ResultCode cql_store_rc = ResultCode::OK;

  switch (cass_result_code) {
    case CASS_OK:
      cql_store_rc = ResultCode::OK;
      break;
    case CASS_ERROR_SERVER_INVALID_QUERY:
      cql_store_rc = ResultCode::INVALID_REQUEST;
      break;
    case CASS_ERROR_LIB_NO_HOSTS_AVAILABLE:
      cql_store_rc = ResultCode::CONNECTION_ERROR;
      break;
    case CASS_ERROR_SERVER_READ_FAILURE:
    case CASS_ERROR_SERVER_FUNCTION_FAILURE:
    case CASS_ERROR_SERVER_WRITE_FAILURE:
      cql_store_rc = ResultCode::RESOURCE_ERROR;
      break;
    case CASS_ERROR_SERVER_UNAVAILABLE:
      cql_store_rc = ResultCode::UNAVAILABLE;
      break;
    case CASS_ERROR_SERVER_WRITE_TIMEOUT:
    case CASS_ERROR_SERVER_READ_TIMEOUT:
    case CASS_ERROR_LIB_REQUEST_TIMED_OUT:
      cql_store_rc = ResultCode::TIMEOUT;
      break;
    default:
      cql_store_rc = ResultCode::UNKNOWN_ERROR;
      break;
  }

  return cql_store_rc;
}

CqlResult was_applied(const CassResultPtr& cass_result) {
  const CassRow* row = cass_result_first_row(cass_result.get());

  if (row == nullptr) {
    return {ResultCode::UNKNOWN_ERROR, "Failed to get row"};
  }

  // Get the first column value using the column's name
  const CassValue* column = cass_row_get_column_by_name(row, "[applied]");
  if (column == nullptr) {
    return {ResultCode::UNKNOWN_ERROR, "Failed to get column from row"};
  }

  cass_bool_t applied;
  CassError cass_error = cass_value_get_bool(column, &applied);
  if (cass_error != CASS_OK) {
    return {ResultCode::UNKNOWN_ERROR, "Failed to get value from column"};
  }

  if (!applied) {
    return {ResultCode::NOT_APPLIED, "Command not applied"};
  }

  return {ResultCode::OK, ""};
}

CqlResult get_bool_value(const CassRow* row, size_t column_index, bool& value) {
  const CassValue* column = cass_row_get_column(row, column_index);
  if (column == nullptr) {
    return {ResultCode::UNKNOWN_ERROR, "Failed to get column from row"};
  }

  cass_bool_t bool_value;
  CassError cass_error = cass_value_get_bool(column, &bool_value);

  if (cass_error != CASS_OK) {
    return {ResultCode::UNKNOWN_ERROR, "Failed to get value from column"};
  }

  value = bool_value == cass_true;

  return {ResultCode::OK};
}

CqlResult get_int_value(const CassRow* row, size_t column_index, int& value) {
  const CassValue* column = cass_row_get_column(row, column_index);
  if (column == nullptr) {
    return {ResultCode::UNKNOWN_ERROR, "Failed to get column from row"};
  }

  cass_int32_t int_value;
  CassError cass_error = cass_value_get_int32(column, &int_value);

  if (cass_error != CASS_OK) {
    return {ResultCode::UNKNOWN_ERROR, "Failed to get value from column"};
  }

  value = int_value;

  return {ResultCode::OK};
}

CqlResult get_float_value(const CassRow* row, size_t column_index,
                          float& value) {
  const CassValue* column = cass_row_get_column(row, column_index);
  if (column == nullptr) {
    return {ResultCode::UNKNOWN_ERROR, "Failed to get column from row"};
  }

  cass_float_t float_value;
  CassError cass_error = cass_value_get_float(column, &float_value);

  if (cass_error != CASS_OK) {
    return {ResultCode::UNKNOWN_ERROR, "Failed to get value from column"};
  }

  value = float_value;

  return {ResultCode::OK};
}

CqlResult get_long_value(const CassRow* row, size_t column_index, long& value) {
  const CassValue* column = cass_row_get_column(row, column_index);
  if (column == nullptr) {
    return {ResultCode::UNKNOWN_ERROR, "Failed to get column from row"};
  }

  cass_int64_t long_value;
  CassError cass_error = cass_value_get_int64(column, &long_value);

  if (cass_error != CASS_OK) {
    return {ResultCode::UNKNOWN_ERROR, "Failed to get value from column"};
  }

  value = long_value;

  return {ResultCode::OK};
}

CqlResult get_text_value(const CassRow* row, size_t column_index,
                         std::string& value) {
  const char* value_str = nullptr;
  size_t value_length = 0;

  const CassValue* column = cass_row_get_column(row, column_index);
  if (column == nullptr) {
    return {ResultCode::UNKNOWN_ERROR, "Failed to get column from row"};
  }

  CassError cass_error =
      cass_value_get_string(column, &value_str, &value_length);

  if (cass_error != CASS_OK) {
    return {ResultCode::UNKNOWN_ERROR, "Failed to get value from column"};
  }

  value = std::string(value_str, value_length);

  return {ResultCode::OK};
}

CqlResult get_uuid_value(const CassRow* row, size_t column_index,
                         CassUuid& cass_uuid) {
  const CassValue* column = cass_row_get_column(row, column_index);
  if (column == nullptr) {
    return {ResultCode::UNKNOWN_ERROR, "Failed to get column from row"};
  }

  CassError cass_error = cass_value_get_uuid(column, &cass_uuid);

  if (cass_error != CASS_OK) {
    return {ResultCode::UNKNOWN_ERROR, "Failed to get value from column"};
  }

  return {ResultCode::OK};
}

CqlResult get_array_uuids_value(const CassRow* row, size_t column_index,
                                std::vector<CassUuid>& uuids) {
  const CassValue* value = cass_row_get_column(row, column_index);
  if (value == nullptr) {
    return {ResultCode::UNKNOWN_ERROR,
            "Failed to get value from row and column"};
  }

  uuids = {};

  CassIteratorPtr list_iterator(cass_iterator_from_collection(value));
  if (list_iterator == nullptr) {
    return {ResultCode::OK};
  }

  CassUuid cass_uuid;

  while (cass_iterator_next(list_iterator.get())) {
    const CassValue* value = cass_iterator_get_value(list_iterator.get());
    if (value == nullptr) {
      return {ResultCode::UNKNOWN_ERROR, "Failed to get value from list"};
    }

    CassError cass_error = cass_value_get_uuid(value, &cass_uuid);

    if (cass_error != CASS_OK) {
      return {ResultCode::UNKNOWN_ERROR, "Failed to get value from column"};
    }

    uuids.push_back(cass_uuid);
  }

  return {ResultCode::OK};
}

CassUuid create_current_uuid() {
  CassUuidGen* uuid_gen = cass_uuid_gen_new();

  CassUuid uuid;

  /* Generate a version 1 UUID */
  cass_uuid_gen_time(uuid_gen, &uuid);

  /* Generate a version 1 UUID from an existing timestamp */
  cass_uuid_gen_from_time(uuid_gen, time(nullptr), &uuid);

  /* Generate a version 4 UUID */
  cass_uuid_gen_random(uuid_gen, &uuid);

  cass_uuid_gen_free(uuid_gen);

  return uuid;
}

CassUuid create_uuid_on_time(time_t time) {
  CassUuidGen* uuid_gen = cass_uuid_gen_new();

  CassUuid uuid;

  /* Generate a version 1 UUID */
  cass_uuid_gen_time(uuid_gen, &uuid);

  /* Generate a version 1 UUID from an existing timestamp */
  cass_uuid_gen_from_time(uuid_gen, time, &uuid);

  /* Generate a version 4 UUID */
  cass_uuid_gen_random(uuid_gen, &uuid);

  cass_uuid_gen_free(uuid_gen);

  return uuid;
}

std::string get_uuid_string(CassUuid uuid) {
  char* uuid_str = (char*)malloc(37);

  cass_uuid_string(uuid, uuid_str);

  std::string uuid_string(uuid_str);

  free(uuid_str);

  return uuid_string;
}
