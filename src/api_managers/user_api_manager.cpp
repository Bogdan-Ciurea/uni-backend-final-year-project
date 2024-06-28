#include "api_managers/user_api_manager.hpp"

void UserAPIManager::send_response(
    std::function<void(const HttpResponsePtr &)> &&callback,
    const drogon::HttpStatusCode status_code, const std::string &message) {
  Json::Value json_response;
  json_response["error"] = message;
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code);
  callback(resp);
}

std::pair<int, std::string> UserAPIManager::get_credentials(
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

void UserAPIManager::create_user(
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
  if ((*json.get())["email"].empty() || !(*json.get())["email"].isString() ||
      (*json.get())["type"].empty() || !(*json.get())["type"].isInt() ||
      (*json.get())["first_name"].empty() ||
      !(*json.get())["first_name"].isString() ||
      (*json.get())["last_name"].empty() ||
      !(*json.get())["last_name"].isString()) {
    send_response(std::move(callback), drogon::k400BadRequest,
                  "Missing fields");
    return;
  }

  // Get the fields from the JSON
  std::string email = (*json.get())["email"].asString();
  int int_type = (*json.get())["type"].asInt();
  UserType type;
  std::string first_name = (*json.get())["first_name"].asString();
  std::string last_name = (*json.get())["last_name"].asString();
  std::optional<std::string> phone_number = std::nullopt;

  if (!(*json.get())["phone_number"].empty() &&
      (*json.get())["phone_number"].isString()) {
    phone_number = (*json.get())["phone_number"].asString();
  }

  // Check if the type is valid
  if (int_type == 2) {
    type = UserType::STUDENT;
  } else if (int_type == 0) {
    type = UserType::ADMIN;
  } else if (int_type == 1) {
    type = UserType::TEACHER;
  } else {
    send_response(std::move(callback), drogon::k400BadRequest, "Invalid type");
    return;
  }

  // Create the user
  // Add the tag to the database
  auto [status_code_response, json_response] = _user_manager->create_user(
      school_id, token, email, type, first_name, last_name,
      phone_number.has_value() ? phone_number.value() : "");

  if (status_code_response == drogon::k201Created) {
    std::string password = json_response["password"].asString();

    json_response.removeMember("password");

    // Send the email to the user with the password
    email_manager->send_email(email, first_name, last_name, password);
  }

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void UserAPIManager::get_users(
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
      _user_manager->get_all_users(school_id, token);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void UserAPIManager::get_user(
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

  CassUuid user_uuid;
  if (cass_uuid_from_string(user_id.c_str(), &user_uuid) != CASS_OK) {
    send_response(std::move(callback), drogon::k400BadRequest,
                  "Invalid todo id");
    return;
  }

  auto [status_code_response, json_response] =
      _user_manager->get_user(school_id, token, user_uuid);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void UserAPIManager::update_user(
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

  // Get the JSON from the request
  auto json = req->getJsonObject();

  // Check if the JSON is valid
  if (!json) {
    send_response(std::move(callback), drogon::k400BadRequest, "Invalid JSON");
    return;
  }
  CassUuid user_uuid;
  if (cass_uuid_from_string(user_id.c_str(), &user_uuid) != CASS_OK) {
    send_response(std::move(callback), drogon::k400BadRequest,
                  "Invalid todo id");
    return;
  }

  // Get the optional fields from the JSON
  std::optional<std::string> email = std::nullopt;
  std::optional<std::string> password = std::nullopt;
  std::optional<UserType> user_type = std::nullopt;
  std::optional<std::string> first_name = std::nullopt;
  std::optional<std::string> last_name = std::nullopt;
  std::optional<std::string> phone_number = std::nullopt;

  if (!(*json.get())["email"].empty() && (*json.get())["email"].isString()) {
    email = (*json.get())["email"].asString();
  }
  if (!(*json.get())["password"].empty() &&
      (*json.get())["password"].isString()) {
    password = (*json.get())["password"].asString();
  }
  if (!(*json.get())["user_type"].empty() &&
      (*json.get())["user_type"].isInt()) {
    const int int_user_type = (*json.get())["user_type"].asInt();
    // Check if the type is valid
    if (int_user_type == 2) {
      user_type = UserType::STUDENT;
    } else if (int_user_type == 0) {
      user_type = UserType::ADMIN;
    } else if (int_user_type == 1) {
      user_type = UserType::TEACHER;
    } else {
      send_response(std::move(callback), drogon::k400BadRequest,
                    "Invalid type");
      return;
    }
  }
  if (!(*json.get())["first_name"].empty() &&
      (*json.get())["first_name"].isString()) {
    first_name = (*json.get())["first_name"].asString();
  }
  if (!(*json.get())["last_name"].empty() &&
      (*json.get())["last_name"].isString()) {
    last_name = (*json.get())["last_name"].asString();
  }
  if (!(*json.get())["phone_number"].empty() &&
      (*json.get())["phone_number"].isString()) {
    phone_number = (*json.get())["phone_number"].asString();
  }

  // Check if there is at least one field to update
  if (!email && !password && !user_type && !first_name && !last_name &&
      !phone_number) {
    send_response(std::move(callback), drogon::k400BadRequest,
                  "Missing fields");
    return;
  }

  auto [status_code_response, json_response] = _user_manager->update_user(
      school_id, token, user_uuid, email, password, user_type, first_name,
      last_name, phone_number);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void UserAPIManager::delete_user(
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

  CassUuid user_uuid;
  if (cass_uuid_from_string(user_id.c_str(), &user_uuid) != CASS_OK) {
    send_response(std::move(callback), drogon::k400BadRequest,
                  "Invalid todo id");
    return;
  }

  auto [status_code_response, json_response] =
      _user_manager->delete_user(school_id, token, user_uuid);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void UserAPIManager::log_in(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  // Get the JSON from the request
  auto json = req->getJsonObject();

  // Check if the JSON is valid
  if (!json) {
    send_response(std::move(callback), drogon::k400BadRequest, "Invalid JSON");
    return;
  }

  // Check if the JSON has the required fields
  if ((*json.get())["school_id"].empty() ||
      !(*json.get())["school_id"].isInt() || (*json.get())["email"].empty() ||
      !(*json.get())["email"].isString() || (*json.get())["password"].empty() ||
      !(*json.get())["password"].isString()) {
    send_response(std::move(callback), drogon::k400BadRequest,
                  "Missing fields");
    return;
  }

  // Get the fields from the JSON
  int school_id = (*json.get())["school_id"].asInt();
  std::string email = (*json.get())["email"].asString();
  std::string password = (*json.get())["password"].asString();

  auto [status_code_response, json_response] =
      _user_manager->log_in(school_id, email, password);

  auto token =
      jwt::create()
          .set_issuer("auth0")
          .set_type("JWT")
          .set_id("rsa-create-example")
          .set_issued_at(std::chrono::system_clock::now())
          .set_expires_at(std::chrono::system_clock::now() +
                          std::chrono::hours{2160})  // 90 days
          .set_payload_claim("token",
                             jwt::claim(json_response["token"].asString()))
          .set_payload_claim("school_id", jwt::claim(std::to_string(school_id)))
          .sign(jwt::algorithm::hs256{_private_key});

  json_response["token"] = token;

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void UserAPIManager::log_out(
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

  auto [status_code_response, json_response] =
      _user_manager->log_out(school_id, token);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}
