/**
 * @file test_holiday_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for testing the holiday cql manager
 * @version 0.1
 * @date 2022-12-15
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <gtest/gtest.h>

#include "cql_helpers/holiday_cql_manager.hpp"

class TestHolidayCqlManager : public ::testing::Test {
 public:
  TestHolidayCqlManager(){};

  virtual ~TestHolidayCqlManager(){};

  const std::string CASSANDRA_IP = "127.0.0.1";

  CqlClient* _cql_client = nullptr;
  HolidayCqlManager* _holiday_cql_manager = nullptr;

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

    _holiday_cql_manager = new HolidayCqlManager(_cql_client);
    cql_result = _holiday_cql_manager->configure(true);

    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << "Failed to configure holiday store: " << cql_result.error();
      delete _cql_client;
      _cql_client = nullptr;
      delete _holiday_cql_manager;
      _holiday_cql_manager = nullptr;

      return;
    }
  }

  void disconnect() {
    if (_cql_client != nullptr) {
      delete _cql_client;
      _cql_client = nullptr;
    }

    if (_holiday_cql_manager != nullptr) {
      delete _holiday_cql_manager;
      _holiday_cql_manager = nullptr;
    }
  }

  bool delete_holidays() {
    CqlResult cql_result =
        _cql_client->execute_statement("TRUNCATE TABLE environment.holidays;");

    return cql_result.code() == ResultCode::OK;
  }
};

TEST_F(TestHolidayCqlManager, WriteHoliday_Test) {
  if (_cql_client == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_holidays()) {
    return;
  }

  HolidayObject holiday(1, HolidayType::NATIONAL, 19216801, "Test holiday");

  CqlResult cql_result = _holiday_cql_manager->create_holiday(holiday);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);
}

TEST_F(TestHolidayCqlManager, ReadHoliday_Test) {
  if (_cql_client == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_holidays()) {
    return;
  }

  HolidayObject holiday(1, HolidayType::NATIONAL, 19216801, "Test holiday");

  CqlResult cql_result = _holiday_cql_manager->create_holiday(holiday);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  std::pair<CqlResult, HolidayObject> result =
      _holiday_cql_manager->get_specific_holiday(1, HolidayType::NATIONAL,
                                                 19216801);

  EXPECT_EQ(result.first.code(), ResultCode::OK);
  EXPECT_EQ(result.second._country_or_school_id, holiday._country_or_school_id);
  EXPECT_EQ(result.second._type, holiday._type);
  EXPECT_EQ(result.second._date, holiday._date);
  EXPECT_EQ(result.second._name, holiday._name);
}

TEST_F(TestHolidayCqlManager, ReadHolidays_Test) {
  if (_cql_client == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_holidays()) {
    return;
  }

  HolidayObject holiday1(1, HolidayType::NATIONAL, 19216801,
                         "Wanted test holiday 1");
  HolidayObject holiday2(1, HolidayType::NATIONAL, 19216802,
                         "Wanted test holiday 2");
  HolidayObject holiday3(2, HolidayType::NATIONAL, 19216803,
                         "Unwanted test holiday 3");

  CqlResult cql_result = _holiday_cql_manager->create_holiday(holiday1);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _holiday_cql_manager->create_holiday(holiday2);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _holiday_cql_manager->create_holiday(holiday3);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  std::pair<CqlResult, std::vector<HolidayObject>> result =
      _holiday_cql_manager->get_holidays(1, HolidayType::NATIONAL);

  EXPECT_EQ(result.first.code(), ResultCode::OK);
  EXPECT_EQ(result.second.size(), 2);
  EXPECT_EQ(result.second[0]._country_or_school_id,
            holiday1._country_or_school_id);
  EXPECT_EQ(result.second[0]._type, holiday1._type);
  EXPECT_EQ(result.second[0]._date, holiday1._date);
  EXPECT_EQ(result.second[0]._name, holiday1._name);
  EXPECT_EQ(result.second[1]._country_or_school_id,
            holiday2._country_or_school_id);
  EXPECT_EQ(result.second[1]._type, holiday2._type);
  EXPECT_EQ(result.second[1]._date, holiday2._date);
  EXPECT_EQ(result.second[1]._name, holiday2._name);
}

TEST_F(TestHolidayCqlManager, UpdateHoliday_Test) {
  if (_cql_client == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_holidays()) {
    return;
  }

  HolidayObject holiday(1, HolidayType::NATIONAL, 19216801, "Test holiday");

  CqlResult cql_result = _holiday_cql_manager->create_holiday(holiday);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  HolidayObject holiday_copy;
  holiday_copy._country_or_school_id = holiday._country_or_school_id;
  holiday_copy._type = holiday._type;
  holiday_copy._name = "Updated test holiday";
  holiday_copy._date = 19216802;

  cql_result = _holiday_cql_manager->update_holiday(holiday_copy, holiday);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  std::pair<CqlResult, HolidayObject> result =
      _holiday_cql_manager->get_specific_holiday(1, HolidayType::NATIONAL,
                                                 19216802);

  EXPECT_EQ(result.first.code(), ResultCode::OK);
  EXPECT_EQ(result.second._country_or_school_id,
            holiday_copy._country_or_school_id);
  EXPECT_EQ(result.second._type, holiday_copy._type);
  EXPECT_EQ(result.second._date, holiday_copy._date);
  EXPECT_EQ(result.second._name, holiday_copy._name);
}

TEST_F(TestHolidayCqlManager, DeleteSpecificHoliday_Test) {
  if (_cql_client == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_holidays()) {
    return;
  }

  HolidayObject holiday(1, HolidayType::NATIONAL, 19216801, "Test holiday");

  CqlResult cql_result = _holiday_cql_manager->create_holiday(holiday);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _holiday_cql_manager->delete_specific_holiday(holiday);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  std::pair<CqlResult, HolidayObject> result =
      _holiday_cql_manager->get_specific_holiday(1, HolidayType::NATIONAL,
                                                 19216801);

  EXPECT_EQ(result.first.code(), ResultCode::NOT_FOUND);
}

TEST_F(TestHolidayCqlManager, DeleteMoreHolidays_Test) {
  if (_cql_client == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_holidays()) {
    return;
  }

  HolidayObject holiday1(1, HolidayType::NATIONAL, 19216801, "Test holiday 1");
  HolidayObject holiday2(1, HolidayType::NATIONAL, 19216802, "Test holiday 2");
  HolidayObject holiday3(1, HolidayType::NATIONAL, 19216803, "Test holiday 3");

  CqlResult cql_result = _holiday_cql_manager->create_holiday(holiday1);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _holiday_cql_manager->create_holiday(holiday2);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _holiday_cql_manager->create_holiday(holiday3);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _holiday_cql_manager->delete_holidays(1, HolidayType::NATIONAL);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  std::pair<CqlResult, std::vector<HolidayObject>> result =
      _holiday_cql_manager->get_holidays(1, HolidayType::NATIONAL);

  EXPECT_EQ(result.first.code(), ResultCode::OK);
  EXPECT_EQ(result.second.size(), 0);
}

TEST_F(TestHolidayCqlManager, InsertHolidayTwice_Test) {
  if (_cql_client == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_holidays()) {
    return;
  }

  HolidayObject holiday(1, HolidayType::NATIONAL, 19216801, "Test holiday");

  CqlResult cql_result = _holiday_cql_manager->create_holiday(holiday);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _holiday_cql_manager->create_holiday(holiday);

  EXPECT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}

TEST_F(TestHolidayCqlManager, ReadNonexistentHoliday_Test) {
  if (_cql_client == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_holidays()) {
    return;
  }

  std::pair<CqlResult, HolidayObject> result =
      _holiday_cql_manager->get_specific_holiday(1, HolidayType::NATIONAL,
                                                 19216801);

  EXPECT_EQ(result.first.code(), ResultCode::NOT_FOUND);
}
