/**
 * @file holiday_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the holiday object
 * @version 0.1
 * @date 2022-12-01
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "cql_helpers/holiday_cql_manager.hpp"

CqlResult HolidayCqlManager::configure(bool init_db_schema) {
  CqlResult cql_result = ResultCode::OK;

  if (init_db_schema) {
    cql_result = init_schema();

    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << "Failed to initialise holiday table:\n"
                << cql_result.str_code() << "\n"
                << cql_result.error();
      return cql_result;
    }
  }

  cql_result = init_prepare_statements();

  return cql_result;
}

CqlResult HolidayCqlManager::init_prepare_statements() {
  CqlResult cql_result =
      _cql_client->prepare_statement(_prepared_insert_holiday, INSERT_HOLIDAY);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise insert holiday prepared statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_select_holiday, SELECT_HOLIDAY);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR
        << "Failed to initialise select specific holiday prepared statement: "
        << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(
      _prepared_select_holidays_by_id_and_type, SELECT_HOLIDAYS_BY_ID_AND_TYPE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise select all holidays by id and type "
                 "prepared statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_delete_holiday, DELETE_HOLIDAY);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR
        << "Failed to initialise delete specific holiday prepared statement: "
        << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(
      _prepared_delete_holidays_by_id_and_type, DELETE_HOLIDAYS_BY_ID_AND_TYPE);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise delete all holidays by id and type "
                 "prepared statement: "
              << cql_result.str_code() << cql_result.error();
  }

  return cql_result;
}

CqlResult HolidayCqlManager::init_schema() {
  CqlResult cql_result =
      _cql_client->execute_statement(CREATE_ENVIRONMENT_KEYSPACE);

  if (cql_result.code() != ResultCode::OK) {
    return cql_result;
  }

  cql_result = _cql_client->execute_statement(CREATE_TABLE_HOLIDAY);

  return cql_result;
}

CqlResult HolidayCqlManager::create_holiday(const HolidayObject& holiday_data) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_insert_holiday.get()));

    cass_statement_bind_int32(statement.get(), 0,
                              holiday_data._country_or_school_id);
    cass_statement_bind_int32(statement.get(), 1,
                              holiday_type_to_int(holiday_data._type));
    cass_statement_bind_int64(statement.get(), 2, holiday_data._date * 1000);
    cass_statement_bind_string(statement.get(), 3, holiday_data._name.c_str());

    auto [cql_result, cass_result] =
        _cql_client->execute_statement(statement.get());

    if (cql_result.code() == ResultCode::OK)
      cql_result = was_applied(cass_result);

    return cql_result;
  } catch (std::exception& ex) {
    error_message = ex.what();
  } catch (...) {
    error_message = "unknown exception";
  }

  return {ResultCode::UNKNOWN_ERROR, error_message};
}

std::pair<CqlResult, HolidayObject> HolidayCqlManager::get_specific_holiday(
    const int school_or_country_id, const HolidayType type,
    const time_t timestamp) {
  std::vector<HolidayObject> holidays;

  // Define the result handler that pre-allocate space in holidays vector
  auto reserve_holidays = [&holidays](size_t num_rows) {
    // Reserve capacity for all returned holidays
    holidays.reserve(num_rows);
  };

  // Define the row handler that will store the holidays
  auto copy_holiday_details = [&holidays](const CassRow* row) {
    holidays.emplace_back();
    HolidayObject& holiday = holidays.back();

    CqlResult cql_result = map_row_to_holiday(row, holiday);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_select_holiday.get()));
  cass_statement_bind_int32(statement.get(), 0, school_or_country_id);
  cass_statement_bind_int32(statement.get(), 1, static_cast<int>(type));
  cass_statement_bind_int64(statement.get(), 2, timestamp * 1000);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_holidays, copy_holiday_details);

  if (cql_result.code() != ResultCode::OK) return {cql_result, HolidayObject()};

  if (holidays.size() != 1)
    return {{ResultCode::NOT_FOUND, "No entries found"}, HolidayObject()};

  return {cql_result, holidays.at(0)};
}

std::pair<CqlResult, std::vector<HolidayObject>>
HolidayCqlManager::get_holidays(const int school_or_country_id,
                                const HolidayType type) {
  std::vector<HolidayObject> holidays;

  // Define the result handler that pre-allocate space in holidays vector
  auto reserve_holidays = [&holidays](size_t num_rows) {
    // Reserve capacity for all returned holidays
    holidays.reserve(num_rows);
  };

  // Define the row handler that will store the holidays
  auto copy_holiday_details = [&holidays](const CassRow* row) {
    holidays.emplace_back();
    HolidayObject& holiday = holidays.back();

    CqlResult cql_result = map_row_to_holiday(row, holiday);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_select_holidays_by_id_and_type.get()));
  cass_statement_bind_int32(statement.get(), 0, school_or_country_id);
  cass_statement_bind_int32(statement.get(), 1, static_cast<int>(type));

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_holidays, copy_holiday_details);

  return {cql_result, holidays};
}

CqlResult HolidayCqlManager::update_holiday(
    const HolidayObject& new_holiday_data,
    const HolidayObject& old_holiday_data) {
  CqlResult cql_result = delete_specific_holiday(old_holiday_data);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to make space for new holiday:\n"
              << cql_result.str_code() << "\n"
              << cql_result.error();
    return cql_result;
  }

  cql_result = create_holiday(new_holiday_data);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to change the holiday:\n"
              << cql_result.str_code() << "\n"
              << cql_result.error();
    return cql_result;
  }

  return cql_result;
}

CqlResult HolidayCqlManager::delete_specific_holiday(
    const HolidayObject& holiday_data) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_delete_holiday.get()));

    cass_statement_bind_int32(statement.get(), 0,
                              holiday_data._country_or_school_id);
    cass_statement_bind_int32(statement.get(), 1,
                              static_cast<int>(holiday_data._type));
    cass_statement_bind_int64(statement.get(), 2, holiday_data._date * 1000);

    auto [cql_result, cass_result] =
        _cql_client->execute_statement(statement.get());

    if (cql_result.code() == ResultCode::OK) {
      cql_result = was_applied(cass_result);
    }

    return cql_result;
  } catch (std::exception& ex) {
    error_message = ex.what();
  } catch (...) {
    error_message = "unknown exception";
  }

  return {ResultCode::UNKNOWN_ERROR, error_message};
}

CqlResult HolidayCqlManager::delete_holidays(const int school_or_country_id,
                                             const HolidayType type) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_delete_holidays_by_id_and_type.get()));

    cass_statement_bind_int32(statement.get(), 0, school_or_country_id);
    cass_statement_bind_int32(statement.get(), 1, static_cast<int>(type));

    auto [cql_result, cass_result] =
        _cql_client->execute_statement(statement.get());

    return cql_result;
  } catch (std::exception& ex) {
    error_message = ex.what();
  } catch (...) {
    error_message = "unknown exception";
  }

  return {ResultCode::UNKNOWN_ERROR, error_message};
}

CqlResult map_row_to_holiday(const CassRow* row, HolidayObject& holiday) {
  CqlResult cql_result = get_int_value(row, 0, holiday._country_or_school_id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  int temp;
  cql_result = get_int_value(row, 1, temp);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }
  holiday._type = holiday_type_from_int(temp);

  cql_result = get_long_value(row, 2, holiday._date);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }
  holiday._date /= 1000;

  cql_result = get_text_value(row, 3, holiday._name);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  return {ResultCode::OK};
}
