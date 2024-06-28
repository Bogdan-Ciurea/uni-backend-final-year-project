/**
 * @file test_grades_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for testing the grades cql manager
 * @version 0.1
 * @date 2023-01-10
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <gtest/gtest.h>

#include "cql_helpers/grades_cql_manager.hpp"

class TestGradesCqlManager : public ::testing::Test {
 public:
  TestGradesCqlManager(){};

  virtual ~TestGradesCqlManager(){};

  const std::string CASSANDRA_IP = "127.0.0.1";

  CqlClient* _cql_client = nullptr;
  GradesCqlManager* _grades_cql_manager = nullptr;

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

    _grades_cql_manager = new GradesCqlManager(_cql_client);
    cql_result = _grades_cql_manager->configure(true);

    if (cql_result.code() != ResultCode::OK) {
      delete _cql_client;
      _cql_client = nullptr;
      delete _grades_cql_manager;
      _grades_cql_manager = nullptr;

      return;
    }
  }

  void disconnect() {
    if (_cql_client != nullptr) {
      delete _cql_client;
      _cql_client = nullptr;
    }

    if (_grades_cql_manager != nullptr) {
      delete _grades_cql_manager;
      _grades_cql_manager = nullptr;
    }
  }

  bool delete_grades() {
    CqlResult cql_result =
        _cql_client->execute_statement("TRUNCATE TABLE schools.grades;");

    return cql_result.code() == ResultCode::OK;
  }
};

TEST_F(TestGradesCqlManager, WriteGrade_Test) {
  if (_grades_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_grades()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  GradeObject temp_grade(1, temp_uuid, temp_uuid, temp_uuid, temp_uuid, 10, 10,
                         time(nullptr), .7f);

  CqlResult cql_result = _grades_cql_manager->create_grade(temp_grade);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);
}

TEST_F(TestGradesCqlManager, ReadGrade_Test) {
  if (_grades_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_grades()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  time_t temp_time = time(nullptr);

  GradeObject temp_grade(1, temp_uuid, temp_uuid, temp_uuid, temp_uuid, 10, 10,
                         temp_time, .7f);

  CqlResult cql_result = _grades_cql_manager->create_grade(temp_grade);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_answer, read_grade] =
      _grades_cql_manager->get_grade_by_id(1, temp_uuid);

  ASSERT_EQ(cql_answer.code(), ResultCode::OK);
  ASSERT_EQ(read_grade._school_id, temp_grade._school_id);
  ASSERT_EQ(read_grade._id.clock_seq_and_node, temp_uuid.clock_seq_and_node);
  ASSERT_EQ(read_grade._evaluated_id.clock_seq_and_node,
            temp_uuid.clock_seq_and_node);
  ASSERT_EQ(read_grade._evaluator_id.clock_seq_and_node,
            temp_uuid.clock_seq_and_node);
  ASSERT_EQ(read_grade._course_id.clock_seq_and_node,
            temp_uuid.clock_seq_and_node);
  ASSERT_EQ(read_grade._grade, temp_grade._grade);
  ASSERT_EQ(read_grade._out_of, temp_grade._out_of);
  ASSERT_EQ(read_grade._created_at, temp_time);
  ASSERT_EQ(read_grade._weight, temp_grade._weight);
}

TEST_F(TestGradesCqlManager, ReadGradesByStudent_Test) {
  if (_grades_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_grades()) {
    return;
  }

  CassUuid temp_student_id1 = create_current_uuid();
  CassUuid temp_student_id2 = create_current_uuid();
  CassUuid temp_grade_id1 = create_current_uuid();
  CassUuid temp_grade_id2 = create_current_uuid();
  CassUuid temp_grade_id3 = create_current_uuid();
  time_t temp_time = time(nullptr);

  GradeObject temp_grade1(1, temp_grade_id1, temp_student_id1,
                          create_current_uuid(), create_current_uuid(), 10, 10,
                          temp_time, .7f);
  GradeObject temp_grade2(1, temp_grade_id2, temp_student_id1,
                          create_current_uuid(), create_current_uuid(), 10, 10,
                          temp_time, .7f);
  GradeObject temp_grade3(1, temp_grade_id3, temp_student_id2,
                          create_current_uuid(), create_current_uuid(), 10, 10,
                          temp_time, .7f);

  CqlResult cql_result = _grades_cql_manager->create_grade(temp_grade1);
  ASSERT_EQ(cql_result.code(), ResultCode::OK);
  cql_result = _grades_cql_manager->create_grade(temp_grade2);
  ASSERT_EQ(cql_result.code(), ResultCode::OK);
  cql_result = _grades_cql_manager->create_grade(temp_grade3);
  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_answer, read_grades] =
      _grades_cql_manager->get_grades_by_student_id(1, temp_student_id1);

  ASSERT_EQ(cql_answer.code(), ResultCode::OK);
  ASSERT_EQ(read_grades.size(), 2);
  for (auto& grade : read_grades)
    ASSERT_TRUE(
        grade._id.clock_seq_and_node == temp_grade_id1.clock_seq_and_node ||
        grade._id.clock_seq_and_node == temp_grade_id2.clock_seq_and_node);
}

TEST_F(TestGradesCqlManager, ReadGradesByEvaluator_Test) {
  if (_grades_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_grades()) {
    return;
  }

  CassUuid temp_evaluator_id1 = create_current_uuid();
  CassUuid temp_evaluator_id2 = create_current_uuid();
  CassUuid temp_grade_id1 = create_current_uuid();
  CassUuid temp_grade_id2 = create_current_uuid();
  CassUuid temp_grade_id3 = create_current_uuid();
  time_t temp_time = time(nullptr);

  GradeObject temp_grade1(1, temp_grade_id1, create_current_uuid(),
                          temp_evaluator_id1, create_current_uuid(), 10, 10,
                          temp_time, .7f);
  GradeObject temp_grade2(1, temp_grade_id2, create_current_uuid(),
                          temp_evaluator_id1, create_current_uuid(), 10, 10,
                          temp_time, .7f);
  GradeObject temp_grade3(1, temp_grade_id3, create_current_uuid(),
                          temp_evaluator_id2, create_current_uuid(), 10, 10,
                          temp_time, .7f);

  CqlResult cql_result = _grades_cql_manager->create_grade(temp_grade1);
  ASSERT_EQ(cql_result.code(), ResultCode::OK);
  cql_result = _grades_cql_manager->create_grade(temp_grade2);
  ASSERT_EQ(cql_result.code(), ResultCode::OK);
  cql_result = _grades_cql_manager->create_grade(temp_grade3);
  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_answer, read_grades] =
      _grades_cql_manager->get_grades_by_evaluator_id(1, temp_evaluator_id1);

  ASSERT_EQ(cql_answer.code(), ResultCode::OK);
  ASSERT_EQ(read_grades.size(), 2);
  for (auto& grade : read_grades)
    ASSERT_TRUE(
        grade._id.clock_seq_and_node == temp_grade_id1.clock_seq_and_node ||
        grade._id.clock_seq_and_node == temp_grade_id2.clock_seq_and_node);
}

TEST_F(TestGradesCqlManager, ReadGradesByCourse_Test) {
  if (_grades_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_grades()) {
    return;
  }

  CassUuid temp_course_id1 = create_current_uuid();
  CassUuid temp_course_id2 = create_current_uuid();
  CassUuid temp_grade_id1 = create_current_uuid();
  CassUuid temp_grade_id2 = create_current_uuid();
  CassUuid temp_grade_id3 = create_current_uuid();
  time_t temp_time = time(nullptr);

  GradeObject temp_grade1(1, temp_grade_id1, create_current_uuid(),
                          create_current_uuid(), temp_course_id1, 10, 10,
                          temp_time, .7f);
  GradeObject temp_grade2(1, temp_grade_id2, create_current_uuid(),
                          create_current_uuid(), temp_course_id1, 10, 10,
                          temp_time, .7f);
  GradeObject temp_grade3(1, temp_grade_id3, create_current_uuid(),
                          create_current_uuid(), temp_course_id2, 10, 10,
                          temp_time, .7f);

  CqlResult cql_result = _grades_cql_manager->create_grade(temp_grade1);
  ASSERT_EQ(cql_result.code(), ResultCode::OK);
  cql_result = _grades_cql_manager->create_grade(temp_grade2);
  ASSERT_EQ(cql_result.code(), ResultCode::OK);
  cql_result = _grades_cql_manager->create_grade(temp_grade3);
  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_answer, read_grades] =
      _grades_cql_manager->get_grades_by_course_id(1, temp_course_id1);

  ASSERT_EQ(cql_answer.code(), ResultCode::OK);
  ASSERT_EQ(read_grades.size(), 2);
  for (auto& grade : read_grades)
    ASSERT_TRUE(
        grade._id.clock_seq_and_node == temp_grade_id1.clock_seq_and_node ||
        grade._id.clock_seq_and_node == temp_grade_id2.clock_seq_and_node);
}

TEST_F(TestGradesCqlManager, UpdateGrade_Test) {
  if (_grades_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_grades()) {
    return;
  }

  CassUuid temp_grade_id = create_current_uuid();
  time_t temp_time = time(nullptr);

  GradeObject temp_grade1(1, temp_grade_id, create_current_uuid(),
                          create_current_uuid(), create_current_uuid(), 10, 10,
                          temp_time, .7f);

  CqlResult cql_result = _grades_cql_manager->create_grade(temp_grade1);
  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  temp_grade1._grade = 5;
  temp_grade1._out_of = 5;
  temp_grade1._created_at = time(nullptr);
  temp_grade1._weight = .5f;

  cql_result = _grades_cql_manager->update_grade(
      1, temp_grade_id, temp_grade1._grade = 5, temp_grade1._out_of = 5,
      temp_grade1._created_at, temp_grade1._weight);
  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_answer, read_grade] =
      _grades_cql_manager->get_grade_by_id(1, temp_grade_id);

  ASSERT_EQ(cql_answer.code(), ResultCode::OK);
  ASSERT_EQ(read_grade._id.clock_seq_and_node,
            temp_grade_id.clock_seq_and_node);
  ASSERT_EQ(read_grade._grade, 5);
  ASSERT_EQ(read_grade._out_of, 5);
  ASSERT_EQ(read_grade._created_at, temp_grade1._created_at);
  ASSERT_EQ(read_grade._weight, .5f);
}

TEST_F(TestGradesCqlManager, DeleteGrade_Test) {
  if (_grades_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_grades()) {
    return;
  }

  CassUuid temp_grade_id = create_current_uuid();
  time_t temp_time = time(nullptr);

  GradeObject temp_grade1(1, temp_grade_id, create_current_uuid(),
                          create_current_uuid(), create_current_uuid(), 10, 10,
                          temp_time, .7f);

  CqlResult cql_result = _grades_cql_manager->create_grade(temp_grade1);
  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _grades_cql_manager->delete_grade(1, temp_grade_id);
  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_answer, read_grade] =
      _grades_cql_manager->get_grade_by_id(1, temp_grade_id);

  ASSERT_EQ(cql_answer.code(), ResultCode::NOT_FOUND);
}

TEST_F(TestGradesCqlManager, InsertGradesTwice_Test) {
  if (_grades_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_grades()) {
    return;
  }

  CassUuid temp_grade_id = create_current_uuid();
  time_t temp_time = time(nullptr);

  GradeObject temp_grade1(1, temp_grade_id, create_current_uuid(),
                          create_current_uuid(), create_current_uuid(), 10, 10,
                          temp_time, .7f);

  CqlResult cql_result = _grades_cql_manager->create_grade(temp_grade1);
  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _grades_cql_manager->create_grade(temp_grade1);
  ASSERT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}

TEST_F(TestGradesCqlManager, ReadNonexistentGrades_Test) {
  if (_grades_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_grades()) {
    return;
  }

  CassUuid temp_grade_id = create_current_uuid();

  auto [cql_answer, read_grade] =
      _grades_cql_manager->get_grade_by_id(1, temp_grade_id);

  ASSERT_EQ(cql_answer.code(), ResultCode::NOT_FOUND);
}

TEST_F(TestGradesCqlManager, DeleteNonexistentGrades_Test) {
  if (_grades_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_grades()) {
    return;
  }

  CassUuid temp_grade_id = create_current_uuid();

  CqlResult cql_result = _grades_cql_manager->delete_grade(1, temp_grade_id);
  ASSERT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}