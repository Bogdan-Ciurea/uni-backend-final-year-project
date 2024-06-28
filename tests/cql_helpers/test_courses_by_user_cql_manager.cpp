/**
 * @file test_courses_by_user_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for testing the courses by user
 * @version 0.1
 * @date 2023-01-10
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <gtest/gtest.h>

#include "cql_helpers/courses_by_user_cql_manager.hpp"

class TestCoursesByUserCqlManager : public ::testing::Test {
 public:
  TestCoursesByUserCqlManager(){};

  virtual ~TestCoursesByUserCqlManager(){};

  const std::string CASSANDRA_IP = "127.0.0.1";

  CqlClient* _cql_client = nullptr;
  CoursesByUserCqlManager* _course_by_user_cql_manager = nullptr;

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

    _course_by_user_cql_manager = new CoursesByUserCqlManager(_cql_client);
    cql_result = _course_by_user_cql_manager->configure(true);

    if (cql_result.code() != ResultCode::OK) {
      delete _cql_client;
      _cql_client = nullptr;
      delete _course_by_user_cql_manager;
      _course_by_user_cql_manager = nullptr;

      return;
    }
  }

  void disconnect() {
    if (_cql_client != nullptr) {
      delete _cql_client;
      _cql_client = nullptr;
    }

    if (_course_by_user_cql_manager != nullptr) {
      delete _course_by_user_cql_manager;
      _course_by_user_cql_manager = nullptr;
    }
  }

  bool delete_relationships() {
    CqlResult cql_result = _cql_client->execute_statement(
        "TRUNCATE TABLE schools.courses_by_user;");

    return cql_result.code() == ResultCode::OK;
  }
};

TEST_F(TestCoursesByUserCqlManager, WriteRelationship_Test) {
  if (_course_by_user_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_relationships()) {
    return;
  }

  CqlResult cql_result = _course_by_user_cql_manager->create_relationship(
      1, create_current_uuid(), create_current_uuid());

  ASSERT_EQ(cql_result.code(), ResultCode::OK);
}

TEST_F(TestCoursesByUserCqlManager, ReadRelationship_Test) {
  if (_course_by_user_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_relationships()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  CassUuid temp_uuid_1 = create_current_uuid();
  CassUuid temp_uuid_2 = create_current_uuid();

  CqlResult cql_result = _course_by_user_cql_manager->create_relationship(
      1, temp_uuid, temp_uuid_1);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _course_by_user_cql_manager->create_relationship(1, temp_uuid,
                                                                temp_uuid_2);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  auto response =
      _course_by_user_cql_manager->get_courses_by_user(1, temp_uuid);

  ASSERT_EQ(response.first.code(), ResultCode::OK);
  ASSERT_EQ(response.second.size(), 2);

  for (auto uuid : response.second) {
    ASSERT_TRUE(uuid.clock_seq_and_node == temp_uuid_1.clock_seq_and_node ||
                uuid.clock_seq_and_node == temp_uuid_2.clock_seq_and_node);
  }
}

TEST_F(TestCoursesByUserCqlManager, DeleteRelationship_Test) {
  if (_course_by_user_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_relationships()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  CassUuid temp_uuid_1 = create_current_uuid();

  CqlResult cql_result = _course_by_user_cql_manager->create_relationship(
      1, temp_uuid, temp_uuid_1);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _course_by_user_cql_manager->delete_relationship(1, temp_uuid,
                                                                temp_uuid_1);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);
}

TEST_F(TestCoursesByUserCqlManager, DeleteRelationshipsByUser_Test) {
  if (_course_by_user_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_relationships()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  CassUuid temp_uuid_1 = create_current_uuid();
  CassUuid temp_uuid_2 = create_current_uuid();

  CqlResult cql_result = _course_by_user_cql_manager->create_relationship(
      1, temp_uuid, temp_uuid_1);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _course_by_user_cql_manager->create_relationship(1, temp_uuid,
                                                                temp_uuid_2);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _course_by_user_cql_manager->delete_all_relationships_of_user(
      1, temp_uuid);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  auto response =
      _course_by_user_cql_manager->get_courses_by_user(1, temp_uuid);

  ASSERT_EQ(response.first.code(), ResultCode::NOT_FOUND);
  ASSERT_EQ(response.second.size(), 0);
}

TEST_F(TestCoursesByUserCqlManager, InsertRelationshipTwice_Test) {
  if (_course_by_user_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_relationships()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  CassUuid temp_uuid_1 = create_current_uuid();

  CqlResult cql_result = _course_by_user_cql_manager->create_relationship(
      1, temp_uuid, temp_uuid_1);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _course_by_user_cql_manager->create_relationship(1, temp_uuid,
                                                                temp_uuid_1);

  ASSERT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}

TEST_F(TestCoursesByUserCqlManager, ReadNonexistentRelationship_Test) {
  if (_course_by_user_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_relationships()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  CassUuid temp_uuid_1 = create_current_uuid();

  auto response =
      _course_by_user_cql_manager->get_courses_by_user(1, temp_uuid);

  ASSERT_EQ(response.first.code(), ResultCode::NOT_FOUND);
  ASSERT_EQ(response.second.size(), 0);
}

TEST_F(TestCoursesByUserCqlManager, DeleteNonexistentRelationship_Test) {
  if (_course_by_user_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_relationships()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  CassUuid temp_uuid_1 = create_current_uuid();

  CqlResult cql_result = _course_by_user_cql_manager->delete_relationship(
      1, temp_uuid, temp_uuid_1);

  ASSERT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}
