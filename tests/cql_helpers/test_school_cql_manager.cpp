/**
 * @file test_school_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for testing the school cql manager
 * @version 0.1
 * @date 2022-12-14
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <gtest/gtest.h>

#include "cql_helpers/school_cql_manager.hpp"

class TestSchoolCqlManager : public ::testing::Test {
 public:
  TestSchoolCqlManager(){};

  virtual ~TestSchoolCqlManager(){};

  const std::string CASSANDRA_IP = "127.0.0.1";

  CqlClient* _cql_client = nullptr;
  SchoolCqlManager* _school_cql_manager = nullptr;

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

    _school_cql_manager = new SchoolCqlManager(_cql_client);
    cql_result = _school_cql_manager->configure(true);

    if (cql_result.code() != ResultCode::OK) {
      LOG_ERROR << "Failed to configure school store: " << cql_result.error();
      delete _cql_client;
      _cql_client = nullptr;
      delete _school_cql_manager;
      _school_cql_manager = nullptr;

      return;
    }
  }

  void disconnect() {
    if (_cql_client != nullptr) {
      delete _cql_client;
      _cql_client = nullptr;
    }
    if (_school_cql_manager != nullptr) {
      delete _school_cql_manager;
      _school_cql_manager = nullptr;
    }
  }

  bool delete_schools() const {
    CqlResult cql_result =
        _cql_client->execute_statement("TRUNCATE TABLE environment.schools;");

    return cql_result.code() == ResultCode::OK;
  }
};

TEST_F(TestSchoolCqlManager, WriteSchool_Test) {
  if (_cql_client == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_schools()) {
    return;
  }

  SchoolObject school_object(1, "Test School", 1, "empty_path");

  CqlResult cql_result = _school_cql_manager->create_school(school_object);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);
}

TEST_F(TestSchoolCqlManager, ReadSchool_Test) {
  if (_cql_client == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_schools()) {
    return;
  }

  SchoolObject school_object(1, "Test School", 1, "empty_path");

  CqlResult cql_result = _school_cql_manager->create_school(school_object);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  std::pair<CqlResult, SchoolObject> result =
      _school_cql_manager->get_school(1);

  EXPECT_EQ(result.first.code(), ResultCode::OK);
  EXPECT_EQ(result.second._id, 1);
  EXPECT_EQ(result.second._name, "Test School");
  EXPECT_EQ(result.second._country_id, 1);
  EXPECT_EQ(result.second._image_path, "empty_path");
}

TEST_F(TestSchoolCqlManager, UpdateSchool_Test) {
  if (_cql_client == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_schools()) {
    return;
  }

  SchoolObject school_object(1, "Test School", 1, "empty_path");

  CqlResult cql_result = _school_cql_manager->create_school(school_object);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  school_object._name = "Test School 2";
  school_object._country_id = 2;
  school_object._image_path = "empty_path_2";

  cql_result = _school_cql_manager->update_school(
      school_object._id, school_object._name, school_object._country_id,
      school_object._image_path);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  std::pair<CqlResult, SchoolObject> result =
      _school_cql_manager->get_school(1);

  EXPECT_EQ(result.first.code(), ResultCode::OK);
  EXPECT_EQ(result.second._id, 1);
  EXPECT_EQ(result.second._name, "Test School 2");
  EXPECT_EQ(result.second._country_id, 2);
  EXPECT_EQ(result.second._image_path, "empty_path_2");
}

TEST_F(TestSchoolCqlManager, DeleteSchool_Test) {
  if (_cql_client == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_schools()) {
    return;
  }

  SchoolObject school_object(1, "Test School", 1, "empty_path");

  CqlResult cql_result = _school_cql_manager->create_school(school_object);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _school_cql_manager->delete_school(1);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  std::pair<CqlResult, SchoolObject> result =
      _school_cql_manager->get_school(1);

  EXPECT_EQ(result.first.code(), ResultCode::NOT_FOUND);
}

TEST_F(TestSchoolCqlManager, InsertSchoolTwice_Test) {
  if (_cql_client == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_schools()) {
    return;
  }

  SchoolObject school_object(1, "Test School", 1, "empty_path");

  CqlResult cql_result = _school_cql_manager->create_school(school_object);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _school_cql_manager->create_school(school_object);

  EXPECT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}

TEST_F(TestSchoolCqlManager, ReadSchools_Test) {
  if (_cql_client == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_schools()) {
    return;
  }

  SchoolObject school_object(1, "Test School", 1, "empty_path");

  CqlResult cql_result = _school_cql_manager->create_school(school_object);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  school_object._id = 2;
  school_object._name = "Test School 2";

  cql_result = _school_cql_manager->create_school(school_object);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  std::pair<CqlResult, std::vector<SchoolObject>> result =
      _school_cql_manager->get_all_schools();

  EXPECT_EQ(result.first.code(), ResultCode::OK);
  EXPECT_EQ(result.second.size(), 2);
  EXPECT_EQ(result.second[0]._id, 1);
  EXPECT_EQ(result.second[0]._name, "Test School");
  EXPECT_EQ(result.second[0]._country_id, 1);
  EXPECT_EQ(result.second[0]._image_path, "empty_path");
  EXPECT_EQ(result.second[1]._id, 2);
  EXPECT_EQ(result.second[1]._name, "Test School 2");
  EXPECT_EQ(result.second[1]._country_id, 1);
  EXPECT_EQ(result.second[1]._image_path, "empty_path");
}

TEST_F(TestSchoolCqlManager, ReadNonexistantSchool_Test) {
  if (_cql_client == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_schools()) {
    return;
  }

  std::pair<CqlResult, SchoolObject> result =
      _school_cql_manager->get_school(1);

  EXPECT_EQ(result.first.code(), ResultCode::NOT_FOUND);
}