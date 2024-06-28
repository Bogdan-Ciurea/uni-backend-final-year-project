/**
 * @file test_country_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for testing the country cql manager
 * @version 0.1
 * @date 2022-12-15
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <gtest/gtest.h>

#include "cql_helpers/country_cql_manager.hpp"

class TestCountryCqlManager : public ::testing::Test {
 public:
  TestCountryCqlManager(){};

  virtual ~TestCountryCqlManager(){};

  const std::string CASSANDRA_IP = "127.0.0.1";

  CqlClient* _cql_client = nullptr;
  CountryCqlManager* _country_cql_manager = nullptr;

  virtual void SetUp() { connect(); }
  virtual void TearDown() { disconnect(); }

  void connect() {
    if (CASSANDRA_IP.empty()) {
      // Test case is disabled
      return;
    }

    _cql_client = new CqlClient(CASSANDRA_IP, 9042);
    CqlResult cql_result = _cql_client->connect();

    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << "Failed to initialize Cassandra connection: "
                << cql_result.error();
      delete _cql_client;
      _cql_client = nullptr;

      return;
    }

    _country_cql_manager = new CountryCqlManager(_cql_client);
    cql_result = _country_cql_manager->configure(true);

    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << "Failed to configure country store: " << cql_result.error();
      delete _cql_client;
      _cql_client = nullptr;
      delete _country_cql_manager;
      _country_cql_manager = nullptr;

      return;
    }
  }

  void disconnect() {
    if (_cql_client != nullptr) {
      delete _cql_client;
      _cql_client = nullptr;
    }

    if (_country_cql_manager != nullptr) {
      delete _country_cql_manager;
      _country_cql_manager = nullptr;
    }
  }

  bool delete_countries() {
    CqlResult cql_result =
        _cql_client->execute_statement("TRUNCATE TABLE environment.countries;");

    return cql_result.code() == ResultCode::OK;
  }
};

TEST_F(TestCountryCqlManager, WriteCountry_Test) {
  if (_country_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_countries()) {
    return;
  }

  CountryObject country(1, "Romania", "RO");

  CqlResult cql_result = _country_cql_manager->create_country(country);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);
}

TEST_F(TestCountryCqlManager, ReadCountry_Test) {
  if (_country_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_countries()) {
    return;
  }

  CountryObject country(1, "Romania", "RO");

  CqlResult cql_result = _country_cql_manager->create_country(country);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  CountryObject country_read;

  std::pair<CqlResult, CountryObject> result =
      _country_cql_manager->get_country(country._id);

  ASSERT_EQ(result.first.code(), ResultCode::OK);
  ASSERT_EQ(result.second._id, country._id);
  ASSERT_EQ(result.second._name, country._name);
  ASSERT_EQ(result.second._code, country._code);
}

TEST_F(TestCountryCqlManager, ReadCountries_Test) {
  if (_country_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_countries()) {
    return;
  }

  CountryObject country1(1, "Romania", "RO");
  CountryObject country2(2, "Bulgaria", "BG");
  CountryObject country3(3, "Greece", "GR");

  CqlResult cql_result = _country_cql_manager->create_country(country1);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _country_cql_manager->create_country(country2);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _country_cql_manager->create_country(country3);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  std::vector<CountryObject> countries;

  std::pair<CqlResult, std::vector<CountryObject>> result =
      _country_cql_manager->get_all_countries();

  ASSERT_EQ(result.first.code(), ResultCode::OK);

  countries = result.second;

  ASSERT_EQ(countries.size(), 3);

  ASSERT_EQ(countries[0]._id, country1._id);
  ASSERT_EQ(countries[0]._name, country1._name);
  ASSERT_EQ(countries[0]._code, country1._code);

  ASSERT_EQ(countries[1]._id, country2._id);
  ASSERT_EQ(countries[1]._name, country2._name);
  ASSERT_EQ(countries[1]._code, country2._code);

  ASSERT_EQ(countries[2]._id, country3._id);
  ASSERT_EQ(countries[2]._name, country3._name);
  ASSERT_EQ(countries[2]._code, country3._code);
}

TEST_F(TestCountryCqlManager, UpdateCountry_Test) {
  if (_country_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_countries()) {
    return;
  }

  CountryObject country(1, "Romania", "RO");

  CqlResult cql_result = _country_cql_manager->create_country(country);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  country._name = "Romania2";
  country._code = "RO2";

  cql_result = _country_cql_manager->update_country(country._id, country._name,
                                                    country._code);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  CountryObject country_read;

  std::pair<CqlResult, CountryObject> result =
      _country_cql_manager->get_country(country._id);

  ASSERT_EQ(result.first.code(), ResultCode::OK);
  ASSERT_EQ(result.second._id, country._id);
  ASSERT_EQ(result.second._name, country._name);
  ASSERT_EQ(result.second._code, country._code);
}

TEST_F(TestCountryCqlManager, DeleteCountry_Test) {
  if (_country_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_countries()) {
    return;
  }

  CountryObject country(1, "Romania", "RO");

  CqlResult cql_result = _country_cql_manager->create_country(country);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _country_cql_manager->delete_country(country._id);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);
}

TEST_F(TestCountryCqlManager, InsertCountryTwice_Test) {
  if (_country_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_countries()) {
    return;
  }

  CountryObject country(1, "Romania", "RO");

  CqlResult cql_result = _country_cql_manager->create_country(country);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _country_cql_manager->create_country(country);

  ASSERT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}

TEST_F(TestCountryCqlManager, ReadNonexistentCountry_Test) {
  if (_country_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_countries()) {
    return;
  }

  CountryObject country_read;

  std::pair<CqlResult, CountryObject> result =
      _country_cql_manager->get_country(1);

  ASSERT_EQ(result.first.code(), ResultCode::NOT_FOUND);
}
