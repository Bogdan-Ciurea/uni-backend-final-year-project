#include "api_managers/course_api_manager.hpp"

void CourseAPIManager::send_response(
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

std::pair<int, std::string> CourseAPIManager::get_credentials(
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

bool CourseAPIManager::is_file_name_valid(const std::string file_name) {
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

void CourseAPIManager::create_course(
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
    send_response(std::move(callback), k400BadRequest, "Invalid JSON");
    return;
  }

  // Check if the JSON has the required fields
  if ((*json.get())["name"].empty() || !(*json.get())["name"].isString() ||
      (*json.get())["start_date"].empty() ||
      !(*json.get())["start_date"].isInt64() ||
      (*json.get())["end_date"].empty() ||
      !(*json.get())["end_date"].isInt64()) {
    send_response(std::move(callback), k400BadRequest,
                  "Missing required fields");
    return;
  }

  // Get the fields from the JSON
  const std::string name = (*json.get())["name"].asString();
  const time_t start_date = (*json.get())["start_date"].asInt64();
  const time_t end_date = (*json.get())["end_date"].asInt64();

  auto [status_code_response, json_response] = _course_manager->create_course(
      school_id, token, name, start_date, end_date);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void CourseAPIManager::get_course(
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

  CassUuid course_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(course_id.c_str(), &course_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid course id");
    return;
  }

  auto [status_code_response, json_response] =
      _course_manager->get_course(school_id, token, course_uuid);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void CourseAPIManager::get_courses_users(
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

  CassUuid course_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(course_id.c_str(), &course_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid course id");
    return;
  }

  auto [status_code_response, json_response] =
      _course_manager->get_courses_users(school_id, course_uuid, token);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void CourseAPIManager::get_user_courses(
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
      _course_manager->get_all_user_courses(school_id, token);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void CourseAPIManager::update_course(
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

  // Get the JSON from the request
  auto json = req->getJsonObject();

  // Check if the JSON is valid
  if (!json) {
    send_response(std::move(callback), k400BadRequest, "Invalid JSON");
    return;
  }

  // Check if the JSON has the required fields
  bool has_required_fields = false;
  if (!(*json.get())["name"].empty() && (*json.get())["name"].isString()) {
    has_required_fields = true;
  }
  if (!(*json.get())["start_date"].empty() &&
      (*json.get())["start_date"].isInt64()) {
    has_required_fields = true;
  }
  if (!(*json.get())["end_date"].empty() &&
      (*json.get())["end_date"].isInt64()) {
    has_required_fields = true;
  }

  if (!has_required_fields) {
    send_response(std::move(callback), k400BadRequest,
                  "Missing required fields");
    return;
  }

  // Get the fields from the JSON
  const std::optional<std::string> title =
      (!(*json.get())["name"].empty() && (*json.get())["name"].isString())
          ? std::optional<std::string>((*json.get())["name"].asString())
          : std::nullopt;
  const std::optional<time_t> start_date =
      (!(*json.get())["start_date"].empty() &&
       (*json.get())["start_date"].isInt64())
          ? std::optional<time_t>((*json.get())["start_date"].asInt64())
          : std::nullopt;
  const std::optional<time_t> end_date =
      (!(*json.get())["end_date"].empty() &&
       (*json.get())["end_date"].isInt64())
          ? std::optional<time_t>((*json.get())["end_date"].asInt64())
          : std::nullopt;

  CassUuid course_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(course_id.c_str(), &course_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid course id");
    return;
  }

  auto [status_code_response, json_response] = _course_manager->update_course(
      school_id, token, course_uuid, title, start_date, end_date);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void CourseAPIManager::delete_course(
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

  CassUuid course_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(course_id.c_str(), &course_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid course id");
    return;
  }

  auto [status_code_response, json_response] =
      _course_manager->delete_course(school_id, token, course_uuid);

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

void CourseAPIManager::create_course_thumbnail(
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

  CassUuid course_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(course_id.c_str(), &course_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid course id");
    return;
  }

  // Get the file from the request
  MultiPartParser fileUpload;
  if (fileUpload.parse(req) != 0 || fileUpload.getFiles().size() > 1) {
    send_response(std::move(callback), k403Forbidden, "Must be one file!");
    return;
  }

  auto &file = fileUpload.getFiles()[0];

  if (!is_file_name_valid(file.getFileName())) {
    send_response(std::move(callback), k400BadRequest, "Invalid file name!");
    return;
  }

  // Get the file extension
  std::string file_extension =
      file.getFileName().substr(file.getFileName().find_last_of("."));

  // Check the file extension
  if (file_extension != ".png" && file_extension != ".jpg" &&
      file_extension != ".jpeg") {
    send_response(std::move(callback), k400BadRequest,
                  "Invalid file extension!");
    return;
  }

  auto [status_code_response, json_response] =
      _course_manager->set_course_thumbnail(school_id, token, course_uuid,
                                            file_extension);

  if (status_code_response == k200OK) {
    std::string file_path = json_response["path"].asString();
    file.saveAs(file_path);
    json_response = Json::Value();
  }

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void CourseAPIManager::get_course_thumbnail(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string course_id, std::string user_token) {
  // Check if the header has the Authorization field
  // It should be of the format 'Bearer <token>'

  auto [school_id, token] = get_credentials(user_token);

  // Check if the headers have the required fields
  if (school_id <= 0 || token.empty()) {
    send_response(std::move(callback), k400BadRequest,
                  "Invalid required fields");
    return;
  }

  CassUuid course_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(course_id.c_str(), &course_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid course id");
    return;
  }

  auto [status_code_response, json_response] =
      _course_manager->get_course_thumbnail(school_id, token, course_uuid);

  if (status_code_response == k200OK) {
    std::string file_name = json_response["path"].asString();

    if (!std::filesystem::exists(file_name)) {
      send_response(std::move(callback), k500InternalServerError,
                    "File not found");
      return;
    }

    std::string file_extension = file_name.substr(file_name.find_last_of("."));
    std::string mime_type = _mime_types.at(file_extension);

    auto resp =
        HttpResponse::newFileResponse(file_name, "", CT_CUSTOM, mime_type);
    resp->setStatusCode(k200OK);
    callback(resp);
    return;
  }

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void CourseAPIManager::delete_course_thumbnail(
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

  CassUuid course_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(course_id.c_str(), &course_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid course id");
    return;
  }

  auto [status_code_response, json_response] =
      _course_manager->delete_course_thumbnail(school_id, token, course_uuid);

  if (status_code_response == k200OK) {
    std::string file_path = json_response["path"].asString();

    if (std::filesystem::exists(file_path)) {
      std::filesystem::remove(file_path);
    } else {
      send_response(std::move(callback), k500InternalServerError,
                    "File not found");
      return;
    }
    json_response = Json::Value();
  }

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void CourseAPIManager::create_course_file(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string course_id) {
  // Get the headers from the request
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

  std::string header_file_name = req->getHeader("file_name");
  std::string header_file_owner = req->getHeader("file_owner");
  std::string header_students_can_add = req->getHeader("students_can_add");
  std::string header_visible_to_students =
      req->getHeader("visible_to_students");
  std::string header_file_type = req->getHeader("file_type");

  // Check if the request has the required fields in the headers
  if (header_file_name.empty() || header_students_can_add.empty() ||
      header_visible_to_students.empty() || header_file_type.empty()) {
    send_response(std::move(callback), k400BadRequest,
                  "Missing required headers");
    return;
  }

  // Check if the headers have the correct information
  if ((header_students_can_add != "true" &&
       header_students_can_add != "false") ||
      (header_visible_to_students != "true" &&
       header_visible_to_students != "false") ||
      (header_file_type != "FILE" && header_file_type != "FOLDER")) {
    send_response(std::move(callback), k400BadRequest,
                  "Invalid header information");
    return;
  }

  // Get the fields from the headers
  std::string file_name = header_file_name;
  CustomFileType file_type = header_file_type.compare("FILE") == 0
                                 ? CustomFileType::FILE
                                 : CustomFileType::FOLDER;
  bool students_can_add =
      header_students_can_add.compare("true") == 0 ? true : false;
  bool visible_to_students =
      header_visible_to_students.compare("true") == 0 ? true : false;
  std::optional<CassUuid> file_owner;

  CassUuid course_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(course_id.c_str(), &course_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid course id");
    return;
  }
  if (!header_file_owner.empty()) {
    CassUuid file_owner_uuid;
    if (CASS_ERROR_LIB_BAD_PARAMS ==
        cass_uuid_from_string(header_file_owner.c_str(), &file_owner_uuid)) {
      send_response(std::move(callback), k400BadRequest,
                    "Invalid file owner id");
      return;
    }
    file_owner = file_owner_uuid;
  } else {
    file_owner = std::nullopt;
  }

  switch (file_type) {
    case CustomFileType::FILE: {
      // Get the file from the request
      MultiPartParser fileUpload;
      if (fileUpload.parse(req) != 0 || fileUpload.getFiles().size() > 1) {
        send_response(std::move(callback), k403Forbidden, "Must be one file!");
        return;
      }
      if (fileUpload.getFiles().size() == 0) {
        file_type = CustomFileType::FOLDER;
      }

      auto &file = fileUpload.getFiles()[0];

      if (!is_file_name_valid(file.getFileName())) {
        send_response(std::move(callback), k400BadRequest,
                      "Invalid file name!");
        return;
      }

      // Get the file extension
      std::string file_extension =
          file.getFileName().substr(file.getFileName().find_last_of("."));

      auto [status_code_response, json_response] =
          _course_manager->create_course_file(
              school_id, token, course_uuid, header_file_name, file_type,
              file_extension, file_owner, visible_to_students,
              students_can_add);

      // If the file was created, save the file
      if (status_code_response == k201Created) {
        // Get the file id
        std::string file_id = json_response["id"].asString();

        if (file_owner.has_value()) {
          file.saveAs(app().getUploadPath() + "/schools/" +
                      std::to_string(school_id) + "/courses/" + course_id +
                      "/files/" + header_file_owner + "/" + file_id +
                      file_extension);
        } else {
          file.saveAs(app().getUploadPath() + "/schools/" +
                      std::to_string(school_id) + "/courses/" + course_id +
                      "/files/" + file_id + file_extension);
        }
      }

      // Send the response
      auto resp = HttpResponse::newHttpJsonResponse(json_response);
      resp->setStatusCode(status_code_response);
      callback(resp);

      return;
    }

    case CustomFileType::FOLDER: {
      auto [status_code_response, json_response] =
          _course_manager->create_course_file(
              school_id, token, course_uuid, header_file_name, file_type, "",
              file_owner, visible_to_students, students_can_add);

      // Send the response
      auto resp = HttpResponse::newHttpJsonResponse(json_response);
      resp->setStatusCode(status_code_response);
      callback(resp);
      return;
    }
  }

  send_response(std::move(callback), k500InternalServerError,
                "Something went wrong!");
}

void CourseAPIManager::get_course_files(
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

  CassUuid course_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(course_id.c_str(), &course_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid course id");
    return;
  }

  auto [status_code_response, json_response] =
      _course_manager->get_course_files(school_id, course_uuid, token);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void CourseAPIManager::get_course_file(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string course_id, std::string file_id) {
  if (req->parameters().size() != 1 &&
      req->parameters().find("file_id") == req->parameters().end()) {
    get_course_files(req, std::move(callback), course_id);
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

  CassUuid course_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(course_id.c_str(), &course_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid course id");
    return;
  }

  CassUuid file_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(file_id.c_str(), &file_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid file id");
    return;
  }

  auto [status_code_response, json_response] =
      _course_manager->has_permission_to_get_file(school_id, token, course_uuid,
                                                  file_uuid);

  if (status_code_response != k200OK) {
    auto resp = HttpResponse::newHttpJsonResponse(json_response);
    resp->setStatusCode(status_code_response);
    callback(resp);
    return;
  }

  std::string file_name = json_response["file_path"].asString();

  if (file_name.empty()) {
    send_response(std::move(callback), k500InternalServerError,
                  "file not found!");
    return;
  }

  std::string file_extension = file_name.substr(file_name.find_last_of("."));
  std::string mime_type = _mime_types.at(file_extension);

  auto resp =
      HttpResponse::newFileResponse(file_name, "", CT_CUSTOM, mime_type);
  resp->setStatusCode(k200OK);
  callback(resp);
}

void CourseAPIManager::update_course_file(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string course_id, std::string file_id) {
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

  std::optional<std::string> file_name = nullopt;
  std::optional<std::vector<CassUuid>> file_ids = nullopt;
  std::optional<bool> visible_to_students = nullopt;
  std::optional<bool> students_can_add = nullopt;

  if (!(*json.get())["file_name"].empty() &&
      (*json.get())["file_name"].isString()) {
    file_name = (*json.get())["file_name"].asString();
  }

  if (!(*json.get())["file_ids"].empty() &&
      (*json.get())["file_ids"].isArray()) {
    file_ids = std::vector<CassUuid>();
    for (auto file_id : (*json.get())["file_ids"]) {
      CassUuid file_uuid;
      if (CASS_ERROR_LIB_BAD_PARAMS ==
          cass_uuid_from_string(file_id.asString().c_str(), &file_uuid)) {
        send_response(std::move(callback), k400BadRequest, "Invalid file id");
        return;
      }
      file_ids->push_back(file_uuid);
    }
  }

  if (!(*json.get())["visible_to_students"].empty() &&
      (*json.get())["visible_to_students"].isBool()) {
    visible_to_students = (*json.get())["visible_to_students"].asBool();
  }

  if (!(*json.get())["students_can_add"].empty() &&
      (*json.get())["students_can_add"].isBool()) {
    students_can_add = (*json.get())["students_can_add"].asBool();
  }

  CassUuid course_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(course_id.c_str(), &course_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid course id");
    return;
  }

  CassUuid file_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(file_id.c_str(), &file_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid file id");
    return;
  }

  auto [status_code_response, json_response] =
      _course_manager->update_course_files(
          school_id, token, course_uuid, file_uuid, file_name, file_ids,
          visible_to_students, students_can_add);

  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void CourseAPIManager::delete_course_file(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string course_id, std::string file_id) {
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

  // Check if the file owner id is in the JSON
  CassUuid course_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(course_id.c_str(), &course_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid course id");
    return;
  }

  // Check if the file owner id is in the JSON
  CassUuid file_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(file_id.c_str(), &file_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid file id");
    return;
  }

  auto [status_code_response, json_response] =
      _course_manager->delete_course_file(school_id, token, course_uuid,
                                          file_uuid);

  if (status_code_response == k200OK) {
    // Delete the file from the path
    std::string file_path = json_response["file_path"].asString();

    if (std::filesystem::exists(file_path)) {
      std::filesystem::remove_all(file_path);
    }

    json_response.clear();
  }

  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void CourseAPIManager::add_users_to_course(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string course_id) {
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
  if ((!(*json.get())["users_id"].empty() &&
       !(*json.get())["users_id"].isArray()) ||
      (!(*json.get())["tags_id"].empty() &&
       !(*json.get())["tags_id"].isArray())) {
    send_response(std::move(callback), k400BadRequest,
                  "Missing required fields");
    return;
  }

  // Get the fields from the JSON
  std::vector<CassUuid> users_id;
  std::vector<CassUuid> tags_id;

  if (!(*json.get())["tags_id"].empty()) {
    // For every uuid in the tags array we add it to the tags_id vector
    for (auto &tag : (*json.get())["tags_id"]) {
      CassUuid tag_uuid;
      if (CASS_ERROR_LIB_BAD_PARAMS ==
          cass_uuid_from_string(tag.asString().c_str(), &tag_uuid)) {
        send_response(std::move(callback), k400BadRequest, "Invalid tag id");
        return;
      }
      tags_id.push_back(tag_uuid);
    }
  }

  if (!(*json.get())["users_id"].empty()) {
    // For every uuid in the users array we add it to the users_id vector
    for (auto &user : (*json.get())["users_id"]) {
      CassUuid user_uuid;
      if (CASS_ERROR_LIB_BAD_PARAMS ==
          cass_uuid_from_string(user.asString().c_str(), &user_uuid)) {
        send_response(std::move(callback), k400BadRequest, "Invalid user id");
        return;
      }
      users_id.push_back(user_uuid);
    }
  }

  if (users_id.empty() && tags_id.empty()) {
    send_response(std::move(callback), k400BadRequest,
                  "Missing required fields");
    return;
  }

  CassUuid course_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(course_id.c_str(), &course_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid course id");
    return;
  }

  auto [status_code_response, json_response] = _course_manager->add_users(
      school_id, token, course_uuid, users_id, tags_id);

  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void CourseAPIManager::remove_users_from_course(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string course_id) {
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
  if ((!(*json.get())["users_id"].empty() &&
       !(*json.get())["users_id"].isArray()) ||
      (!(*json.get())["tags_id"].empty() &&
       !(*json.get())["tags_id"].isArray())) {
    send_response(std::move(callback), k400BadRequest,
                  "Missing required fields");
    return;
  }

  // Get the fields from the JSON
  std::vector<CassUuid> users_id;
  std::vector<CassUuid> tags_id;

  if (!(*json.get())["tags_id"].empty()) {
    // For every uuid in the tags array we add it to the tags_id vector
    for (auto &tag : (*json.get())["tags_id"]) {
      CassUuid tag_uuid;
      if (CASS_ERROR_LIB_BAD_PARAMS ==
          cass_uuid_from_string(tag.asString().c_str(), &tag_uuid)) {
        send_response(std::move(callback), k400BadRequest, "Invalid tag id");
        return;
      }
      tags_id.push_back(tag_uuid);
    }
  }

  if (!(*json.get())["users_id"].empty()) {
    // For every uuid in the users array we add it to the users_id vector
    for (auto &user : (*json.get())["users_id"]) {
      CassUuid user_uuid;
      if (CASS_ERROR_LIB_BAD_PARAMS ==
          cass_uuid_from_string(user.asString().c_str(), &user_uuid)) {
        send_response(std::move(callback), k400BadRequest, "Invalid user id");
        return;
      }
      users_id.push_back(user_uuid);
    }
  }

  if (users_id.empty() && tags_id.empty()) {
    send_response(std::move(callback), k400BadRequest,
                  "Missing required fields");
    return;
  }

  CassUuid course_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(course_id.c_str(), &course_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid course id");
    return;
  }

  auto [status_code_response, json_response] = _course_manager->remove_users(
      school_id, token, course_uuid, users_id, tags_id);

  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void CourseAPIManager::create_course_question(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string course_id) {
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
  if ((*json.get())["text"].empty() || !(*json.get())["text"].isString()) {
    send_response(std::move(callback), k400BadRequest,
                  "Missing required fields");
    return;
  }

  // Get the fields from the JSON
  std::string text = (*json.get())["text"].asString();

  CassUuid course_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(course_id.c_str(), &course_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid course id");
    return;
  }

  auto [status_code_response, json_response] =
      _course_manager->create_question(school_id, token, course_uuid, text);

  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void CourseAPIManager::get_course_questions(
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

  CassUuid course_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(course_id.c_str(), &course_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid course id");
    return;
  }

  auto [status_code_response, json_response] =
      _course_manager->get_questions_by_course(school_id, token, course_uuid);

  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void CourseAPIManager::delete_course_question(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string course_id, std::string question_id) {
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

  CassUuid question_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(question_id.c_str(), &question_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid question id");
    return;
  }

  CassUuid course_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(course_id.c_str(), &course_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid course id");
    return;
  }

  auto [status_code_response, json_response] = _course_manager->delete_question(
      school_id, token, course_uuid, question_uuid);

  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void CourseAPIManager::create_course_answer(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string course_id, std::string question_id) {
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
  if ((*json.get())["text"].empty() || !(*json.get())["text"].isString()) {
    send_response(std::move(callback), k400BadRequest,
                  "Missing required fields");
    return;
  }

  // Get the fields from the JSON
  std::string text = (*json.get())["text"].asString();

  CassUuid question_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(question_id.c_str(), &question_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid question id");
    return;
  }

  CassUuid course_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(course_id.c_str(), &course_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid course id");
    return;
  }

  auto [status_code_response, json_response] = _course_manager->create_answer(
      school_id, token, course_uuid, question_uuid, text);

  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void CourseAPIManager::delete_course_answer(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string course_id, std::string question_id, std::string answer_id) {
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

  CassUuid answer_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(answer_id.c_str(), &answer_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid answer id");
    return;
  }

  CassUuid question_uuid;
  if (CASS_ERROR_LIB_BAD_PARAMS ==
      cass_uuid_from_string(question_id.c_str(), &question_uuid)) {
    send_response(std::move(callback), k400BadRequest, "Invalid question id");
    return;
  }

  auto [status_code_response, json_response] = _course_manager->delete_answer(
      school_id, token, question_uuid, answer_uuid);

  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}
