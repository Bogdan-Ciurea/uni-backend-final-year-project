/**
 * @file test_questions_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for testing the questions cql manager
 * @version 0.1
 * @date 2023-01-11
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <gtest/gtest.h>

#include <cql_helpers/questions_cql_manager.hpp>

class TestQuestionsCqlManager : public ::testing::Test {
 public:
  TestQuestionsCqlManager(){};

  virtual ~TestQuestionsCqlManager(){};

  const std::string CASSANDRA_IP = "127.0.0.1";

  CqlClient* _cql_client = nullptr;
  QuestionsCqlManager* _questions_cql_manager = nullptr;

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

    _questions_cql_manager = new QuestionsCqlManager(_cql_client);
    cql_result = _questions_cql_manager->configure(true);

    if (cql_result.code() != ResultCode::OK) {
      delete _cql_client;
      _cql_client = nullptr;
      delete _questions_cql_manager;
      _questions_cql_manager = nullptr;

      return;
    }
  }

  void disconnect() {
    if (_cql_client != nullptr) {
      delete _cql_client;
      _cql_client = nullptr;
    }

    if (_questions_cql_manager != nullptr) {
      delete _questions_cql_manager;
      _questions_cql_manager = nullptr;
    }
  }

  bool delete_questions() {
    CqlResult cql_result =
        _cql_client->execute_statement("TRUNCATE TABLE schools.questions;");

    return cql_result.code() == ResultCode::OK;
  }
};

TEST_F(TestQuestionsCqlManager, WriteQuestion_Test) {
  if (_questions_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_questions()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  QuestionObject temp_question(1, temp_uuid, "Test question", time(nullptr),
                               temp_uuid);

  CqlResult cql_result = _questions_cql_manager->create_question(temp_question);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);
}

TEST_F(TestQuestionsCqlManager, ReadQuestion_Test) {
  if (_questions_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_questions()) {
    return;
  }

  time_t temp_time = time(nullptr);
  CassUuid temp_uuid1 = create_current_uuid();
  CassUuid temp_uuid2 = create_current_uuid();

  QuestionObject temp_question(1, temp_uuid1, "Test question", temp_time,
                               temp_uuid2);

  CqlResult cql_result = _questions_cql_manager->create_question(temp_question);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_answer, question] =
      _questions_cql_manager->get_question_by_id(1, temp_uuid1);

  EXPECT_EQ(cql_answer.code(), ResultCode::OK);
  EXPECT_EQ(question._school_id, 1);
  EXPECT_EQ(question._question_id.clock_seq_and_node,
            temp_uuid1.clock_seq_and_node);
  EXPECT_EQ(question._text, "Test question");
  EXPECT_EQ(question._time_added, temp_time);
  EXPECT_EQ(question._added_by_user_id.clock_seq_and_node,
            temp_uuid2.clock_seq_and_node);
}

TEST_F(TestQuestionsCqlManager, UpdateQuestion_Test) {
  if (_questions_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_questions()) {
    return;
  }

  time_t temp_time = time(nullptr);
  CassUuid temp_uuid1 = create_current_uuid();
  CassUuid temp_uuid2 = create_current_uuid();
  CassUuid temp_uuid3 = create_current_uuid();

  QuestionObject temp_question(1, temp_uuid1, "Test question", temp_time,
                               temp_uuid2);

  CqlResult cql_result = _questions_cql_manager->create_question(temp_question);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  temp_question._text = "Test question updated";
  temp_question._time_added = temp_time + 1000;

  cql_result = _questions_cql_manager->update_question(
      1, temp_uuid1, temp_question._text, temp_question._time_added,
      temp_uuid3);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_answer2, question2] =
      _questions_cql_manager->get_question_by_id(1, temp_uuid1);

  EXPECT_EQ(cql_answer2.code(), ResultCode::OK);
  EXPECT_EQ(question2._school_id, 1);
  EXPECT_EQ(question2._question_id.clock_seq_and_node,
            temp_uuid1.clock_seq_and_node);
  EXPECT_EQ(question2._text, "Test question updated");
  EXPECT_EQ(question2._time_added, temp_time + 1000);
  EXPECT_EQ(question2._added_by_user_id.clock_seq_and_node,
            temp_uuid3.clock_seq_and_node);
}

TEST_F(TestQuestionsCqlManager, DeleteQuestion_Test) {
  if (_questions_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_questions()) {
    return;
  }

  time_t temp_time = time(nullptr);
  CassUuid temp_uuid1 = create_current_uuid();
  CassUuid temp_uuid2 = create_current_uuid();

  QuestionObject temp_question(1, temp_uuid1, "Test question", temp_time,
                               temp_uuid2);

  CqlResult cql_result = _questions_cql_manager->create_question(temp_question);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _questions_cql_manager->delete_question(1, temp_uuid1);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_answer, question] =
      _questions_cql_manager->get_question_by_id(1, temp_uuid1);

  EXPECT_EQ(cql_answer.code(), ResultCode::NOT_FOUND);
}

TEST_F(TestQuestionsCqlManager, InsertQuestionsTwice_Test) {
  if (_questions_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_questions()) {
    return;
  }

  time_t temp_time = time(nullptr);
  CassUuid temp_uuid1 = create_current_uuid();
  CassUuid temp_uuid2 = create_current_uuid();

  QuestionObject temp_question(1, temp_uuid1, "Test question", temp_time,
                               temp_uuid2);

  CqlResult cql_result = _questions_cql_manager->create_question(temp_question);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _questions_cql_manager->create_question(temp_question);

  EXPECT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}

TEST_F(TestQuestionsCqlManager, ReadNonexistentQuestions_Test) {
  if (_questions_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_questions()) {
    return;
  }

  auto [cql_answer, question] =
      _questions_cql_manager->get_question_by_id(1, create_current_uuid());

  EXPECT_EQ(cql_answer.code(), ResultCode::NOT_FOUND);
}

TEST_F(TestQuestionsCqlManager, DeleteNonexistentQuestions_Test) {
  if (_questions_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_questions()) {
    return;
  }

  CqlResult cql_result =
      _questions_cql_manager->delete_question(1, create_current_uuid());

  EXPECT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}