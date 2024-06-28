/**
 * @file test_courses_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for testing the courses cql manager
 * @version 0.1
 * @date 2023-01-10
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <gtest/gtest.h>

#include "cql_helpers/courses_cql_manager.hpp"

class TestCourseCqlManager : public ::testing::Test {
 public:
  TestCourseCqlManager(){};

  virtual ~TestCourseCqlManager(){};

  const std::string CASSANDRA_IP = "127.0.0.1";

  CqlClient* _cql_client = nullptr;
  CoursesCqlManager* _courses_cql_manager = nullptr;

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

    _courses_cql_manager = new CoursesCqlManager(_cql_client);
    cql_result = _courses_cql_manager->configure(true);

    if (cql_result.code() != ResultCode::OK) {
      delete _cql_client;
      _cql_client = nullptr;
      delete _courses_cql_manager;
      _courses_cql_manager = nullptr;

      return;
    }
  }

  void disconnect() {
    if (_cql_client != nullptr) {
      delete _cql_client;
      _cql_client = nullptr;
    }

    if (_courses_cql_manager != nullptr) {
      delete _courses_cql_manager;
      _courses_cql_manager = nullptr;
    }
  }

  bool delete_courses() {
    CqlResult cql_result =
        _cql_client->execute_statement("TRUNCATE TABLE schools.courses;");

    return cql_result.code() == ResultCode::OK;
  }
};

TEST_F(TestCourseCqlManager, WriteCourse_Test) {
  if (_courses_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_courses()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  CourseObject temp_course(1, temp_uuid, "Test Course", "../", time(nullptr),
                           time(nullptr), time(nullptr), {temp_uuid});

  CqlResult cql_result = _courses_cql_manager->create_course(temp_course);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);
}

TEST_F(TestCourseCqlManager, ReadCourse_Test) {
  if (_courses_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_courses()) {
    return;
  }
  CassUuid temp_uuid = create_current_uuid();
  CassUuid temp_uuid2 = create_current_uuid();
  time_t temp_time = time(nullptr);

  CourseObject temp_course(1, temp_uuid, "Test Course", "../", temp_time,
                           temp_time - 1000, temp_time + 1000,
                           {temp_uuid, temp_uuid2});

  CqlResult cql_result = _courses_cql_manager->create_course(temp_course);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_answer, read_course] =
      _courses_cql_manager->get_course_by_id(1, temp_uuid);

  EXPECT_EQ(cql_answer.code(), ResultCode::OK);
  EXPECT_EQ(read_course._school_id, temp_course._school_id);
  EXPECT_EQ(read_course._id.clock_seq_and_node,
            temp_course._id.clock_seq_and_node);
  EXPECT_EQ(read_course._name, temp_course._name);
  EXPECT_EQ(read_course._course_thumbnail, temp_course._course_thumbnail);
  EXPECT_EQ(read_course._created_at, temp_course._created_at);
  EXPECT_EQ(read_course._end_date, temp_course._end_date);
  EXPECT_EQ(read_course._start_date, temp_course._start_date);
  EXPECT_EQ(read_course._files.size(), 2);
  EXPECT_EQ(read_course._files.at(0).clock_seq_and_node,
            temp_uuid.clock_seq_and_node);
  EXPECT_EQ(read_course._files.at(1).clock_seq_and_node,
            temp_uuid2.clock_seq_and_node);
}

TEST_F(TestCourseCqlManager, UpdateCourse_Test) {
  if (_courses_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_courses()) {
    return;
  }
  CassUuid temp_uuid = create_current_uuid();
  CassUuid temp_uuid2 = create_current_uuid();
  time_t temp_time = time(nullptr);

  CourseObject temp_course(1, temp_uuid, "Test Course", "../", temp_time,
                           temp_time - 1000, temp_time + 1000,
                           {temp_uuid, temp_uuid2});

  CqlResult cql_result = _courses_cql_manager->create_course(temp_course);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _courses_cql_manager->update_course(
      1, temp_uuid, "New Name", "../new", temp_time + 1000, temp_time + 2000,
      temp_time + 3000, {temp_uuid2});

  auto [cql_answer, read_course] =
      _courses_cql_manager->get_course_by_id(1, temp_uuid);

  EXPECT_EQ(cql_answer.code(), ResultCode::OK);
  EXPECT_EQ(read_course._school_id, temp_course._school_id);
  EXPECT_EQ(read_course._id.clock_seq_and_node,
            temp_course._id.clock_seq_and_node);
  EXPECT_EQ(read_course._name, "New Name");
  EXPECT_EQ(read_course._course_thumbnail, "../new");
  EXPECT_EQ(read_course._created_at, temp_time + 1000);
  EXPECT_EQ(read_course._start_date, temp_time + 2000);
  EXPECT_EQ(read_course._end_date, temp_time + 3000);
  EXPECT_EQ(read_course._files.size(), 1);
  EXPECT_EQ(read_course._files.at(0).clock_seq_and_node,
            temp_uuid2.clock_seq_and_node);
}

TEST_F(TestCourseCqlManager, DeleteCourse_Test) {
  if (_courses_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_courses()) {
    return;
  }
  CassUuid temp_uuid = create_current_uuid();
  CassUuid temp_uuid2 = create_current_uuid();
  time_t temp_time = time(nullptr);

  CourseObject temp_course(1, temp_uuid, "Test Course", "../", temp_time,
                           temp_time - 1000, temp_time + 1000,
                           {temp_uuid, temp_uuid2});

  CqlResult cql_result = _courses_cql_manager->create_course(temp_course);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _courses_cql_manager->delete_course_by_id(1, temp_uuid);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_answer, read_course] =
      _courses_cql_manager->get_course_by_id(1, temp_uuid);

  EXPECT_EQ(cql_answer.code(), ResultCode::NOT_FOUND);
}

TEST_F(TestCourseCqlManager, InsertCourseTwice_Test) {
  if (_courses_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_courses()) {
    return;
  }
  CassUuid temp_uuid = create_current_uuid();
  CassUuid temp_uuid2 = create_current_uuid();
  time_t temp_time = time(nullptr);

  CourseObject temp_course(1, temp_uuid, "Test Course", "../", temp_time,
                           temp_time - 1000, temp_time + 1000,
                           {temp_uuid, temp_uuid2});

  CqlResult cql_result = _courses_cql_manager->create_course(temp_course);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _courses_cql_manager->create_course(temp_course);

  EXPECT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}

TEST_F(TestCourseCqlManager, ReadNonexistentCourse_Test) {
  if (_courses_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_courses()) {
    return;
  }
  CassUuid temp_uuid = create_current_uuid();
  CassUuid temp_uuid2 = create_current_uuid();
  time_t temp_time = time(nullptr);

  CourseObject temp_course(1, temp_uuid, "Test Course", "../", temp_time,
                           temp_time - 1000, temp_time + 1000,
                           {temp_uuid, temp_uuid2});

  CqlResult cql_result = _courses_cql_manager->create_course(temp_course);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_answer, read_course] =
      _courses_cql_manager->get_course_by_id(1, temp_uuid2);

  EXPECT_EQ(cql_answer.code(), ResultCode::NOT_FOUND);
}

TEST_F(TestCourseCqlManager, DeleteNonexistentCourse_Test) {
  if (_courses_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_courses()) {
    return;
  }
  CassUuid temp_uuid = create_current_uuid();
  CassUuid temp_uuid2 = create_current_uuid();
  time_t temp_time = time(nullptr);

  CourseObject temp_course(1, temp_uuid, "Test Course", "../", temp_time,
                           temp_time - 1000, temp_time + 1000,
                           {temp_uuid, temp_uuid2});

  CqlResult cql_result = _courses_cql_manager->create_course(temp_course);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _courses_cql_manager->delete_course_by_id(1, temp_uuid2);

  EXPECT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}
