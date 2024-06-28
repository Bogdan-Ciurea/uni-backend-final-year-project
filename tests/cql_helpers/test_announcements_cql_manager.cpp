/**
 * @file test_announcements_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for testing the announcements cql manager
 * @version 0.1
 * @date 2022-12-16
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <gtest/gtest.h>

#include "cql_helpers/announcements_cql_manager.hpp"

class TestAnnouncementsCqlManager : public ::testing::Test {
 public:
  TestAnnouncementsCqlManager(){};

  virtual ~TestAnnouncementsCqlManager(){};

  const std::string CASSANDRA_IP = "127.0.0.1";

  CqlClient* _cql_client = nullptr;
  AnnouncementsCqlManager* _announcement_cql_manager = nullptr;

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

    _announcement_cql_manager = new AnnouncementsCqlManager(_cql_client);
    cql_result = _announcement_cql_manager->configure(true);

    if (cql_result.code() != ResultCode::OK) {
      delete _cql_client;
      _cql_client = nullptr;
      delete _announcement_cql_manager;
      _announcement_cql_manager = nullptr;

      return;
    }
  }

  void disconnect() {
    if (_cql_client != nullptr) {
      delete _cql_client;
      _cql_client = nullptr;
    }

    if (_announcement_cql_manager != nullptr) {
      delete _announcement_cql_manager;
      _announcement_cql_manager = nullptr;
    }
  }

  bool delete_announcements() {
    CqlResult cql_result =
        _cql_client->execute_statement("TRUNCATE TABLE schools.announcements;");

    return cql_result.code() == ResultCode::OK;
  }
};

TEST_F(TestAnnouncementsCqlManager, WriteAnnouncement_Test) {
  if (_announcement_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_announcements()) {
    return;
  }

  long timestamp = std::time(nullptr);

  AnnouncementObject announcement(
      1, "e7b67300-6ddc-11ed-91a3-d5edd27b9fba", timestamp,
      "e7b67300-6ddc-11ed-91a3-d5edd27b9fba", "This is a test announcement",
      "This is a test announcement content", true,
      std::vector<std::string>{"e7b67300-6ddc-11ed-91a3-d5edd27b9fba",
                               "4f410f90-6ddc-11ed-91a3-d5edd27b9fba"});

  CqlResult cql_result =
      _announcement_cql_manager->create_announcement(announcement);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);
}

TEST_F(TestAnnouncementsCqlManager, ReadAnnouncement_Test) {
  if (_announcement_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_announcements()) {
    return;
  }

  long timestamp = std::time(nullptr);
  std::vector<std::string> files_list = {
      "e7b67300-6ddc-11ed-91a3-d5edd27b9fba",
      "4f410f90-6ddc-11ed-91a3-d5edd27b9fba"};

  AnnouncementObject announcement(
      1, "e7b67300-6ddc-11ed-91a3-d5edd27b9fba", timestamp,
      "e7b67300-6ddc-11ed-91a3-d5edd27b9fba", "This is a test announcement",
      "This is a test announcement content", true, files_list);

  CqlResult cql_result =
      _announcement_cql_manager->create_announcement(announcement);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  auto result =
      _announcement_cql_manager->get_announcement_by_id(1, announcement._id);

  ASSERT_EQ(result.first.code(), ResultCode::OK);

  AnnouncementObject* announcement_read = &result.second;

  ASSERT_EQ(announcement_read->_school_id, 1);
  ASSERT_EQ(announcement_read->_created_at, timestamp);
  ASSERT_EQ(announcement_read->_title, "This is a test announcement");
  ASSERT_EQ(announcement_read->_content, "This is a test announcement content");
  ASSERT_EQ(announcement_read->_allow_answers, true);
  ASSERT_EQ(announcement_read->_files.size(), 2);
  ASSERT_EQ(announcement_read->_id.time_and_version,
            announcement._id.time_and_version);
  for (auto& file : announcement_read->_files) {
    for (int i = 0; i < announcement._files.size(); i++)
      if (file.clock_seq_and_node ==
          announcement._files[i].clock_seq_and_node) {
        announcement._files.erase(announcement._files.begin() + i);
        break;
      }
  }
  ASSERT_EQ(announcement._files.size(), 0);
}

TEST_F(TestAnnouncementsCqlManager, UpdateAnnouncement_Test) {
  if (_announcement_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_announcements()) {
    return;
  }

  long timestamp = std::time(nullptr);
  std::vector<std::string> files_list = {
      "e7b67300-6ddc-11ed-91a3-d5edd27b9fba",
      "4f410f90-6ddc-11ed-91a3-d5edd27b9fba"};

  AnnouncementObject announcement(
      1, "e7b67300-6ddc-11ed-91a3-d5edd27b9fba", timestamp,
      "e7b67300-6ddc-11ed-91a3-d5edd27b9fba", "This is a test announcement",
      "This is a test announcement content", true, files_list);

  CqlResult cql_result =
      _announcement_cql_manager->create_announcement(announcement);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  announcement._title = "This is a test announcement updated";
  announcement._content = "This is a test announcement content updated";
  announcement._allow_answers = false;

  cql_result = _announcement_cql_manager->update_announcement(
      announcement._school_id, announcement._id, announcement._created_at,
      announcement._created_by, announcement._title, announcement._content,
      announcement._allow_answers, announcement._files);

  auto result =
      _announcement_cql_manager->get_announcement_by_id(1, announcement._id);

  ASSERT_EQ(result.first.code(), ResultCode::OK);

  AnnouncementObject* announcement_read = &result.second;

  ASSERT_EQ(announcement_read->_school_id, 1);
  ASSERT_EQ(announcement_read->_created_at, timestamp);
  ASSERT_EQ(announcement_read->_title, announcement._title);
  ASSERT_EQ(announcement_read->_content, announcement._content);
  ASSERT_EQ(announcement_read->_allow_answers, false);
  ASSERT_EQ(announcement_read->_files.size(), 2);
  ASSERT_EQ(announcement_read->_id.time_and_version,
            announcement._id.time_and_version);
  for (auto& file : announcement_read->_files) {
    for (int i = 0; i < announcement._files.size(); i++)
      if (file.clock_seq_and_node ==
          announcement._files[i].clock_seq_and_node) {
        announcement._files.erase(announcement._files.begin() + i);
        break;
      }
  }
  ASSERT_EQ(announcement._files.size(), 0);
}

TEST_F(TestAnnouncementsCqlManager, DeleteAnnouncement_Test) {
  if (_announcement_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_announcements()) {
    return;
  }

  long timestamp = std::time(nullptr);

  AnnouncementObject announcement(
      1, "e7b67300-6ddc-11ed-91a3-d5edd27b9fba", timestamp,
      "e7b67300-6ddc-11ed-91a3-d5edd27b9fba", "This is a test announcement",
      "This is a test announcement content", true,
      std::vector<std::string>{"e7b67300-6ddc-11ed-91a3-d5edd27b9fba",
                               "4f410f90-6ddc-11ed-91a3-d5edd27b9fba"});

  CqlResult cql_result =
      _announcement_cql_manager->create_announcement(announcement);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _announcement_cql_manager->delete_announcement_by_id(
      1, announcement._id, announcement._created_at);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);
}

TEST_F(TestAnnouncementsCqlManager, InsertAnnouncementTwice_Test) {
  if (_announcement_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_announcements()) {
    return;
  }

  long timestamp = std::time(nullptr);

  AnnouncementObject announcement(
      1, "e7b67300-6ddc-11ed-91a3-d5edd27b9fba", timestamp,
      "e7b67300-6ddc-11ed-91a3-d5edd27b9fba", "This is a test announcement",
      "This is a test announcement content", true,
      std::vector<std::string>{"e7b67300-6ddc-11ed-91a3-d5edd27b9fba",
                               "4f410f90-6ddc-11ed-91a3-d5edd27b9fba"});

  CqlResult cql_result =
      _announcement_cql_manager->create_announcement(announcement);

  ASSERT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _announcement_cql_manager->create_announcement(announcement);

  ASSERT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}

TEST_F(TestAnnouncementsCqlManager, ReadNonexistentAnnouncement_Test) {
  if (_announcement_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_announcements()) {
    return;
  }

  auto result = _announcement_cql_manager->get_announcement_by_id(
      1, create_current_uuid());

  ASSERT_EQ(result.first.code(), ResultCode::NOT_FOUND);
}
