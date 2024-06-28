/**
 * @file test_users_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for testing the users cql manager
 * @version 0.1
 * @date 2023-01-11
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <gtest/gtest.h>

#include "cql_helpers/users_cql_manager.hpp"

class TestUsersCqlManager : public ::testing::Test {
 public:
  TestUsersCqlManager(){};

  virtual ~TestUsersCqlManager(){};

  const std::string CASSANDRA_IP = "127.0.0.1";

  CqlClient* _cql_client = nullptr;
  UsersCqlManager* _users_cql_manager = nullptr;

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

    _users_cql_manager = new UsersCqlManager(_cql_client);
    cql_result = _users_cql_manager->configure(true);

    if (cql_result.code() != ResultCode::OK) {
      delete _cql_client;
      _cql_client = nullptr;
      delete _users_cql_manager;
      _users_cql_manager = nullptr;

      return;
    }
  }

  void disconnect() {
    if (_cql_client != nullptr) {
      delete _cql_client;
      _cql_client = nullptr;
    }

    if (_users_cql_manager != nullptr) {
      delete _users_cql_manager;
      _users_cql_manager = nullptr;
    }
  }

  bool delete_users() {
    CqlResult cql_result =
        _cql_client->execute_statement("TRUNCATE TABLE schools.users;");

    return cql_result.code() == ResultCode::OK;
  }
};

TEST_F(TestUsersCqlManager, WriteUser_Test) {
  if (_users_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_users()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  UserObject temp_user(1, temp_uuid, "test email", "test_password",
                       UserType::STUDENT, true, "Test First Name",
                       "Test last name", "+07something", time(nullptr));

  CqlResult cql_result = _users_cql_manager->create_user(temp_user);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);
}

TEST_F(TestUsersCqlManager, ReadUser_Test) {
  if (_users_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_users()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  UserObject temp_user(1, temp_uuid, "test email", "test_password",
                       UserType::STUDENT, true, "Test First Name",
                       "Test last name", "+07something", time(nullptr));

  CqlResult cql_result = _users_cql_manager->create_user(temp_user);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [read_cql_result, read_user] =
      _users_cql_manager->get_user(1, temp_uuid);

  EXPECT_EQ(read_cql_result.code(), ResultCode::OK);

  EXPECT_EQ(read_user._school_id, temp_user._school_id);
  EXPECT_EQ(read_user._user_id.clock_seq_and_node,
            temp_user._user_id.clock_seq_and_node);
  EXPECT_EQ(read_user._email, temp_user._email);
  EXPECT_EQ(read_user._password, temp_user._password);
  EXPECT_EQ(read_user._user_type, temp_user._user_type);
  EXPECT_EQ(read_user._changed_password, temp_user._changed_password);
  EXPECT_EQ(read_user._first_name, temp_user._first_name);
  EXPECT_EQ(read_user._last_name, temp_user._last_name);
  EXPECT_EQ(read_user._phone_number, temp_user._phone_number);
  EXPECT_EQ(read_user._last_time_online, temp_user._last_time_online);
}

TEST_F(TestUsersCqlManager, ReadUserByEmailAndPassword_Test) {
  if (_users_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_users()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  UserObject temp_user(1, temp_uuid, "test email", "test_password",
                       UserType::STUDENT, true, "Test First Name",
                       "Test last name", "+07something", time(nullptr));

  CqlResult cql_result = _users_cql_manager->create_user(temp_user);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [read_cql_result, read_user] =
      _users_cql_manager->get_user_by_email(1, "test email");

  EXPECT_EQ(read_cql_result.code(), ResultCode::OK);

  EXPECT_EQ(read_user._school_id, temp_user._school_id);
  EXPECT_EQ(read_user._user_id.clock_seq_and_node,
            temp_user._user_id.clock_seq_and_node);
  EXPECT_EQ(read_user._email, temp_user._email);
  EXPECT_EQ(read_user._password, temp_user._password);
  EXPECT_EQ(read_user._user_type, temp_user._user_type);
  EXPECT_EQ(read_user._changed_password, temp_user._changed_password);
  EXPECT_EQ(read_user._first_name, temp_user._first_name);
  EXPECT_EQ(read_user._last_name, temp_user._last_name);
  EXPECT_EQ(read_user._phone_number, temp_user._phone_number);
  EXPECT_EQ(read_user._last_time_online, temp_user._last_time_online);
}

TEST_F(TestUsersCqlManager, UpdateUser_Test) {
  if (_users_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_users()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  UserObject temp_user(1, temp_uuid, "test email", "test_password",
                       UserType::STUDENT, true, "Test First Name",
                       "Test last name", "+07something", time(nullptr));

  CqlResult cql_result = _users_cql_manager->create_user(temp_user);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  temp_user._user_type = UserType::TEACHER;
  temp_user._changed_password = false;
  temp_user._first_name = "New First Name";
  temp_user._last_name = "New Last Name";
  temp_user._phone_number = "+07something else";

  cql_result = _users_cql_manager->update_user(
      1, temp_user._user_id, temp_user._email, temp_user._password,
      temp_user._user_type, temp_user._changed_password, temp_user._first_name,
      temp_user._last_name, temp_user._phone_number,
      temp_user._last_time_online);

  auto [read_cql_result, read_user] =
      _users_cql_manager->get_user(1, temp_uuid);

  EXPECT_EQ(read_cql_result.code(), ResultCode::OK);

  EXPECT_EQ(read_user._school_id, temp_user._school_id);
  EXPECT_EQ(read_user._user_id.clock_seq_and_node,
            temp_user._user_id.clock_seq_and_node);
  EXPECT_EQ(read_user._email, temp_user._email);
  EXPECT_EQ(read_user._password, temp_user._password);
  EXPECT_EQ(read_user._user_type, temp_user._user_type);
  EXPECT_EQ(read_user._changed_password, temp_user._changed_password);
  EXPECT_EQ(read_user._first_name, temp_user._first_name);
  EXPECT_EQ(read_user._last_name, temp_user._last_name);
  EXPECT_EQ(read_user._phone_number, temp_user._phone_number);
  EXPECT_EQ(read_user._last_time_online, temp_user._last_time_online);
}

TEST_F(TestUsersCqlManager, DeleteUser_Test) {
  if (_users_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_users()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  UserObject temp_user(1, temp_uuid, "test email", "test_password",
                       UserType::STUDENT, true, "Test First Name",
                       "Test last name", "+07something", time(nullptr));

  CqlResult cql_result = _users_cql_manager->create_user(temp_user);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _users_cql_manager->delete_user(1, temp_uuid);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [read_cql_result, read_user] =
      _users_cql_manager->get_user(1, temp_uuid);

  EXPECT_EQ(read_cql_result.code(), ResultCode::NOT_FOUND);
}

TEST_F(TestUsersCqlManager, InsertUsersTwice_Test) {
  if (_users_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_users()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  UserObject temp_user(1, temp_uuid, "test email", "test_password",
                       UserType::STUDENT, true, "Test First Name",
                       "Test last name", "+07something", time(nullptr));

  CqlResult cql_result = _users_cql_manager->create_user(temp_user);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _users_cql_manager->create_user(temp_user);

  EXPECT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}

TEST_F(TestUsersCqlManager, ReadNonexistentUsers_Test) {
  if (_users_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_users()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  UserObject temp_user(1, temp_uuid, "test email", "test_password",
                       UserType::STUDENT, true, "Test First Name",
                       "Test last name", "+07something", time(nullptr));

  auto [read_cql_result, read_user] =
      _users_cql_manager->get_user(1, temp_uuid);

  EXPECT_EQ(read_cql_result.code(), ResultCode::NOT_FOUND);
}

TEST_F(TestUsersCqlManager, DeleteNonexistentUsers_Test) {
  if (_users_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_users()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  UserObject temp_user(1, temp_uuid, "test email", "test_password",
                       UserType::STUDENT, true, "Test First Name",
                       "Test last name", "+07something", time(nullptr));

  CqlResult cql_result = _users_cql_manager->delete_user(1, temp_uuid);

  EXPECT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}