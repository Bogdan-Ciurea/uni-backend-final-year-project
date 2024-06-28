/**
 * @file todo_api_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief The purpose of this file is to define the manager of
 * the apis for the todo related objects
 * @version 0.1
 * @date 2023-02-08
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef TODO_API_MANAGER_H
#define TODO_API_MANAGER_H

#include "../relations_managers/todo_manager.hpp"
#include "jwt-cpp/jwt.h"

using namespace drogon;

class TodoAPIManager : public HttpController<TodoAPIManager, false> {
 public:
  TodoAPIManager(TodoManager *todo_manager, std::string public_key)
      : _todo_manager(todo_manager), _public_key(public_key) {
    app().registerHandler("/api/todos", &TodoAPIManager::create_todo, {Post});
    app().registerHandler("/api/todos", &TodoAPIManager::get_todos, {Get});
    app().registerHandler("/api/todos/{todo-id}", &TodoAPIManager::get_todo,
                          {Get});
    app().registerHandler("/api/todos/{todo-id}", &TodoAPIManager::update_todo,
                          {Put});
    app().registerHandler("/api/todos/{todo-id}", &TodoAPIManager::delete_todo,
                          {Delete});
  }
  ~TodoAPIManager(){};

  METHOD_LIST_BEGIN
  METHOD_LIST_END

 protected:
  void create_todo(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback);
  void get_todos(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback);
  void get_todo(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback,
                std::string todo_id);
  void update_todo(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback,
                   std::string todo_id);
  void delete_todo(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback,
                   std::string todo_id);

 private:
  TodoManager *_todo_manager;
  std::string _public_key;

  void send_response(std::function<void(const HttpResponsePtr &)> &&callback,
                     const drogon::HttpStatusCode status_code,
                     const std::string &message);

  /**
   * @brief Get the credentials from the authorization header.
   * It will return a pair of the school id and the token.
   * If there was an error, the school id will be -1 and the token will be
   * empty.
   *
   * @param authorization_header - The authorization header.
   * @return std::pair<int, std::string>
   */
  std::pair<int, std::string> get_credentials(
      const std::string authorization_header);
};

#endif