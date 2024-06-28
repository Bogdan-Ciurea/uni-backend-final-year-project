#include "relations_managers/todo_manager.hpp"

std::pair<drogon::HttpStatusCode, Json::Value> TodoManager::create_todo(
    const int school_id, const std::string &creator_token,
    const std::string &text, const TodoType type) {
  Json::Value json_response;

  // Get the user from the token
  auto [cql_result0, user_id] =
      _tokens_cql_manager->get_user_from_token(school_id, creator_token);

  if (cql_result0.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The token is invalid";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  } else if (cql_result0.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the user from the token";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the user exists

  auto [cql_result1, user] = _users_cql_manager->get_user(school_id, user_id);

  if (cql_result1.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The user does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_result1.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the user";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Parse and check if the data is valid

  if (text.empty()) {
    json_response["error"] = "The text is empty";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  }

  // Create the todo
  CassUuid todo_id = create_current_uuid();

  TodoObject todo_data(school_id, todo_id, text, type);

  auto cql_result2 = _todos_cql_manager->create_todo(todo_data);

  if (cql_result2.code() != ResultCode::OK) {
    json_response["error"] = "Could not create the todo";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Create the relation between the user and the todo
  auto cql_result3 = _todos_by_user_cql_manager->create_relationship(
      school_id, user_id, todo_id);

  if (cql_result3.code() != ResultCode::OK) {
    json_response["error"] =
        "Could not create the relation between the user and the todo";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Return 201 Created
  json_response["id"] = get_uuid_string(todo_id);
  return {drogon::HttpStatusCode::k201Created, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> TodoManager::get_todo(
    const int school_id, const std::string &token, const CassUuid &todo_id) {
  Json::Value json_response;

  // Get the user from the token
  auto [cql_result0, user_id] =
      _tokens_cql_manager->get_user_from_token(school_id, token);

  if (cql_result0.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The token is invalid";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  } else if (cql_result0.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the user from the token";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the user exists

  auto [cql_result1, user] = _users_cql_manager->get_user(school_id, user_id);

  if (cql_result1.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The user does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_result1.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the user";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Get the list of todos from the user

  auto [cql_result2, todos] =
      _todos_by_user_cql_manager->get_todos_by_user(school_id, user_id);

  if (cql_result2.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The user does not have any todo";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_result2.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the list of todos from the user";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the todo is in the list

  bool in_list = false;
  for (const auto &todo : todos) {
    if (todo.clock_seq_and_node == todo_id.clock_seq_and_node) {
      in_list = true;
      break;
    }
  }

  if (!in_list) {
    json_response["error"] =
        "The todo is not in the list of todos from this user";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  }

  // Get the todo

  auto [cql_result3, todo] =
      _todos_cql_manager->get_todo_by_id(school_id, todo_id);

  if (cql_result3.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The todo does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_result3.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the todo";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Return the todo
  json_response = todo.to_json(true);

  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> TodoManager::get_all_todos(
    const int school_id, const std::string &token) {
  Json::Value json_response;

  // Get the user from the token
  auto [cql_result0, user_id] =
      _tokens_cql_manager->get_user_from_token(school_id, token);

  if (cql_result0.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The token is invalid";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  } else if (cql_result0.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the user from the token";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the user exists

  auto [cql_result1, user] = _users_cql_manager->get_user(school_id, user_id);

  if (cql_result1.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The user does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_result1.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the user";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Get the list of todos from the user

  auto [cql_result2, todos_id] =
      _todos_by_user_cql_manager->get_todos_by_user(school_id, user_id);

  if (cql_result2.code() == ResultCode::NOT_FOUND) {
    json_response = Json::arrayValue;
    return {drogon::HttpStatusCode::k200OK, json_response};
  } else if (cql_result2.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the list of todos from the user";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Get the todos

  for (const auto &todo_id : todos_id) {
    auto [cql_result3, todo] =
        _todos_cql_manager->get_todo_by_id(school_id, todo_id);

    if (cql_result3.code() == ResultCode::NOT_FOUND) {
      json_response["error"] = "The todo does not exist";
      return {drogon::HttpStatusCode::k404NotFound, json_response};
    } else if (cql_result3.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the todo";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    json_response.append(todo.to_json(true));
  }

  // Return the todos

  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> TodoManager::update_todo(
    const int school_id, const std::string &editor_token,
    const CassUuid &todo_id, const std::optional<std::string> text,
    const std::optional<TodoType> type) {
  Json::Value json_response;

  // Get the user from the token
  auto [cql_result0, user_id] =
      _tokens_cql_manager->get_user_from_token(school_id, editor_token);

  if (cql_result0.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The token is invalid";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  } else if (cql_result0.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the user from the token";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the user exists

  auto [cql_result1, user] = _users_cql_manager->get_user(school_id, user_id);

  if (cql_result1.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The user does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_result1.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the user";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Get the list of todos from the user

  auto [cql_result2, todos] =
      _todos_by_user_cql_manager->get_todos_by_user(school_id, user_id);

  if (cql_result2.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the list of todos from the user";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the todo is in the list

  bool in_list = false;
  for (const auto &todo : todos) {
    if (todo.clock_seq_and_node == todo_id.clock_seq_and_node &&
        todo.time_and_version == todo_id.time_and_version) {
      in_list = true;
      break;
    }
  }

  if (!in_list) {
    json_response["error"] =
        "The todo is not in the list of todos from this user";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  }

  // Get the todo

  auto [cql_result3, todo] =
      _todos_cql_manager->get_todo_by_id(school_id, todo_id);

  if (cql_result3.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The todo does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_result3.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the todo";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Update the todo

  if (text.has_value()) {
    todo._text = text.value();
  }
  if (type.has_value()) {
    todo._type = type.value();
  }

  auto cql_result4 = _todos_cql_manager->update_todo(school_id, todo._todo_id,
                                                     todo._text, todo._type);

  if (cql_result4.code() != ResultCode::OK) {
    json_response["error"] = "Could not update the todo";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Return 200 OK
  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> TodoManager::delete_todo(
    const int school_id, const std::string &editor_token,
    const CassUuid &todo_id) {
  Json::Value json_response;

  // Get the user from the token
  auto [cql_result0, user_id] =
      _tokens_cql_manager->get_user_from_token(school_id, editor_token);

  if (cql_result0.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The token is invalid";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  } else if (cql_result0.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the user from the token";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the user exists

  auto [cql_result1, user] = _users_cql_manager->get_user(school_id, user_id);

  if (cql_result1.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The user does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_result1.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the user";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Get the list of todos from the user

  auto [cql_result2, todos] =
      _todos_by_user_cql_manager->get_todos_by_user(school_id, user_id);

  if (cql_result2.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the list of todos from the user";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the todo is in the list

  bool in_list = false;
  for (const auto &todo : todos) {
    if (todo.clock_seq_and_node == todo_id.clock_seq_and_node) {
      in_list = true;
      break;
    }
  }

  if (!in_list) {
    json_response["error"] =
        "The todo is not in the list of todos from this user";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  }

  // Delete the todo

  auto cql_result3 = _todos_cql_manager->delete_todo(school_id, todo_id);

  if (cql_result3.code() != ResultCode::OK) {
    json_response["error"] = "Could not delete the todo";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Delete the relation between the user and the todo

  auto cql_result4 = _todos_by_user_cql_manager->delete_relationship(
      school_id, user_id, todo_id);

  // Return 200 OK

  return {drogon::HttpStatusCode::k200OK, json_response};
}
