/**
 * @file test_files_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for testing the files cql manager
 * @version 0.1
 * @date 2023-01-10
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <gtest/gtest.h>

#include "cql_helpers/files_cql_manager.hpp"

class TestFilesCqlManager : public ::testing::Test {
 public:
  TestFilesCqlManager(){};

  virtual ~TestFilesCqlManager(){};

  const std::string CASSANDRA_IP = "127.0.0.1";

  CqlClient* _cql_client = nullptr;
  FilesCqlManager* _files_cql_manager = nullptr;

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

    _files_cql_manager = new FilesCqlManager(_cql_client);
    cql_result = _files_cql_manager->configure(true);

    if (cql_result.code() != ResultCode::OK) {
      delete _cql_client;
      _cql_client = nullptr;
      delete _files_cql_manager;
      _files_cql_manager = nullptr;

      return;
    }
  }

  void disconnect() {
    if (_cql_client != nullptr) {
      delete _cql_client;
      _cql_client = nullptr;
    }

    if (_files_cql_manager != nullptr) {
      delete _files_cql_manager;
      _files_cql_manager = nullptr;
    }
  }

  bool delete_files() {
    CqlResult cql_result =
        _cql_client->execute_statement("TRUNCATE TABLE schools.files;");

    return cql_result.code() == ResultCode::OK;
  }
};

TEST_F(TestFilesCqlManager, WriteFile_Test) {
  if (_files_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_files()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  FileObject temp_file(1, temp_uuid, CustomFileType::FOLDER, "Test File", {},
                       "../to_test_file", 10, temp_uuid, true, false);

  CqlResult cql_result = _files_cql_manager->create_file(temp_file);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);
}

TEST_F(TestFilesCqlManager, ReadFile_Test) {
  if (_files_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_files()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  FileObject temp_file(1, temp_uuid, CustomFileType::FILE, "Test File", {},
                       "../to_test_file", 10, temp_uuid, true, false);

  CqlResult cql_result = _files_cql_manager->create_file(temp_file);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_response, read_file] =
      _files_cql_manager->get_file_by_id(1, temp_uuid);

  EXPECT_EQ(cql_response.code(), ResultCode::OK);
  EXPECT_EQ(read_file._school_id, temp_file._school_id);
  EXPECT_EQ(read_file._id.clock_seq_and_node, temp_file._id.clock_seq_and_node);
  EXPECT_EQ(read_file._type, temp_file._type);
  EXPECT_EQ(read_file._name, temp_file._name);
  EXPECT_EQ(read_file._files.size(), temp_file._files.size());
  EXPECT_EQ(read_file._size, temp_file._size);
  EXPECT_EQ(read_file._added_by_user.clock_seq_and_node,
            temp_file._added_by_user.clock_seq_and_node);
  EXPECT_EQ(read_file._visible_to_students, temp_file._visible_to_students);
  EXPECT_EQ(read_file._students_can_add, temp_file._students_can_add);
}

TEST_F(TestFilesCqlManager, UpdateFile_Test) {
  if (_files_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_files()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  FileObject temp_file(1, temp_uuid, CustomFileType::FILE, "Test File", {},
                       "../to_test_file", 10, temp_uuid, true, false);

  CqlResult cql_result = _files_cql_manager->create_file(temp_file);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  temp_file._name = "Test File 2";
  temp_file._files.push_back(temp_uuid);

  cql_result = _files_cql_manager->update_file(
      1, temp_uuid, CustomFileType::FILE, temp_file._name, temp_file._files,
      "../to_test_file", 10, temp_uuid, true, false);

  auto [cql_response, read_file] =
      _files_cql_manager->get_file_by_id(1, temp_uuid);

  EXPECT_EQ(cql_response.code(), ResultCode::OK);
  EXPECT_EQ(read_file._school_id, temp_file._school_id);
  EXPECT_EQ(read_file._id.clock_seq_and_node, temp_file._id.clock_seq_and_node);
  EXPECT_EQ(read_file._type, temp_file._type);
  EXPECT_EQ(read_file._name, temp_file._name);
  EXPECT_EQ(read_file._files.size(), temp_file._files.size());
  EXPECT_EQ(read_file._files.at(0).clock_seq_and_node,
            temp_file._files.at(0).clock_seq_and_node);
  EXPECT_EQ(read_file._size, temp_file._size);
  EXPECT_EQ(read_file._added_by_user.clock_seq_and_node,
            temp_file._added_by_user.clock_seq_and_node);
  EXPECT_EQ(read_file._visible_to_students, temp_file._visible_to_students);
  EXPECT_EQ(read_file._students_can_add, temp_file._students_can_add);
}

TEST_F(TestFilesCqlManager, DeleteFile_Test) {
  if (_files_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_files()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  FileObject temp_file(1, temp_uuid, CustomFileType::FILE, "Test File", {},
                       "../to_test_file", 10, temp_uuid, true, false);

  CqlResult cql_result = _files_cql_manager->create_file(temp_file);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _files_cql_manager->delete_file(1, temp_uuid);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_response, read_file] =
      _files_cql_manager->get_file_by_id(1, temp_uuid);

  EXPECT_EQ(cql_response.code(), ResultCode::NOT_FOUND);
}

TEST_F(TestFilesCqlManager, InsertFileTwice_Test) {
  if (_files_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_files()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  FileObject temp_file(1, temp_uuid, CustomFileType::FILE, "Test File", {},
                       "../to_test_file", 10, temp_uuid, true, false);

  CqlResult cql_result = _files_cql_manager->create_file(temp_file);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _files_cql_manager->create_file(temp_file);

  EXPECT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}

TEST_F(TestFilesCqlManager, ReadNonexistentFiles_Test) {
  if (_files_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_files()) {
    return;
  }

  auto [cql_response, read_file] =
      _files_cql_manager->get_file_by_id(1, create_current_uuid());

  EXPECT_EQ(cql_response.code(), ResultCode::NOT_FOUND);
}

TEST_F(TestFilesCqlManager, DeleteNonexistentFile_Test) {
  if (_files_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_files()) {
    return;
  }

  CqlResult cql_result =
      _files_cql_manager->delete_file(1, create_current_uuid());

  EXPECT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}