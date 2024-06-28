#include "api_managers/todo_api_manager.hpp"

void TodoAPIManager::send_response(
    std::function<void(const HttpResponsePtr &)> &&callback,
    const drogon::HttpStatusCode status_code, const std::string &message) {
  Json::Value json_response;
  json_response["error"] = message;
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code);
  callback(resp);
}

std::pair<int, std::string> TodoAPIManager::get_credentials(
    const std::string authorization_header) {
  // Get the credentials from the authorization header
  auto decoded = jwt::decode(authorization_header);

  // Get the token and the school id
  std::string token = "";
  int school_id = -1;

  try {
    token = decoded.get_payload_claim("token").as_string();
    school_id = std::stoi(decoded.get_payload_claim("school_id").as_string());
  } catch (const std::exception &e) {
    LOG_DEBUG << "Error getting token and school id";
    LOG_DEBUG << e.what();
  }

  return {school_id, token};
}

void TodoAPIManager::create_todo(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  // Check if the header has the Authorization field
  // It should be of the format 'Bearer <token>'
  std::string header_token = req->getHeader("Authorization");
  if (header_token.empty() || header_token.find(" ") == std::string::npos ||
      header_token.substr(0, 6) != "Bearer") {
    send_response(std::move(callback), k400BadRequest,
                  "Missing required fields");
    return;
  }
  header_token = header_token.substr(header_token.find(" ") + 1);

  auto [school_id, token] = get_credentials(header_token);

  // Check if the headers have the required fields
  if (school_id <= 0 || token.empty()) {
    send_response(std::move(callback), k400BadRequest,
                  "Invalid required fields");
    return;
  }

  // Get the JSON from the request
  auto json = req->getJsonObject();

  // Check if the JSON is valid
  if (!json) {
    send_response(std::move(callback), drogon::k400BadRequest, "Invalid JSON");
    return;
  }

  // Check if the JSON has the required fields
  if ((*json.get())["text"].empty() || !(*json.get())["text"].isString() ||
      (*json.get())["type"].empty() || !(*json.get())["type"].isString()) {
    send_response(std::move(callback), drogon::k400BadRequest,
                  "Missing fields");
    return;
  }

  // Get the fields from the JSON
  std::string text = (*json.get())["text"].asString();
  std::string str_type = (*json.get())["type"].asString();
  TodoType type;

  // Check if the type is valid
  if (str_type != "NOT_STARTED" && str_type != "IN_PROGRESS" &&
      str_type != "DONE") {
    send_response(std::move(callback), drogon::k400BadRequest, "Invalid type");
    return;
  }

  if (str_type == "NOT_STARTED")
    type = TodoType::NOT_STARTED;
  else if (str_type == "IN_PROGRESS")
    type = TodoType::IN_PROGRESS;
  else
    type = TodoType::DONE;

  // Add the tag to the database
  auto [status_code_response, json_response] =
      _todo_manager->create_todo(school_id, token, text, type);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void TodoAPIManager::get_todos(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  // Check if the header has the Authorization field
  // It should be of the format 'Bearer <token>'
  std::string header_token = req->getHeader("Authorization");
  if (header_token.empty() || header_token.find(" ") == std::string::npos ||
      header_token.substr(0, 6) != "Bearer") {
    send_response(std::move(callback), k400BadRequest,
                  "Missing required fields");
    return;
  }
  header_token = header_token.substr(header_token.find(" ") + 1);

  auto [school_id, token] = get_credentials(header_token);

  // Check if the headers have the required fields
  if (school_id <= 0 || token.empty()) {
    send_response(std::move(callback), k400BadRequest,
                  "Invalid required fields");
    return;
  }

  // Add the tag to the database
  auto [status_code_response, json_response] =
      _todo_manager->get_all_todos(school_id, token);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void TodoAPIManager::get_todo(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string todo_id) {
  // Check if the header has the Authorization field
  // It should be of the format 'Bearer <token>'
  std::string header_token = req->getHeader("Authorization");
  if (header_token.empty() || header_token.find(" ") == std::string::npos ||
      header_token.substr(0, 6) != "Bearer") {
    send_response(std::move(callback), k400BadRequest,
                  "Missing required fields");
    return;
  }
  header_token = header_token.substr(header_token.find(" ") + 1);

  auto [school_id, token] = get_credentials(header_token);

  // Check if the headers have the required fields
  if (school_id <= 0 || token.empty()) {
    send_response(std::move(callback), k400BadRequest,
                  "Invalid required fields");
    return;
  }

  CassUuid todo_uuid;
  if (cass_uuid_from_string(todo_id.c_str(), &todo_uuid) != CASS_OK) {
    send_response(std::move(callback), drogon::k400BadRequest,
                  "Invalid todo id");
    return;
  }

  // Add the tag to the database
  auto [status_code_response, json_response] =
      _todo_manager->get_todo(school_id, token, todo_uuid);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void TodoAPIManager::update_todo(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string todo_id) {
  // Check if the header has the Authorization field
  // It should be of the format 'Bearer <token>'
  std::string header_token = req->getHeader("Authorization");
  if (header_token.empty() || header_token.find(" ") == std::string::npos ||
      header_token.substr(0, 6) != "Bearer") {
    send_response(std::move(callback), k400BadRequest,
                  "Missing required fields");
    return;
  }
  header_token = header_token.substr(header_token.find(" ") + 1);

  auto [school_id, token] = get_credentials(header_token);

  // Check if the headers have the required fields
  if (school_id <= 0 || token.empty()) {
    send_response(std::move(callback), k400BadRequest,
                  "Invalid required fields");
    return;
  }

  // Get the JSON from the request
  auto json = req->getJsonObject();

  // Check if the JSON is valid
  if (!json) {
    send_response(std::move(callback), drogon::k400BadRequest, "Invalid JSON");
    return;
  }

  std::optional<std::string> text = nullopt;
  std::optional<std::string> str_type = (*json.get())["type"].asString();
  std::optional<TodoType> type = nullopt;

  if (!(*json.get())["text"].empty()) text = (*json.get())["text"].asString();

  // Check if the type is valid
  if (str_type != "") {
    // Check if the type is valid
    if (str_type != "NOT_STARTED" && str_type != "IN_PROGRESS" &&
        str_type != "DONE") {
      send_response(std::move(callback), drogon::k400BadRequest,
                    "Invalid type");
      return;
    }

    if (str_type == "NOT_STARTED")
      type = TodoType::NOT_STARTED;
    else if (str_type == "IN_PROGRESS")
      type = TodoType::IN_PROGRESS;
    else
      type = TodoType::DONE;
  }

  CassUuid todo_uuid;
  if (cass_uuid_from_string(todo_id.c_str(), &todo_uuid) ==
      CASS_ERROR_LIB_BAD_PARAMS) {
    send_response(std::move(callback), drogon::k400BadRequest,
                  "Invalid todo id");
    return;
  }

  if (text == nullopt && type == nullopt) {
    send_response(std::move(callback), drogon::k400BadRequest,
                  "Missing fields");
    return;
  }

  // Add the tag to the database
  auto [status_code_response, json_response] =
      _todo_manager->update_todo(school_id, token, todo_uuid, text, type);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void TodoAPIManager::delete_todo(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string todo_id) {
  // Check if the header has the Authorization field
  // It should be of the format 'Bearer <token>'
  std::string header_token = req->getHeader("Authorization");
  if (header_token.empty() || header_token.find(" ") == std::string::npos ||
      header_token.substr(0, 6) != "Bearer") {
    send_response(std::move(callback), k400BadRequest,
                  "Missing required fields");
    return;
  }
  header_token = header_token.substr(header_token.find(" ") + 1);

  auto [school_id, token] = get_credentials(header_token);

  // Check if the headers have the required fields
  if (school_id <= 0 || token.empty()) {
    send_response(std::move(callback), k400BadRequest,
                  "Invalid required fields");
    return;
  }

  CassUuid todo_uuid;
  if (cass_uuid_from_string(todo_id.c_str(), &todo_uuid) != CASS_OK) {
    send_response(std::move(callback), drogon::k400BadRequest,
                  "Invalid todo id");
    return;
  }

  // Add the tag to the database
  auto [status_code_response, json_response] =
      _todo_manager->delete_todo(school_id, token, todo_uuid);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}
