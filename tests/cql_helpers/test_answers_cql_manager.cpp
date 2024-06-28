/**
 * @file test_answers_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for testing the answers by announcement or
 * question cql manager
 * @version 0.1
 * @date 2023-01-08
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <gtest/gtest.h>

#include "cql_helpers/answers_cql_manager.hpp"

class TestAnswersCqlManager : public ::testing::Test {
 public:
  TestAnswersCqlManager(){};

  virtual ~TestAnswersCqlManager(){};

  const std::string CASSANDRA_IP = "127.0.0.1";

  CqlClient* _cql_client = nullptr;
  AnswersCqlManager* _answers_cql_manager = nullptr;

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

    _answers_cql_manager = new AnswersCqlManager(_cql_client);
    cql_result = _answers_cql_manager->configure(true);

    if (cql_result.code() != ResultCode::OK) {
      delete _cql_client;
      _cql_client = nullptr;
      delete _answers_cql_manager;
      _answers_cql_manager = nullptr;

      return;
    }
  }

  void disconnect() {
    if (_cql_client != nullptr) {
      delete _cql_client;
      _cql_client = nullptr;
    }

    if (_answers_cql_manager != nullptr) {
      delete _answers_cql_manager;
      _answers_cql_manager = nullptr;
    }
  }

  bool delete_announcements() {
    CqlResult cql_result =
        _cql_client->execute_statement("TRUNCATE TABLE schools.answers;");

    return cql_result.code() == ResultCode::OK;
  }
};

TEST_F(TestAnswersCqlManager, WriteAnswer_Test) {
  if (_answers_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_announcements()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  AnswerObject answer_object(1, temp_uuid, 1, temp_uuid,
                             "This is a test answer");

  CqlResult cql_result = _answers_cql_manager->create_answer(answer_object);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);
}

TEST_F(TestAnswersCqlManager, ReadAnswer_Test) {
  if (_answers_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_announcements()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  time_t temp_time = time(nullptr);

  AnswerObject answer_object(1, temp_uuid, temp_time, temp_uuid,
                             "This is a test answer");

  CqlResult cql_result = _answers_cql_manager->create_answer(answer_object);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  AnswerObject answer_object_read;

  auto response = _answers_cql_manager->get_answer_by_id(1, temp_uuid);

  ASSERT_EQ(response.first.code(), ResultCode::OK);

  AnswerObject* read_answer = &response.second;

  ASSERT_EQ(read_answer->_school_id, answer_object._school_id);
  ASSERT_EQ(read_answer->_id.clock_seq_and_node,
            answer_object._id.clock_seq_and_node);
  ASSERT_EQ(read_answer->_id.time_and_version,
            answer_object._id.time_and_version);
  ASSERT_EQ(read_answer->_created_by.clock_seq_and_node,
            answer_object._created_by.clock_seq_and_node);
  ASSERT_EQ(read_answer->_created_by.time_and_version,
            answer_object._created_by.time_and_version);
  ASSERT_EQ(read_answer->_content, answer_object._content);
  ASSERT_EQ(read_answer->_created_at, temp_time);
}

TEST_F(TestAnswersCqlManager, DeleteAnswer_Test) {
  if (_answers_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_announcements()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  time_t temp_time = time(nullptr);

  AnswerObject answer_object(1, temp_uuid, temp_time, temp_uuid,
                             "This is a test answer");

  CqlResult cql_result = _answers_cql_manager->create_answer(answer_object);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _answers_cql_manager->delete_answer(1, temp_uuid, temp_time);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  AnswerObject answer_object_read;

  auto response = _answers_cql_manager->get_answer_by_id(1, temp_uuid);

  ASSERT_EQ(response.first.code(), ResultCode::NOT_FOUND);
}

TEST_F(TestAnswersCqlManager, InsertAnswerTwice_Test) {
  if (_answers_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_announcements()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  time_t temp_time = time(nullptr);

  AnswerObject answer_object(1, temp_uuid, temp_time, temp_uuid,
                             "This is a test answer");

  CqlResult cql_result = _answers_cql_manager->create_answer(answer_object);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _answers_cql_manager->create_answer(answer_object);

  ASSERT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}

TEST_F(TestAnswersCqlManager, ReadNonexistentAnswer_Test) {
  if (_answers_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_announcements()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  AnswerObject answer_object_read;

  auto response = _answers_cql_manager->get_answer_by_id(1, temp_uuid);

  ASSERT_EQ(response.first.code(), ResultCode::NOT_FOUND);
}

TEST_F(TestAnswersCqlManager, DeleteNonexistentAnswer_Test) {
  if (_answers_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_announcements()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  time_t temp_time = time(nullptr);

  CqlResult cql_result =
      _answers_cql_manager->delete_answer(1, temp_uuid, temp_time);

  ASSERT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}
