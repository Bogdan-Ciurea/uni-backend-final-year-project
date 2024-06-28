/**
 * @file test_users_by_course_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for testing the users by course cql manager
 * @version 0.1
 * @date 2023-01-11
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <gtest/gtest.h>

#include "cql_helpers/users_by_course_cql_manager.hpp"

class TestUsersByCourseCqlManager : public ::testing::Test {
 public:
  TestUsersByCourseCqlManager(){};

  virtual ~TestUsersByCourseCqlManager(){};

  const std::string CASSANDRA_IP = "127.0.0.1";

  CqlClient* _cql_client = nullptr;
  UsersByCourseCqlManager* _users_by_course_cql_manager = nullptr;

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

    _users_by_course_cql_manager = new UsersByCourseCqlManager(_cql_client);
    cql_result = _users_by_course_cql_manager->configure(true);

    if (cql_result.code() != ResultCode::OK) {
      delete _cql_client;
      _cql_client = nullptr;
      delete _users_by_course_cql_manager;
      _users_by_course_cql_manager = nullptr;

      return;
    }
  }

  void disconnect() {
    if (_cql_client != nullptr) {
      delete _cql_client;
      _cql_client = nullptr;
    }

    if (_users_by_course_cql_manager != nullptr) {
      delete _users_by_course_cql_manager;
      _users_by_course_cql_manager = nullptr;
    }
  }

  bool delete_questions_by_course() {
    CqlResult cql_result = _cql_client->execute_statement(
        "TRUNCATE TABLE schools.users_by_course;");

    return cql_result.code() == ResultCode::OK;
  }
};

TEST_F(TestUsersByCourseCqlManager, WriteRelationship_Test) {
  if (_users_by_course_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_questions_by_course()) {
    return;
  }

  CqlResult cql_result = _users_by_course_cql_manager->create_relationship(
      1, create_current_uuid(), create_current_uuid());

  ASSERT_EQ(cql_result.code(), ResultCode::OK);
}

TEST_F(TestUsersByCourseCqlManager, WriteMultipleRelationships_Test) {
  if (_users_by_course_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_questions_by_course()) {
    return;
  }

  for (int i = 0; i < 10; i++) {
    CqlResult cql_result = _users_by_course_cql_manager->create_relationship(
        1, create_current_uuid(), create_current_uuid());

    ASSERT_EQ(cql_result.code(), ResultCode::OK);
  }
}

TEST_F(TestUsersByCourseCqlManager, ReadRelationship_Test) {
  if (_users_by_course_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_questions_by_course()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  CqlResult cql_result = _users_by_course_cql_manager->create_relationship(
      1, temp_uuid, temp_uuid);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  auto result = _users_by_course_cql_manager->get_users_by_course(1, temp_uuid);

  ASSERT_EQ(result.first.code(), ResultCode::OK);

  ASSERT_EQ(result.second.size(), 1);
  ASSERT_EQ(result.second.at(0).clock_seq_and_node,
            temp_uuid.clock_seq_and_node);
}

TEST_F(TestUsersByCourseCqlManager, DeleteRelationship_Test) {
  if (_users_by_course_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_questions_by_course()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  CqlResult cql_result = _users_by_course_cql_manager->create_relationship(
      1, temp_uuid, temp_uuid);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _users_by_course_cql_manager->delete_relationship(1, temp_uuid,
                                                                 temp_uuid);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_read_result, questions] =
      _users_by_course_cql_manager->get_users_by_course(1, temp_uuid);

  ASSERT_EQ(cql_read_result.code(), ResultCode::NOT_FOUND);

  ASSERT_EQ(questions.size(), 0);
}

TEST_F(TestUsersByCourseCqlManager, DeleteRelationshipsByTag_Test) {
  if (_users_by_course_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_questions_by_course()) {
    return;
  }

  CassUuid temp_uuid_1 = create_current_uuid();
  CassUuid temp_uuid_2 = create_current_uuid();

  CqlResult cql_result = _users_by_course_cql_manager->create_relationship(
      1, temp_uuid_1, temp_uuid_1);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _users_by_course_cql_manager->create_relationship(1, temp_uuid_1,
                                                                 temp_uuid_2);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _users_by_course_cql_manager->create_relationship(1, temp_uuid_2,
                                                                 temp_uuid_2);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _users_by_course_cql_manager->delete_relationships_by_course(
      1, temp_uuid_1);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  auto result =
      _users_by_course_cql_manager->get_users_by_course(1, temp_uuid_2);

  ASSERT_EQ(result.first.code(), ResultCode::OK);

  ASSERT_EQ(result.second.size(), 1);
  ASSERT_EQ(result.second.at(0).clock_seq_and_node,
            temp_uuid_2.clock_seq_and_node);
}

TEST_F(TestUsersByCourseCqlManager, InsertRelationshipTwice_Test) {
  if (_users_by_course_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_questions_by_course()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  CqlResult cql_result = _users_by_course_cql_manager->create_relationship(
      1, temp_uuid, temp_uuid);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _users_by_course_cql_manager->create_relationship(1, temp_uuid,
                                                                 temp_uuid);

  ASSERT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}

TEST_F(TestUsersByCourseCqlManager, ReadNonexistentRelationship_Test) {
  if (_users_by_course_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_questions_by_course()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  auto result = _users_by_course_cql_manager->get_users_by_course(1, temp_uuid);

  ASSERT_EQ(result.first.code(), ResultCode::NOT_FOUND);

  ASSERT_EQ(result.second.size(), 0);
}

TEST_F(TestUsersByCourseCqlManager, DeleteNonexistentRelationship_Test) {
  if (_users_by_course_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_questions_by_course()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  CqlResult cql_result = _users_by_course_cql_manager->delete_relationship(
      1, temp_uuid, temp_uuid);

  ASSERT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}
