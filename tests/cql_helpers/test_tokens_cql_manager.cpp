/**
 * @file test_tokens_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for testing the tokens cql manager
 * @version 0.1
 * @date 2023-01-11
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <gtest/gtest.h>

#include <cql_helpers/tokens_cql_manager.hpp>

class TestTokensCqlManager : public ::testing::Test {
 public:
  TestTokensCqlManager(){};

  virtual ~TestTokensCqlManager(){};

  const std::string CASSANDRA_IP = "127.0.0.1";

  CqlClient* _cql_client = nullptr;
  TokensCqlManager* _tokens_cql_manager = nullptr;

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

    _tokens_cql_manager = new TokensCqlManager(_cql_client);
    cql_result = _tokens_cql_manager->configure(true);

    if (cql_result.code() != ResultCode::OK) {
      delete _cql_client;
      _cql_client = nullptr;
      delete _tokens_cql_manager;
      _tokens_cql_manager = nullptr;

      return;
    }
  }

  void disconnect() {
    if (_cql_client != nullptr) {
      delete _cql_client;
      _cql_client = nullptr;
    }

    if (_tokens_cql_manager != nullptr) {
      delete _tokens_cql_manager;
      _tokens_cql_manager = nullptr;
    }
  }

  bool delete_tokens() {
    CqlResult cql_result =
        _cql_client->execute_statement("TRUNCATE TABLE schools.tokens;");

    return cql_result.code() == ResultCode::OK;
  }
};

TEST_F(TestTokensCqlManager, WriteToken_Test) {
  if (_tokens_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_tokens()) {
    return;
  }

  CqlResult cql_result =
      _tokens_cql_manager->create_token(1, "token1", create_current_uuid());

  EXPECT_EQ(cql_result.code(), ResultCode::OK);
}

TEST_F(TestTokensCqlManager, ReadToken_Test) {
  if (_tokens_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_tokens()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  CqlResult cql_result =
      _tokens_cql_manager->create_token(1, "token1", temp_uuid);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_read_result, read_uuid] =
      _tokens_cql_manager->get_user_from_token(1, "token1");

  EXPECT_EQ(cql_read_result.code(), ResultCode::OK);
  EXPECT_EQ(read_uuid.clock_seq_and_node, temp_uuid.clock_seq_and_node);
}

TEST_F(TestTokensCqlManager, DeleteToken_Test) {
  if (_tokens_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_tokens()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  CqlResult cql_result =
      _tokens_cql_manager->create_token(1, "token1", temp_uuid);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _tokens_cql_manager->delete_token(1, "token1");

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_read_result, read_uuid] =
      _tokens_cql_manager->get_user_from_token(1, "token1");

  EXPECT_EQ(cql_read_result.code(), ResultCode::NOT_FOUND);
}

TEST_F(TestTokensCqlManager, InsertTokensTwice_Test) {
  if (_tokens_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_tokens()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  CqlResult cql_result =
      _tokens_cql_manager->create_token(1, "token1", temp_uuid);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _tokens_cql_manager->create_token(1, "token1", temp_uuid);

  EXPECT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}

TEST_F(TestTokensCqlManager, ReadNonexistentTokens_Test) {
  if (_tokens_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_tokens()) {
    return;
  }

  auto [cql_read_result, read_uuid] =
      _tokens_cql_manager->get_user_from_token(1, "token1");

  EXPECT_EQ(cql_read_result.code(), ResultCode::NOT_FOUND);
}

TEST_F(TestTokensCqlManager, DeleteNonexistentTokens_Test) {
  if (_tokens_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_tokens()) {
    return;
  }

  CqlResult cql_result = _tokens_cql_manager->delete_token(1, "token1");

  EXPECT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}