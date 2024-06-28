#include "api_managers/announcement_api_manager.hpp"

void AnnouncementAPIManager::send_response(
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

bool AnnouncementAPIManager::is_file_name_valid(const std::string file_name) {
  // Allowed characters are: a-z, A-Z, 0-9, -, _ and space .
  // The file name must not start with a dot and contain only one dot
  // The file name must not be longer than 255 characters
  // The file name must not be empty
  // The file has to have a name and a supported extension
  if (file_name.empty() || file_name.size() > 255) {
    return false;
  }

  if (file_name[0] == '.') {
    return false;
  }

  int dot_count = 0;
  for (char c : file_name) {
    if (c == '.') {
      dot_count++;
    } else if (!isalnum(c) && c != '-' && c != '_' && c != ' ') {
      return false;
    }
  }

  if (dot_count != 1) {
    return false;
  }

  // Get the file extension
  std::string file_extension = file_name.substr(file_name.find_last_of("."));

  // Check if the file extension is supported
  if (_mime_types.find(file_extension) == _mime_types.end()) {
    return false;
  }

  return true;
}

std::pair<int, std::string> AnnouncementAPIManager::get_credentials(
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

void AnnouncementAPIManager::create_announcement(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  // Get the JSON from the request
  auto json = req->getJsonObject();

  // Check if the JSON is valid
  if (!json) {
    send_response(std::move(callback), k400BadRequest, "Invalid JSON");
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

  // Check if the JSON has the required fields
  if ((*json.get())["title"].empty() || !(*json.get())["title"].isString() ||
      (*json.get())["content"].empty() ||
      !(*json.get())["content"].isString() ||
      (*json.get())["allow_answers"].empty() ||
      !(*json.get())["allow_answers"].isBool()) {
    send_response(std::move(callback), k400BadRequest,
                  "Missing required fields");
    return;
  }

  // Get the fields from the JSON
  std::string name = (*json.get())["title"].asString();
  std::string content = (*json.get())["content"].asString();
  bool allow_answers = (*json.get())["allow_answers"].asBool();

  auto [status_code_response, json_response] =
      _announcement_manager->create_announcement(school_id, token, name,
                                                 content, allow_answers);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void AnnouncementAPIManager::get_user_announcements(
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
      _announcement_manager->get_announcements(school_id, token);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void AnnouncementAPIManager::delete_announcement(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string announcement_id) {
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

  CassUuid announcement_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(announcement_id.c_str(), &announcement_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid course id");
    return;
  }

  auto [status_code_response, json_response] =
      _announcement_manager->delete_announcement(school_id, token,
                                                 announcement_uuid);

  if (status_code_response == k200OK) {
    std::string file_path = json_response["path"].asString();
    if (std::filesystem::exists(file_path)) {
      std::filesystem::remove_all(file_path);
    }
    json_response = Json::Value();
  }

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void AnnouncementAPIManager::create_announcement_file(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string announcement_id) {
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

  // std::string header_file_name = req->getHeader("file_name");
  // std::string file_name = header_file_name;
  CassUuid announcement_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(announcement_id.c_str(), &announcement_uuid)) {
    send_response(std::move(callback), k400BadRequest,
                  "Invalid announcement id");
    return;
  }

  MultiPartParser fileUpload;
  if (fileUpload.parse(req) != 0 || fileUpload.getFiles().size() != 1 ||
      fileUpload.getParameters().empty() ||
      fileUpload.getParameters().find("file_name") ==
          fileUpload.getParameters().end()) {
    send_response(
        std::move(callback), k403Forbidden,
        "Must be one file and a file_name variable inside the request!");
    return;
  }

  try {
    auto &file = fileUpload.getFiles()[0];
    auto file_name = fileUpload.getParameters().at("file_name");

    if (!is_file_name_valid(file.getFileName())) {
      send_response(std::move(callback), k400BadRequest,
                    "The file that you uploaded has an invalid meme type or an "
                    "invalid file name!");
      return;
    }

    // Get the file extension
    std::string file_extension =
        file.getFileName().substr(file.getFileName().find_last_of("."));

    auto [status_code_response, json_response] =
        _announcement_manager->create_announcement_file(
            school_id, token, announcement_uuid, file_name, file_extension);

    // If the file was created, save the file
    if (status_code_response == k201Created) {
      // Get the file id
      std::string path_to_file = json_response["path_to_file"].asString();

      file.saveAs(path_to_file);

      // Delete the path_to_file field
      json_response.removeMember("path_to_file");
    }
    // Send the response
    auto resp = HttpResponse::newHttpJsonResponse(json_response);
    resp->setStatusCode(status_code_response);
    callback(resp);

  } catch (const std::exception &e) {
    send_response(std::move(callback), k400BadRequest,
                  "The file that you uploaded has an invalid meme type or an "
                  "invalid file name!");
    return;
  }
}

void AnnouncementAPIManager::get_announcement_file(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string announcement_id, std::string file_id) {
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

  CassUuid announcement_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(announcement_id.c_str(), &announcement_uuid)) {
    send_response(std::move(callback), k400BadRequest,
                  "Invalid announcement id");
    return;
  }

  CassUuid file_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(file_id.c_str(), &file_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid file id");
    return;
  }

  auto [status_code_response, json_response] =
      _announcement_manager->has_permission_to_get_file(
          school_id, token, announcement_uuid, file_uuid);

  if (status_code_response != k200OK) {
    auto resp = HttpResponse::newHttpJsonResponse(json_response);
    resp->setStatusCode(status_code_response);
    callback(resp);
    return;
  }

  std::string file_path = json_response["path"].asString();

  if (file_path.empty()) {
    send_response(std::move(callback), k500InternalServerError,
                  "file not found!");
    return;
  }

  std::string file_extension = file_path.substr(file_path.find_last_of("."));
  std::string mime_type = _mime_types.at(file_extension);

  auto resp =
      HttpResponse::newFileResponse(file_path, "", CT_CUSTOM, mime_type);
  resp->setStatusCode(k200OK);
  callback(resp);
}

void AnnouncementAPIManager::delete_announcement_file(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string announcement_id, std::string file_id) {
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

  CassUuid announcement_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(announcement_id.c_str(), &announcement_uuid)) {
    send_response(std::move(callback), k400BadRequest,
                  "Invalid announcement id");
    return;
  }

  CassUuid file_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(file_id.c_str(), &file_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid file id");
    return;
  }

  auto [status_code_response, json_response] =
      _announcement_manager->delete_announcement_file(
          school_id, token, announcement_uuid, file_uuid);

  if (status_code_response == k200OK) {
    // Delete the file from the path
    std::string file_path = json_response["path"].asString();

    if (std::filesystem::exists(file_path)) {
      std::filesystem::remove_all(file_path);
    }

    json_response = Json::Value();
  }

  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void AnnouncementAPIManager::add_tags_to_announcement(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string announcement_id) {
  // Get the JSON from the request
  auto json = req->getJsonObject();

  // Check if the JSON is valid
  if (!json) {
    send_response(std::move(callback), k400BadRequest, "Invalid JSON");
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

  // Check if the JSON has the required fields
  if ((*json.get())["tags"].empty() || !(*json.get())["tags"].isArray()) {
    send_response(std::move(callback), k400BadRequest,
                  "Missing required fields");
    return;
  }

  std::vector<std::string> tags;

  for (auto tag : (*json.get())["tags"]) {
    tags.push_back(tag.asString());
  }

  CassUuid announcement_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(announcement_id.c_str(), &announcement_uuid)) {
    send_response(std::move(callback), k400BadRequest,
                  "Invalid announcement id");
    return;
  }

  for (auto tag : tags) {
    if (tag.empty()) {
      send_response(std::move(callback), k400BadRequest, "Invalid tag");
      return;
    }

    CassUuid tag_uuid;
    if (CASS_ERROR_LIB_BAD_PARAMS ==
        cass_uuid_from_string(tag.c_str(), &tag_uuid)) {
      send_response(std::move(callback), k400BadRequest, "Invalid tag");
      return;
    }

    auto [status_code_response, json_response] =
        _announcement_manager->add_tag_to_announcement(
            school_id, token, announcement_uuid, tag_uuid);

    if (status_code_response != k200OK) {
      auto resp = HttpResponse::newHttpJsonResponse(json_response);
      resp->setStatusCode(status_code_response);
      callback(resp);
      return;
    }
  }

  auto resp = HttpResponse::newHttpJsonResponse(Json::Value());
  resp->setStatusCode(k200OK);
  callback(resp);
}

void AnnouncementAPIManager::get_announcement_tags(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string announcement_id) {
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

  CassUuid announcement_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(announcement_id.c_str(), &announcement_uuid)) {
    send_response(std::move(callback), k400BadRequest,
                  "Invalid announcement id");
    return;
  }

  auto [status_code_response, json_response] =
      _announcement_manager->get_announcement_tags(school_id, token,
                                                   announcement_uuid);

  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void AnnouncementAPIManager::remove_tags_from_announcement(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string announcement_id) {
  // Get the JSON from the request
  auto json = req->getJsonObject();

  // Check if the JSON is valid
  if (!json) {
    send_response(std::move(callback), k400BadRequest, "Invalid JSON");
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

  // Check if the JSON has the required fields
  if ((*json.get())["tags"].empty() || !(*json.get())["tags"].isArray()) {
    send_response(std::move(callback), k400BadRequest,
                  "Missing required fields");
    return;
  }

  std::vector<std::string> tags;

  for (auto tag : (*json.get())["tags"]) {
    tags.push_back(tag.asString());
  }

  CassUuid announcement_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(announcement_id.c_str(), &announcement_uuid)) {
    send_response(std::move(callback), k400BadRequest,
                  "Invalid announcement id");
    return;
  }

  for (auto tag : tags) {
    if (tag.empty()) {
      send_response(std::move(callback), k400BadRequest, "Invalid tag");
      return;
    }

    CassUuid tag_uuid;
    if (CASS_ERROR_LIB_BAD_PARAMS ==
        cass_uuid_from_string(tag.c_str(), &tag_uuid)) {
      send_response(std::move(callback), k400BadRequest, "Invalid tag");
      return;
    }

    auto [status_code_response, json_response] =
        _announcement_manager->remove_tag_from_announcement(
            school_id, token, announcement_uuid, tag_uuid);

    if (status_code_response != k200OK) {
      auto resp = HttpResponse::newHttpJsonResponse(json_response);
      resp->setStatusCode(status_code_response);
      callback(resp);
      return;
    }
  }

  auto resp = HttpResponse::newHttpJsonResponse(Json::Value());
  resp->setStatusCode(k200OK);
  callback(resp);
}

void AnnouncementAPIManager::create_announcement_answer(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string announcement_id) {
  // Get the JSON from the request
  auto json = req->getJsonObject();

  // Check if the JSON is valid
  if (!json) {
    send_response(std::move(callback), k400BadRequest, "Invalid JSON");
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

  // Check if the JSON has the required fields
  if ((*json.get())["content"].empty() ||
      !(*json.get())["content"].isString()) {
    send_response(std::move(callback), k400BadRequest,
                  "Missing required fields");
    return;
  }

  std::string content = (*json.get())["content"].asString();

  CassUuid announcement_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(announcement_id.c_str(), &announcement_uuid)) {
    send_response(std::move(callback), k400BadRequest,
                  "Invalid announcement id");
    return;
  }

  auto [status_code_response, json_response] =
      _announcement_manager->create_answer(school_id, token, announcement_uuid,
                                           content);

  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void AnnouncementAPIManager::delete_announcement_answer(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string announcement_id, std::string answer_id) {
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

  CassUuid announcement_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(announcement_id.c_str(), &announcement_uuid)) {
    send_response(std::move(callback), k400BadRequest,
                  "Invalid announcement id");
    return;
  }

  CassUuid answer_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(answer_id.c_str(), &answer_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid answer id");
    return;
  }

  auto [status_code_response, json_response] =
      _announcement_manager->delete_answer(school_id, token, announcement_uuid,
                                           answer_uuid);

  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}
