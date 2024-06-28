/**
 * @file test_answers_by_announcement_or_question_cql_manager.cpp
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

#include "cql_helpers/answers_by_announcement_or_question_cql_manager.hpp"

class TestAnswersByAnnOrQuestionCqlManager : public ::testing::Test {
 public:
  TestAnswersByAnnOrQuestionCqlManager(){};

  virtual ~TestAnswersByAnnOrQuestionCqlManager(){};

  const std::string CASSANDRA_IP = "127.0.0.1";

  CqlClient* _cql_client = nullptr;
  AnswersByAnnouncementOrQuestionCqlManager* _answer_by_ann_or_que_cql_manager =
      nullptr;

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

    _answer_by_ann_or_que_cql_manager =
        new AnswersByAnnouncementOrQuestionCqlManager(_cql_client);
    cql_result = _answer_by_ann_or_que_cql_manager->configure(true);

    if (cql_result.code() != ResultCode::OK) {
      delete _cql_client;
      _cql_client = nullptr;
      delete _answer_by_ann_or_que_cql_manager;
      _answer_by_ann_or_que_cql_manager = nullptr;

      return;
    }
  }

  void disconnect() {
    if (_cql_client != nullptr) {
      delete _cql_client;
      _cql_client = nullptr;
    }

    if (_answer_by_ann_or_que_cql_manager != nullptr) {
      delete _answer_by_ann_or_que_cql_manager;
      _answer_by_ann_or_que_cql_manager = nullptr;
    }
  }

  bool delete_relationships() {
    CqlResult cql_result = _cql_client->execute_statement(
        "TRUNCATE TABLE schools.answers_by_announcement_or_question;");

    return cql_result.code() == ResultCode::OK;
  }
};

TEST_F(TestAnswersByAnnOrQuestionCqlManager, WriteRelationship_Test) {
  if (_answer_by_ann_or_que_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_relationships()) {
    return;
  }

  CqlResult cql_result = _answer_by_ann_or_que_cql_manager->create_relationship(
      1, create_current_uuid(), 1, create_current_uuid());

  ASSERT_EQ(cql_result.code(), ResultCode::OK);
}

TEST_F(TestAnswersByAnnOrQuestionCqlManager, ReadRelationship_Test) {
  if (_answer_by_ann_or_que_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_relationships()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  CqlResult cql_result = _answer_by_ann_or_que_cql_manager->create_relationship(
      1, temp_uuid, 1, create_current_uuid());

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  auto answers = _answer_by_ann_or_que_cql_manager
                     ->get_answers_by_announcement_or_question(1, temp_uuid, 1);

  ASSERT_EQ(answers.first.code(), ResultCode::OK);
  ASSERT_EQ(answers.second.size(), 1);
}

TEST_F(TestAnswersByAnnOrQuestionCqlManager, DeleteRelationship_Test) {
  if (_answer_by_ann_or_que_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_relationships()) {
    return;
  }

  CassUuid temp_uuid1 = create_current_uuid();
  CassUuid temp_uuid2 = create_current_uuid();

  CqlResult cql_result = _answer_by_ann_or_que_cql_manager->create_relationship(
      1, temp_uuid1, 1, temp_uuid2);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _answer_by_ann_or_que_cql_manager->delete_relationship(
      1, temp_uuid1, 1, temp_uuid2);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);
}

TEST_F(TestAnswersByAnnOrQuestionCqlManager,
       DeleteRelationshipsByAnnouncement_Test) {
  if (_answer_by_ann_or_que_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_relationships()) {
    return;
  }

  CassUuid temp_uuid1 = create_current_uuid();
  CassUuid temp_uuid2 = create_current_uuid();

  CqlResult cql_result = _answer_by_ann_or_que_cql_manager->create_relationship(
      1, temp_uuid1, 0, temp_uuid1);
  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _answer_by_ann_or_que_cql_manager->create_relationship(
      1, temp_uuid2, 0, temp_uuid2);
  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _answer_by_ann_or_que_cql_manager->create_relationship(
      1, temp_uuid1, 0, temp_uuid2);
  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result =
      _answer_by_ann_or_que_cql_manager
          ->delete_relationships_by_announcement_or_question(1, temp_uuid1, 0);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  auto answers =
      _answer_by_ann_or_que_cql_manager
          ->get_answers_by_announcement_or_question(1, temp_uuid2, 0);
  ASSERT_EQ(answers.first.code(), ResultCode::OK);
  ASSERT_EQ(answers.second.size(), 1);
  ASSERT_EQ(answers.second[0].clock_seq_and_node,
            temp_uuid2.clock_seq_and_node);
}

TEST_F(TestAnswersByAnnOrQuestionCqlManager,
       DeleteRelationshipsByQuestion_Test) {
  if (_answer_by_ann_or_que_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_relationships()) {
    return;
  }

  CassUuid temp_uuid1 = create_current_uuid();
  CassUuid temp_uuid2 = create_current_uuid();

  CqlResult cql_result = _answer_by_ann_or_que_cql_manager->create_relationship(
      1, temp_uuid1, 1, temp_uuid1);
  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _answer_by_ann_or_que_cql_manager->create_relationship(
      1, temp_uuid2, 1, temp_uuid2);
  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _answer_by_ann_or_que_cql_manager->create_relationship(
      1, temp_uuid1, 1, temp_uuid2);
  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result =
      _answer_by_ann_or_que_cql_manager
          ->delete_relationships_by_announcement_or_question(1, temp_uuid1, 1);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  auto answers =
      _answer_by_ann_or_que_cql_manager
          ->get_answers_by_announcement_or_question(1, temp_uuid2, 1);
  ASSERT_EQ(answers.first.code(), ResultCode::OK);
  ASSERT_EQ(answers.second.size(), 1);
  ASSERT_EQ(answers.second[0].clock_seq_and_node,
            temp_uuid2.clock_seq_and_node);
}

TEST_F(TestAnswersByAnnOrQuestionCqlManager, InsertRelationshipTwice_Test) {
  if (_answer_by_ann_or_que_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_relationships()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  CqlResult cql_result = _answer_by_ann_or_que_cql_manager->create_relationship(
      1, temp_uuid, 1, temp_uuid);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _answer_by_ann_or_que_cql_manager->create_relationship(
      1, temp_uuid, 1, temp_uuid);

  ASSERT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}

TEST_F(TestAnswersByAnnOrQuestionCqlManager, ReadNonexistentRelationship_Test) {
  if (_answer_by_ann_or_que_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_relationships()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  auto answers = _answer_by_ann_or_que_cql_manager
                     ->get_answers_by_announcement_or_question(1, temp_uuid, 1);
  ASSERT_EQ(answers.first.code(), ResultCode::NOT_FOUND);
  ASSERT_EQ(answers.second.size(), 0);
}

TEST_F(TestAnswersByAnnOrQuestionCqlManager,
       DeleteNonexistentRelationship_Test) {
  if (_answer_by_ann_or_que_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_relationships()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  CqlResult cql_result = _answer_by_ann_or_que_cql_manager->delete_relationship(
      1, temp_uuid, 1, temp_uuid);

  ASSERT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}
