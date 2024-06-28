#include "api_managers/tag_api_manager.hpp"

void TagAPIManager::send_response(
    std::function<void(const HttpResponsePtr &)> &&callback,
    const drogon::HttpStatusCode status_code, const std::string &message) {
  // Create the JSON response
  Json::Value json_response;
  json_response["error"] = message;

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code);
  callback(resp);
}

std::pair<int, std::string> TagAPIManager::get_credentials(
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

void TagAPIManager::create_tag(
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
  if ((*json.get())["name"].empty() || !(*json.get())["name"].isString() ||
      (*json.get())["colour"].empty() || !(*json.get())["colour"].isString()) {
    send_response(std::move(callback), drogon::k400BadRequest,
                  "Missing fields");
    return;
  }

  // Get the fields from the JSON
  std::string name = (*json.get())["name"].asString();
  std::string colour = (*json.get())["colour"].asString();

  // Add the tag to the database
  auto [status_code_response, json_response] =
      _tag_manager->create_tag(school_id, token, name, colour);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void TagAPIManager::get_tags(
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
      _tag_manager->get_all_tags(school_id, token);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void TagAPIManager::get_tag(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string tag_id) {
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

  CassUuid tag_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(tag_id.c_str(), &tag_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid tag id");
    return;
  }

  // Add the tag to the database
  auto [status_code_response, json_response] =
      _tag_manager->get_tag(school_id, tag_uuid, token);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void TagAPIManager::update_tag(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string tag_id) {
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

  // Get the fields from the JSON
  std::optional<std::string> name = std::nullopt;
  std::optional<std::string> colour = std::nullopt;

  if (!(*json.get())["name"].empty() && (*json.get())["name"].isString()) {
    name = (*json.get())["name"].asString();
  }

  if (!(*json.get())["colour"].empty() && (*json.get())["colour"].isString()) {
    colour = (*json.get())["colour"].asString();
  }

  // Check if the fields are valid
  if (!name.has_value() && !colour.has_value()) {
    send_response(std::move(callback), k400BadRequest,
                  "Missing required fields");
    return;
  }

  CassUuid tag_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(tag_id.c_str(), &tag_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid tag id");
    return;
  }

  // Add the tag to the database
  auto [status_code_response, json_response] =
      _tag_manager->update_tag(school_id, tag_uuid, token, name, colour);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void TagAPIManager::delete_tag(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string tag_id) {
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

  CassUuid tag_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(tag_id.c_str(), &tag_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid tag id");
    return;
  }

  // Add the tag to the database
  auto [status_code_response, json_response] =
      _tag_manager->delete_tag(school_id, tag_uuid, token);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void TagAPIManager::add_user_to_tag(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback, std::string tag_id,
    std::string user_id) {
  if (req->parameters().size() != 1 &&
      req->parameters().find("user_id") == req->parameters().end()) {
    send_response(std::move(callback), drogon::k400BadRequest,
                  "Missing fields");
    return;
  }

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

  CassUuid tag_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(tag_id.c_str(), &tag_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid tag id");
    return;
  }

  CassUuid user_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(user_id.c_str(), &user_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid tag id");
    return;
  }

  // Add the tag to the database
  auto [status_code_response, json_response] =
      _tag_manager->create_tag_user_relation(school_id, token, tag_uuid,
                                             user_uuid);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void TagAPIManager::get_users_by_tag(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string tag_id) {
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

  CassUuid tag_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(tag_id.c_str(), &tag_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid tag id");
    return;
  }

  // Add the tag to the database
  auto [status_code_response, json_response] =
      _tag_manager->get_users_by_tag(school_id, token, tag_uuid);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void TagAPIManager::get_tags_by_user(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string user_id) {
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

  std::string header_user_id = req->getParameter("user_id");
  std::optional<CassUuid> user_uuid = std::nullopt;

  if (!header_user_id.empty()) {
    CassUuid user_uuid_temp;
    if (CASS_ERROR_LIB_BAD_PARAMS ==
        cass_uuid_from_string(header_user_id.c_str(), &user_uuid_temp)) {
      send_response(std::move(callback), k400BadRequest, "Invalid user id");
      return;
    }
    user_uuid = user_uuid_temp;
  }

  // Add the tag to the database
  auto [status_code_response, json_response] =
      _tag_manager->get_tags_by_user(school_id, token, user_uuid);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void TagAPIManager::remove_user_from_tag(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback, std::string tag_id,
    std::string user_id) {
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

  CassUuid tag_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(tag_id.c_str(), &tag_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid tag id");
    return;
  }

  CassUuid user_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(user_id.c_str(), &user_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid tag id");
    return;
  }

  // Add the tag to the database
  auto [status_code_response, json_response] =
      _tag_manager->delete_tag_user_relation(school_id, token, tag_uuid,
                                             user_uuid);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}
