/**
 * @file test_tags_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for testing the tags cql manager
 * @version 0.1
 * @date 2023-01-11
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <gtest/gtest.h>

#include "cql_helpers/tags_cql_manager.hpp"

class TestTagsCqlManager : public ::testing::Test {
 public:
  TestTagsCqlManager(){};

  virtual ~TestTagsCqlManager(){};

  const std::string CASSANDRA_IP = "127.0.0.1";

  CqlClient* _cql_client = nullptr;
  TagsCqlManager* _tags_cql_manager = nullptr;

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

    _tags_cql_manager = new TagsCqlManager(_cql_client);
    cql_result = _tags_cql_manager->configure(true);

    if (cql_result.code() != ResultCode::OK) {
      delete _cql_client;
      _cql_client = nullptr;
      delete _tags_cql_manager;
      _tags_cql_manager = nullptr;

      return;
    }
  }

  void disconnect() {
    if (_cql_client != nullptr) {
      delete _cql_client;
      _cql_client = nullptr;
    }

    if (_tags_cql_manager != nullptr) {
      delete _tags_cql_manager;
      _tags_cql_manager = nullptr;
    }
  }

  bool delete_tags() {
    CqlResult cql_result =
        _cql_client->execute_statement("TRUNCATE TABLE schools.tags;");

    return cql_result.code() == ResultCode::OK;
  }
};

TEST_F(TestTagsCqlManager, WriteTag_Test) {
  if (_tags_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_tags()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  TagObject temp_tag(1, temp_uuid, "Test tag", "Blue");

  CqlResult cql_result = _tags_cql_manager->create_tag(temp_tag);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);
}

TEST_F(TestTagsCqlManager, ReadTag_Test) {
  if (_tags_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_tags()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  TagObject temp_tag(1, temp_uuid, "Test tag", "Blue");

  CqlResult cql_result = _tags_cql_manager->create_tag(temp_tag);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [cass_result, read_tag] = _tags_cql_manager->get_tag_by_id(1, temp_uuid);

  EXPECT_EQ(cass_result.code(), ResultCode::OK);
  EXPECT_EQ(read_tag._school_id, 1);
  EXPECT_EQ(read_tag._id.clock_seq_and_node, temp_uuid.clock_seq_and_node);
  EXPECT_EQ(read_tag._colour, "Blue");
  EXPECT_EQ(read_tag._name, "Test tag");
}

TEST_F(TestTagsCqlManager, ReadTagsBySchoolId_Test) {
  if (_tags_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_tags()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  CassUuid temp_uuid2 = create_current_uuid();

  TagObject temp_tag(1, temp_uuid, "Test tag1", "Blue");
  TagObject temp_tag2(1, temp_uuid2, "Test tag2", "Blue");
  TagObject temp_tag3(2, temp_uuid, "Test tag3", "Blue");

  CqlResult cql_result = _tags_cql_manager->create_tag(temp_tag);
  EXPECT_EQ(cql_result.code(), ResultCode::OK);
  cql_result = _tags_cql_manager->create_tag(temp_tag2);
  EXPECT_EQ(cql_result.code(), ResultCode::OK);
  cql_result = _tags_cql_manager->create_tag(temp_tag3);
  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [cass_result, read_tags] = _tags_cql_manager->get_tags_by_school_id(1);

  EXPECT_EQ(cass_result.code(), ResultCode::OK);
  for (auto& read_tag : read_tags) {
    EXPECT_EQ(read_tag._school_id, 1);
    EXPECT_TRUE(
        read_tag._id.clock_seq_and_node == temp_uuid.clock_seq_and_node ||
        read_tag._id.clock_seq_and_node == temp_uuid2.clock_seq_and_node);
    EXPECT_EQ(read_tag._colour, "Blue");
    EXPECT_TRUE(read_tag._name == "Test tag1" || read_tag._name == "Test tag2");
  }
}

TEST_F(TestTagsCqlManager, UpdateTag_Test) {
  if (_tags_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_tags()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  TagObject temp_tag(1, temp_uuid, "Test tag", "Blue");

  CqlResult cql_result = _tags_cql_manager->create_tag(temp_tag);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [cass_result, read_tag] = _tags_cql_manager->get_tag_by_id(1, temp_uuid);

  EXPECT_EQ(cass_result.code(), ResultCode::OK);
  EXPECT_EQ(read_tag._school_id, 1);
  EXPECT_EQ(read_tag._id.clock_seq_and_node, temp_uuid.clock_seq_and_node);
  EXPECT_EQ(read_tag._colour, "Blue");
  EXPECT_EQ(read_tag._name, "Test tag");
}

TEST_F(TestTagsCqlManager, DeleteTag_Test) {
  if (_tags_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_tags()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  TagObject temp_tag(1, temp_uuid, "Test tag", "Blue");

  CqlResult cql_result = _tags_cql_manager->create_tag(temp_tag);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _tags_cql_manager->delete_tag(1, temp_uuid);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [cass_result, read_tag] = _tags_cql_manager->get_tag_by_id(1, temp_uuid);

  EXPECT_EQ(cass_result.code(), ResultCode::NOT_FOUND);
}

TEST_F(TestTagsCqlManager, InsertTagsTwice_Test) {
  if (_tags_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_tags()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  TagObject temp_tag(1, temp_uuid, "Test tag", "Blue");

  CqlResult cql_result = _tags_cql_manager->create_tag(temp_tag);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _tags_cql_manager->create_tag(temp_tag);

  EXPECT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}

TEST_F(TestTagsCqlManager, ReadNonexistentTags_Test) {
  if (_tags_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_tags()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  auto [cass_result, read_tag] = _tags_cql_manager->get_tag_by_id(1, temp_uuid);

  EXPECT_EQ(cass_result.code(), ResultCode::NOT_FOUND);
}

TEST_F(TestTagsCqlManager, DeleteNonexistentTags_Test) {
  if (_tags_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_tags()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  CqlResult cql_result = _tags_cql_manager->delete_tag(1, temp_uuid);

  EXPECT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}
