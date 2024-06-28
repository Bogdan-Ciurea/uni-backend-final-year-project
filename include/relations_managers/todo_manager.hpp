/**
 * @file todo_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief The purpose of this class is to manage the todos and the relations
 * between them and other entities.
 * @version 0.1
 * @date 2023-02-01
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef TODO_MANAGER_H
#define TODO_MANAGER_H

#include "../cql_helpers/todos_by_user_cql_manager.hpp"
#include "../cql_helpers/todos_cql_manager.hpp"
#include "../cql_helpers/tokens_cql_manager.hpp"
#include "../cql_helpers/users_cql_manager.hpp"
#include "drogon/drogon.h"

class TodoManager {
 public:
  TodoManager(UsersCqlManager *usersCqlManager,
              TokensCqlManager *tokensCqlManager,
              TodosCqlManager *todosCqlManager,
              TodosByUserCqlManager *todosByUserCqlManager)
      : _users_cql_manager(usersCqlManager),
        _tokens_cql_manager(tokensCqlManager),
        _todos_cql_manager(todosCqlManager),
        _todos_by_user_cql_manager(todosByUserCqlManager) {}
  ~TodoManager() {}

  ///
  /// The methods bellow are used to manage the todos and the relations
  /// between them and other entities. They are used to create, read and
  /// delete the todos. They are also used to validate the todos. If the todo is
  /// valid, the method will return a pair with the HTTP status code 200 or 201
  /// and an empty JSON object. If the todo is not valid, the method will return
  /// a pair with the HTTP status code 400 or 404 and a JSON object containing
  /// the error message. If the todo could not be created, read or
  /// deleted due to internal errors, the method will return a pair with the
  /// HTTP status code 500 and a JSON object containing the error message.
  ///

  // Todo related methods
  std::pair<drogon::HttpStatusCode, Json::Value> create_todo(
      const int school_id, const std::string &creator_token,
      const std::string &text, const TodoType type);
  std::pair<drogon::HttpStatusCode, Json::Value> get_todo(
      const int school_id, const std::string &token, const CassUuid &todo_id);
  std::pair<drogon::HttpStatusCode, Json::Value> get_all_todos(
      const int school_id, const std::string &token);
  std::pair<drogon::HttpStatusCode, Json::Value> update_todo(
      const int school_id, const std::string &editor_token,
      const CassUuid &todo_id, const std::optional<std::string> text,
      const std::optional<TodoType> type);
  std::pair<drogon::HttpStatusCode, Json::Value> delete_todo(
      const int school_id, const std::string &editor_token,
      const CassUuid &todo_id);

 private:
  UsersCqlManager *_users_cql_manager;
  TokensCqlManager *_tokens_cql_manager;
  TodosCqlManager *_todos_cql_manager;
  TodosByUserCqlManager *_todos_by_user_cql_manager;
};

#endif
