/**
 * @file country_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for communication between the backend and
 * database and manages the country object
 * @version 0.1
 * @date 2022-12-01
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "cql_helpers/country_cql_manager.hpp"

CqlResult CountryCqlManager::configure(bool init_db_schema) {
  CqlResult cql_result = ResultCode::OK;

  if (init_db_schema) {
    cql_result = init_schema();

    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << "Failed to initialise country table:\n"
                << cql_result.str_code() << "\n"
                << cql_result.error();
      return cql_result;
    }
  }

  cql_result = init_prepare_statements();

  return cql_result;
}

CqlResult CountryCqlManager::init_prepare_statements() {
  CqlResult cql_result =
      _cql_client->prepare_statement(_prepared_insert_country, INSERT_COUNTRY);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise insert country prepared statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_select_country, SELECT_COUNTRY);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR
        << "Failed to initialise select specific country prepared statement: "
        << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result = _cql_client->prepare_statement(_prepared_select_all_countries,
                                              SELECT_ALL_COUNTRIES);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR
        << "Failed to initialise select all countries prepared statement: "
        << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_update_country, UPDATE_COUNTRY);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise update holiday prepared statement: "
              << cql_result.str_code() << cql_result.error();

    return cql_result;
  }

  cql_result =
      _cql_client->prepare_statement(_prepared_delete_country, DELETE_COUNTRY);

  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << "Failed to initialise delete country prepared statement: "
              << cql_result.str_code() << cql_result.error();
  }

  return cql_result;
}

CqlResult CountryCqlManager::init_schema() {
  CqlResult cql_result =
      _cql_client->execute_statement(CREATE_ENVIRONMENT_KEYSPACE);

  if (cql_result.code() != ResultCode::OK) {
    return cql_result;
  }

  cql_result = _cql_client->execute_statement(CREATE_TABLE_COUNTRIES);

  return cql_result;
}

CqlResult CountryCqlManager::create_country(const CountryObject& country_data) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_insert_country.get()));

    cass_statement_bind_int32(statement.get(), 0, country_data._id);
    cass_statement_bind_string(statement.get(), 1, country_data._name.c_str());
    cass_statement_bind_string(statement.get(), 2, country_data._code.c_str());

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

std::pair<CqlResult, CountryObject> CountryCqlManager::get_country(
    const int country_id) {
  std::vector<CountryObject> countries;
  // Define the result handler that pre-allocate space in countries vector
  auto reserve_countries = [&countries](size_t num_rows) {
    // Reserve capacity for all returned countries
    countries.reserve(num_rows);
  };

  // Define the row handler that will store the countries
  auto copy_country_details = [&countries](const CassRow* row) {
    countries.emplace_back();
    CountryObject& country = countries.back();

    CqlResult cql_result = map_row_to_country(row, country);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_select_country.get()));
  cass_statement_bind_int32(statement.get(), 0, country_id);

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_countries, copy_country_details);

  if (cql_result.code() != ResultCode::OK) return {cql_result, CountryObject()};

  if (countries.size() != 1)
    return {{ResultCode::NOT_FOUND, "No entries found"}, CountryObject()};

  return {cql_result, countries.at(0)};
}

std::pair<CqlResult, std::vector<CountryObject>>
CountryCqlManager::get_all_countries() {
  std::vector<CountryObject> countries;
  // Define the result handler that pre-allocate space in countries vector
  auto reserve_countries = [&countries](size_t num_rows) {
    // Reserve capacity for all returned countries
    countries.reserve(num_rows);
  };

  // Define the row handler that will store the countries
  auto copy_country_details = [&countries](const CassRow* row) {
    countries.emplace_back();
    CountryObject& country = countries.back();

    CqlResult cql_result = map_row_to_country(row, country);
    if (cql_result.code() != ResultCode::OK) {
      return cql_result;
    }

    return CqlResult(ResultCode::OK);
  };

  CassStatementPtr statement(
      cass_prepared_bind(_prepared_select_all_countries.get()));

  CqlResult cql_result = _cql_client->select_rows(
      statement.get(), reserve_countries, copy_country_details);

  if (cql_result.code() != ResultCode::OK)
    return {cql_result, std::vector<CountryObject>()};

  return {cql_result, countries};
}

CqlResult CountryCqlManager::update_country(const int id,
                                            const std::string name,
                                            const std::string code) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_update_country.get()));

    cass_statement_bind_string(statement.get(), 0, name.c_str());
    cass_statement_bind_string(statement.get(), 1, code.c_str());
    cass_statement_bind_int32(statement.get(), 2, id);

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

CqlResult CountryCqlManager::delete_country(const int country_id) {
  std::string error_message;

  try {
    CassStatementPtr statement(
        cass_prepared_bind(_prepared_delete_country.get()));

    cass_statement_bind_int32(statement.get(), 0, country_id);

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

CqlResult map_row_to_country(const CassRow* row, CountryObject& country) {
  CqlResult cql_result = get_int_value(row, 0, country._id);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_text_value(row, 1, country._name);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  cql_result = get_text_value(row, 2, country._code);
  if (cql_result.code() != ResultCode::OK) {
    LOG_ERROR << static_cast<int>(cql_result.code()) << " | "
              << cql_result.error();
    return cql_result;
  }

  return {ResultCode::OK};
}