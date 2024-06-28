/**
 * @file test_student_references_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for testing the student references cql
 * manager
 * @version 0.1
 * @date 2023-01-11
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <gtest/gtest.h>

#include <cql_helpers/student_references_cql_manager.hpp>

class TestStudentReferencesCqlManager : public ::testing::Test {
 public:
  TestStudentReferencesCqlManager(){};

  virtual ~TestStudentReferencesCqlManager(){};

  const std::string CASSANDRA_IP = "127.0.0.1";

  CqlClient* _cql_client = nullptr;
  StudentReferencesCqlManager* _student_references_cql_manager = nullptr;

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

    _student_references_cql_manager =
        new StudentReferencesCqlManager(_cql_client);
    cql_result = _student_references_cql_manager->configure(true);

    if (cql_result.code() != ResultCode::OK) {
      delete _cql_client;
      _cql_client = nullptr;
      delete _student_references_cql_manager;
      _student_references_cql_manager = nullptr;

      return;
    }
  }

  void disconnect() {
    if (_cql_client != nullptr) {
      delete _cql_client;
      _cql_client = nullptr;
    }

    if (_student_references_cql_manager != nullptr) {
      delete _student_references_cql_manager;
      _student_references_cql_manager = nullptr;
    }
  }

  bool delete_student_references() {
    CqlResult cql_result = _cql_client->execute_statement(
        "TRUNCATE TABLE schools.student_reference;");

    return cql_result.code() == ResultCode::OK;
  }
};

TEST_F(TestStudentReferencesCqlManager, WriteReference_Test) {
  if (_student_references_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_student_references()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  StudentReferenceObject temp_reference(1, temp_uuid, "+40733350380",
                                        ReferenceType::PHONE_NUMBER);

  CqlResult cql_result =
      _student_references_cql_manager->create_student_reference(temp_reference);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);
}

TEST_F(TestStudentReferencesCqlManager, ReadReference_Test) {
  if (_student_references_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_student_references()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  StudentReferenceObject temp_reference(1, temp_uuid, "+40733350380",
                                        ReferenceType::PHONE_NUMBER);

  CqlResult cql_result =
      _student_references_cql_manager->create_student_reference(temp_reference);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_answer, read_references] =
      _student_references_cql_manager->get_student_references(1, temp_uuid);

  EXPECT_EQ(cql_answer.code(), ResultCode::OK);
  EXPECT_EQ(read_references.size(), 1);
  EXPECT_EQ(read_references[0]._school_id, 1);
  EXPECT_EQ(read_references[0]._student_id.clock_seq_and_node,
            temp_uuid.clock_seq_and_node);
  EXPECT_EQ(read_references[0]._reference, "+40733350380");
  EXPECT_EQ(read_references[0]._type, ReferenceType::PHONE_NUMBER);
}

TEST_F(TestStudentReferencesCqlManager, ReadMultipleReferences_Test) {
  if (_student_references_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_student_references()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  StudentReferenceObject temp_reference1(1, temp_uuid, "+40733350380",
                                         ReferenceType::PHONE_NUMBER);
  StudentReferenceObject temp_reference2(1, temp_uuid, "sc20bac@leeds.ac.uk",
                                         ReferenceType::EMAIL);

  CqlResult cql_result =
      _student_references_cql_manager->create_student_reference(
          temp_reference1);
  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _student_references_cql_manager->create_student_reference(
      temp_reference2);
  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_answer, read_references] =
      _student_references_cql_manager->get_student_references(1, temp_uuid);

  EXPECT_EQ(cql_answer.code(), ResultCode::OK);
  EXPECT_EQ(read_references.size(), 2);
  EXPECT_EQ(read_references[0]._school_id, 1);
  EXPECT_EQ(read_references[0]._student_id.clock_seq_and_node,
            temp_uuid.clock_seq_and_node);
  EXPECT_EQ(read_references[0]._reference, "+40733350380");
  EXPECT_EQ(read_references[0]._type, ReferenceType::PHONE_NUMBER);
  EXPECT_EQ(read_references[1]._school_id, 1);
  EXPECT_EQ(read_references[1]._student_id.clock_seq_and_node,
            temp_uuid.clock_seq_and_node);
  EXPECT_EQ(read_references[1]._reference, "sc20bac@leeds.ac.uk");
  EXPECT_EQ(read_references[1]._type, ReferenceType::EMAIL);
}

TEST_F(TestStudentReferencesCqlManager, UpdateReference_Test) {
  if (_student_references_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_student_references()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  StudentReferenceObject temp_reference(1, temp_uuid, "+40733350380",
                                        ReferenceType::PHONE_NUMBER);

  CqlResult cql_result =
      _student_references_cql_manager->create_student_reference(temp_reference);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _student_references_cql_manager->update_student_reference(
      1, temp_uuid, "+40733350380", "sc20bac@leeds.ac.uk",
      ReferenceType::EMAIL);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_answer, read_references] =
      _student_references_cql_manager->get_student_references(1, temp_uuid);

  EXPECT_EQ(cql_answer.code(), ResultCode::OK);
  EXPECT_EQ(read_references.size(), 1);
  EXPECT_EQ(read_references[0]._school_id, 1);
  EXPECT_EQ(read_references[0]._student_id.clock_seq_and_node,
            temp_uuid.clock_seq_and_node);
  EXPECT_EQ(read_references[0]._reference, "sc20bac@leeds.ac.uk");
  EXPECT_EQ(read_references[0]._type, ReferenceType::EMAIL);
}

TEST_F(TestStudentReferencesCqlManager, DeleteReference_Test) {
  if (_student_references_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_student_references()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  StudentReferenceObject temp_reference(1, temp_uuid, "+40733350380",
                                        ReferenceType::PHONE_NUMBER);

  CqlResult cql_result =
      _student_references_cql_manager->create_student_reference(temp_reference);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _student_references_cql_manager->delete_student_reference(
      1, temp_uuid, "+40733350380");

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_answer, read_references] =
      _student_references_cql_manager->get_student_references(1, temp_uuid);

  EXPECT_EQ(cql_answer.code(), ResultCode::NOT_FOUND);
  EXPECT_EQ(read_references.size(), 0);
}

TEST_F(TestStudentReferencesCqlManager, DeleteReferencesByUser_Test) {
  if (_student_references_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_student_references()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  StudentReferenceObject temp_reference1(1, temp_uuid, "+40733350380",
                                         ReferenceType::PHONE_NUMBER);
  StudentReferenceObject temp_reference2(1, temp_uuid, "sc20bac@leeds.ac.uk",
                                         ReferenceType::EMAIL);

  CqlResult cql_result =
      _student_references_cql_manager->create_student_reference(
          temp_reference1);
  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _student_references_cql_manager->create_student_reference(
      temp_reference2);
  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result =
      _student_references_cql_manager->delete_student_references(1, temp_uuid);
  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_answer, read_references] =
      _student_references_cql_manager->get_student_references(1, temp_uuid);

  EXPECT_EQ(cql_answer.code(), ResultCode::NOT_FOUND);
  EXPECT_EQ(read_references.size(), 0);
}

TEST_F(TestStudentReferencesCqlManager, InsertStudentReferencesTwice_Test) {
  if (_student_references_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_student_references()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  StudentReferenceObject temp_reference(1, temp_uuid, "+40733350380",
                                        ReferenceType::PHONE_NUMBER);

  CqlResult cql_result =
      _student_references_cql_manager->create_student_reference(temp_reference);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result =
      _student_references_cql_manager->create_student_reference(temp_reference);

  EXPECT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}

TEST_F(TestStudentReferencesCqlManager, ReadNonexistentStudentReferences_Test) {
  if (_student_references_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_student_references()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  auto [cql_answer, read_references] =
      _student_references_cql_manager->get_student_references(1, temp_uuid);

  EXPECT_EQ(cql_answer.code(), ResultCode::NOT_FOUND);
  EXPECT_EQ(read_references.size(), 0);
}

TEST_F(TestStudentReferencesCqlManager,
       DeleteNonexistentStudentReferences_Test) {
  if (_student_references_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_student_references()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  CqlResult cql_result =
      _student_references_cql_manager->delete_student_reference(1, temp_uuid,
                                                                "+40733350380");

  EXPECT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}