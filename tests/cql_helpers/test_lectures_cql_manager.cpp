/**
 * @file test_lectures_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for testing the lectures cql manager
 * @version 0.1
 * @date 2023-01-10
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <gtest/gtest.h>

#include "cql_helpers/lectures_cql_manager.hpp"

class TestLecturesCqlManager : public ::testing::Test {
 public:
  TestLecturesCqlManager(){};

  virtual ~TestLecturesCqlManager(){};

  const std::string CASSANDRA_IP = "127.0.0.1";

  CqlClient* _cql_client = nullptr;
  LecturesCqlManager* _lectures_cql_manager = nullptr;

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

    _lectures_cql_manager = new LecturesCqlManager(_cql_client);
    cql_result = _lectures_cql_manager->configure(true);

    if (cql_result.code() != ResultCode::OK) {
      delete _cql_client;
      _cql_client = nullptr;
      delete _lectures_cql_manager;
      _lectures_cql_manager = nullptr;

      return;
    }
  }

  void disconnect() {
    if (_cql_client != nullptr) {
      delete _cql_client;
      _cql_client = nullptr;
    }

    if (_lectures_cql_manager != nullptr) {
      delete _lectures_cql_manager;
      _lectures_cql_manager = nullptr;
    }
  }

  bool delete_lectures() {
    CqlResult cql_result =
        _cql_client->execute_statement("TRUNCATE TABLE schools.lectures;");

    return cql_result.code() == ResultCode::OK;
  }
};

TEST_F(TestLecturesCqlManager, WriteLecture_Test) {
  if (_lectures_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_lectures()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  LectureObject temp_lecture(1, temp_uuid, time(nullptr), 60, "test location");

  CqlResult cql_result = _lectures_cql_manager->create_lecture(temp_lecture);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);
}

TEST_F(TestLecturesCqlManager, ReadLecture_Test) {
  if (_lectures_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_lectures()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  LectureObject temp_lecture(1, temp_uuid, time(nullptr), 60, "test location");

  CqlResult cql_result = _lectures_cql_manager->create_lecture(temp_lecture);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_answer, read_lectures] =
      _lectures_cql_manager->get_lectures_by_course(1, temp_uuid);

  EXPECT_EQ(cql_answer.code(), ResultCode::OK);
  EXPECT_EQ(read_lectures.size(), 1);
  EXPECT_EQ(read_lectures[0]._school_id, 1);
  EXPECT_EQ(read_lectures[0]._course_id.clock_seq_and_node,
            temp_uuid.clock_seq_and_node);
  EXPECT_EQ(read_lectures[0]._starting_time, temp_lecture._starting_time);
  EXPECT_EQ(read_lectures[0]._duration, temp_lecture._duration);
  EXPECT_EQ(read_lectures[0]._location, temp_lecture._location);
}

TEST_F(TestLecturesCqlManager, UpdateLecture_Test) {
  if (_lectures_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_lectures()) {
    return;
  }

  time_t temp_time = time(nullptr);
  CassUuid temp_uuid = create_current_uuid();

  LectureObject temp_lecture(1, temp_uuid, temp_time, 60, "test location");

  CqlResult cql_result = _lectures_cql_manager->create_lecture(temp_lecture);
  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  temp_lecture._starting_time = temp_time + 1000;
  temp_lecture._duration = 50;
  temp_lecture._location = "new test location";

  cql_result = _lectures_cql_manager->update_lecture(
      1, temp_uuid, temp_time, temp_lecture._starting_time, 50,
      "new test location");
  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_answer, read_lectures] =
      _lectures_cql_manager->get_lectures_by_course(1, temp_uuid);

  EXPECT_EQ(cql_answer.code(), ResultCode::OK);
  EXPECT_EQ(read_lectures.size(), 1);
  EXPECT_EQ(read_lectures[0]._school_id, 1);
  EXPECT_EQ(read_lectures[0]._course_id.clock_seq_and_node,
            temp_uuid.clock_seq_and_node);
  EXPECT_EQ(read_lectures[0]._starting_time, temp_time + 1000);
  EXPECT_EQ(read_lectures[0]._duration, 50);
  EXPECT_EQ(read_lectures[0]._location, "new test location");
}

TEST_F(TestLecturesCqlManager, DeleteLecture_Test) {
  if (_lectures_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_lectures()) {
    return;
  }

  time_t temp_time = time(nullptr);
  CassUuid temp_uuid = create_current_uuid();

  LectureObject temp_lecture(1, temp_uuid, temp_time, 60, "test location");

  CqlResult cql_result = _lectures_cql_manager->create_lecture(temp_lecture);
  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _lectures_cql_manager->delete_lecture(1, temp_uuid, temp_time);
  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_answer, read_lectures] =
      _lectures_cql_manager->get_lectures_by_course(1, temp_uuid);

  EXPECT_EQ(cql_answer.code(), ResultCode::NOT_FOUND);
  EXPECT_EQ(read_lectures.size(), 0);
}

TEST_F(TestLecturesCqlManager, DeleteLecturesByCourse_Test) {
  if (_lectures_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_lectures()) {
    return;
  }

  time_t temp_time = time(nullptr);
  CassUuid temp_uuid = create_current_uuid();

  LectureObject temp_lecture1(1, temp_uuid, temp_time, 60, "test location1");
  LectureObject temp_lecture2(1, temp_uuid, temp_time + 1000, 60,
                              "test location2");

  CqlResult cql_result = _lectures_cql_manager->create_lecture(temp_lecture1);
  EXPECT_EQ(cql_result.code(), ResultCode::OK);
  cql_result = _lectures_cql_manager->create_lecture(temp_lecture2);
  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _lectures_cql_manager->delete_lectures_by_course(1, temp_uuid);
  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_answer, read_lectures] =
      _lectures_cql_manager->get_lectures_by_course(1, temp_uuid);

  EXPECT_EQ(cql_answer.code(), ResultCode::NOT_FOUND);
  EXPECT_EQ(read_lectures.size(), 0);
}

TEST_F(TestLecturesCqlManager, InsertLecturesTwice_Test) {
  if (_lectures_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_lectures()) {
    return;
  }

  time_t temp_time = time(nullptr);
  CassUuid temp_uuid = create_current_uuid();

  LectureObject temp_lecture(1, temp_uuid, temp_time, 60, "test location");

  CqlResult cql_result = _lectures_cql_manager->create_lecture(temp_lecture);
  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _lectures_cql_manager->create_lecture(temp_lecture);
  EXPECT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}

TEST_F(TestLecturesCqlManager, ReadNonexistentLectures_Test) {
  if (_lectures_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_lectures()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  auto [cql_answer, read_lectures] =
      _lectures_cql_manager->get_lectures_by_course(1, temp_uuid);

  EXPECT_EQ(cql_answer.code(), ResultCode::NOT_FOUND);
  EXPECT_EQ(read_lectures.size(), 0);
}

TEST_F(TestLecturesCqlManager, DeleteNonexistentLectures_Test) {
  if (_lectures_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_lectures()) {
    return;
  }

  time_t temp_time = time(nullptr);
  CassUuid temp_uuid = create_current_uuid();

  CqlResult cql_result =
      _lectures_cql_manager->delete_lecture(1, temp_uuid, temp_time);
  EXPECT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}