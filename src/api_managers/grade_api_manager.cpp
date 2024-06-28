#include "api_managers/grade_api_manager.hpp"

void GradeAPIManager::send_response(
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

std::pair<int, std::string> GradeAPIManager::get_credentials(
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

void GradeAPIManager::create_grade(
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

  // Get the grade from the JSON
  if ((*json.get())["course_id"].empty() ||
      !(*json.get())["course_id"].isString() ||
      (*json.get())["user_id"].empty() ||
      !(*json.get())["user_id"].isString() || (*json.get())["grade"].empty() ||
      !(*json.get())["grade"].isInt()) {
    send_response(std::move(callback), drogon::k400BadRequest,
                  "Missing fields");
    return;
  }

  // Get the grade from the JSON
  std::string course_id = (*json.get())["course_id"].asString();
  std::string user_id = (*json.get())["user_id"].asString();
  int grade = (*json.get())["grade"].asInt();
  std::optional<int> out_of = std::nullopt;
  std::optional<float> weight = std::nullopt;

  if (!(*json.get())["out_of"].empty() && (*json.get())["out_of"].isInt()) {
    out_of.emplace((*json.get())["out_of"].asInt());
  }

  if (!(*json.get())["weight"].empty() && (*json.get())["weight"].isDouble()) {
    weight.emplace((float)(*json.get())["weight"].asDouble());
  }

  CassUuid course_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(course_id.c_str(), &course_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid course id");
    return;
  }

  CassUuid user_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(user_id.c_str(), &user_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid user id");
    return;
  }

  // Add the tag to the database
  auto [status_code_response, json_response] = _grade_manager->add_grade(
      school_id, token, course_uuid, user_uuid, grade, out_of, weight);

  if (status_code_response == drogon::k201Created) {
    // Get the student references
    auto [cql_response0, references] =
        _student_references_cql_manager->get_student_references(school_id,
                                                                user_uuid);

    if (cql_response0.code() == ResultCode::OK ||
        cql_response0.code() == ResultCode::NOT_FOUND) {
      // Send emails to the student references
      for (const auto &ref : references) {
        if (ref._type == ReferenceType::EMAIL) {
          _email_manager->send_grade_email(
              ref._reference, grade, json_response["out_of"].asInt(),
              json_response["course_name"].asString());
        }
      }
    }
  }

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void GradeAPIManager::get_grades(
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

  // Get the grades from the database
  auto [status_code_response, json_response] =
      _grade_manager->get_personal_grades(school_id, token);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void GradeAPIManager::get_user_grades(
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

  // Check if the user id is valid
  CassUuid user_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(user_id.c_str(), &user_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid user id");
    return;
  }

  // Get the grades from the database
  auto [status_code_response, json_response] =
      _grade_manager->get_user_grades(school_id, token, user_uuid);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void GradeAPIManager::get_course_grades(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string course_id) {
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

  // Check if the user id is valid
  CassUuid course_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(course_id.c_str(), &course_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid user id");
    return;
  }

  // Get the grades from the database
  auto [status_code_response, json_response] =
      _grade_manager->get_course_grades(school_id, token, course_uuid);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void GradeAPIManager::update_grade(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string grade_id) {
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

  // Get the grade from the JSON
  if ((*json.get())["grade"].empty() || !(*json.get())["grade"].isInt()) {
    send_response(std::move(callback), drogon::k400BadRequest,
                  "Missing fields");
    return;
  }

  // Get the grade from the JSON
  int grade = (*json.get())["grade"].asInt();
  std::optional<int> out_of = std::nullopt;
  std::optional<float> weight = std::nullopt;

  if (!(*json.get())["out_of"].empty() && (*json.get())["out_of"].isInt()) {
    out_of.emplace((*json.get())["out_of"].asInt());
  }

  if (!(*json.get())["weight"].empty() && (*json.get())["weight"].isDouble()) {
    out_of.emplace((*json.get())["out_of"].asDouble());
  }

  CassUuid grade_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(grade_id.c_str(), &grade_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid course id");
    return;
  }

  // Edit the grade
  auto [status_code_response, json_response] = _grade_manager->edit_grade(
      school_id, token, grade_uuid, grade, out_of, weight);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void GradeAPIManager::delete_grade(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string grade_id) {
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

  CassUuid grade_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(grade_id.c_str(), &grade_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid course id");
    return;
  }

  // Delete the grade
  auto [status_code_response, json_response] =
      _grade_manager->delete_grade(school_id, token, grade_uuid);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}
