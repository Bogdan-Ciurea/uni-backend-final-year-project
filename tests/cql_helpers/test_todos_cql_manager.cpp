/**
 * @file test_todos_cql_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief This file is responsible for testing the todos cql manager
 * @version 0.1
 * @date 2023-01-11
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <gtest/gtest.h>

#include <cql_helpers/todos_cql_manager.hpp>

class TestTodosCqlManager : public ::testing::Test {
 public:
  TestTodosCqlManager(){};

  virtual ~TestTodosCqlManager(){};

  const std::string CASSANDRA_IP = "127.0.0.1";

  CqlClient* _cql_client = nullptr;
  TodosCqlManager* _todos_cql_manager = nullptr;

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

    _todos_cql_manager = new TodosCqlManager(_cql_client);
    cql_result = _todos_cql_manager->configure(true);

    if (cql_result.code() != ResultCode::OK) {
      delete _cql_client;
      _cql_client = nullptr;
      delete _todos_cql_manager;
      _todos_cql_manager = nullptr;

      return;
    }
  }

  void disconnect() {
    if (_cql_client != nullptr) {
      delete _cql_client;
      _cql_client = nullptr;
    }

    if (_todos_cql_manager != nullptr) {
      delete _todos_cql_manager;
      _todos_cql_manager = nullptr;
    }
  }

  bool delete_todos() {
    CqlResult cql_result =
        _cql_client->execute_statement("TRUNCATE TABLE schools.todos;");

    return cql_result.code() == ResultCode::OK;
  }
};

TEST_F(TestTodosCqlManager, WriteTodo_Test) {
  if (_todos_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_todos()) {
    return;
  }

  TodoObject todo(1, create_current_uuid(), "Test todo", TodoType::NOT_STARTED);

  CqlResult cql_result = _todos_cql_manager->create_todo(todo);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);
}

TEST_F(TestTodosCqlManager, ReadTodo_Test) {
  if (_todos_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_todos()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  TodoObject todo(1, temp_uuid, "Test todo", TodoType::NOT_STARTED);

  CqlResult cql_result = _todos_cql_manager->create_todo(todo);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_result_read, todo_read] =
      _todos_cql_manager->get_todo_by_id(1, temp_uuid);

  EXPECT_EQ(cql_result_read.code(), ResultCode::OK);
  EXPECT_EQ(todo_read._school_id, 1);
  EXPECT_EQ(todo_read._todo_id.clock_seq_and_node,
            temp_uuid.clock_seq_and_node);
  EXPECT_EQ(todo_read._text, "Test todo");
  EXPECT_EQ(todo_read._type, TodoType::NOT_STARTED);
}

TEST_F(TestTodosCqlManager, UpdateTodo_Test) {
  if (_todos_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_todos()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  TodoObject todo(1, temp_uuid, "Test todo", TodoType::NOT_STARTED);

  CqlResult cql_result = _todos_cql_manager->create_todo(todo);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _todos_cql_manager->update_todo(1, temp_uuid, "Test todo 2",
                                               TodoType::IN_PROGRESS);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_result_read, todo_read] =
      _todos_cql_manager->get_todo_by_id(1, temp_uuid);

  EXPECT_EQ(cql_result_read.code(), ResultCode::OK);
  EXPECT_EQ(todo_read._school_id, 1);
  EXPECT_EQ(todo_read._todo_id.clock_seq_and_node,
            temp_uuid.clock_seq_and_node);
  EXPECT_EQ(todo_read._text, "Test todo 2");
  EXPECT_EQ(todo_read._type, TodoType::IN_PROGRESS);
}

TEST_F(TestTodosCqlManager, DeleteTodo_Test) {
  if (_todos_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_todos()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  TodoObject todo(1, temp_uuid, "Test todo", TodoType::NOT_STARTED);

  CqlResult cql_result = _todos_cql_manager->create_todo(todo);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _todos_cql_manager->delete_todo(1, temp_uuid);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  auto [cql_result_read, todo_read] =
      _todos_cql_manager->get_todo_by_id(1, temp_uuid);

  EXPECT_EQ(cql_result_read.code(), ResultCode::NOT_FOUND);
}

TEST_F(TestTodosCqlManager, InsertTodosTwice_Test) {
  if (_todos_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_todos()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();
  TodoObject todo(1, temp_uuid, "Test todo", TodoType::NOT_STARTED);

  CqlResult cql_result = _todos_cql_manager->create_todo(todo);

  EXPECT_EQ(cql_result.code(), ResultCode::OK);

  cql_result = _todos_cql_manager->create_todo(todo);

  EXPECT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}

TEST_F(TestTodosCqlManager, ReadNonexistentTodos_Test) {
  if (_todos_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_todos()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  auto [cql_result_read, todo_read] =
      _todos_cql_manager->get_todo_by_id(1, temp_uuid);

  EXPECT_EQ(cql_result_read.code(), ResultCode::NOT_FOUND);
}

TEST_F(TestTodosCqlManager, DeleteNonexistentTodos_Test) {
  if (_todos_cql_manager == nullptr) {
    // Test case is disabled
    return;
  }

  if (!delete_todos()) {
    return;
  }

  CassUuid temp_uuid = create_current_uuid();

  CqlResult cql_result = _todos_cql_manager->delete_todo(1, temp_uuid);

  EXPECT_EQ(cql_result.code(), ResultCode::NOT_APPLIED);
}