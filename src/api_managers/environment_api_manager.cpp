#include "api_managers/environment_api_manager.hpp"

void EnvironmentAPIManager::send_response(
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

void EnvironmentAPIManager::create_school(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  // Get the JSON object from the request
  auto json = req->jsonObject();

  // Check if the JSON object is valid
  if (!json) {
    send_response(std::move(callback), k400BadRequest, "Invalid JSON");
    return;
  }

  // Check if the JSON object has the required fields and they are of the
  // correct type
  if ((*json.get())["name"].empty() || (*json.get())["country_id"].empty() ||
      !(*json.get())["name"].isString() ||
      !(*json.get())["country_id"].isInt()) {
    send_response(std::move(callback), k400BadRequest, "Invalid JSON");
    return;
  }

  // Get the values from the JSON object
  std::string school_name = (*json.get())["name"].asString();
  int school_country = (*json.get())["country_id"].asInt();
  std::string school_image_path = (*json.get())["image_path"].asString();

  auto [status_code_response, json_response] =
      _environment_manager->create_school(school_name, school_country,
                                          school_image_path);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void EnvironmentAPIManager::get_school(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback, int school_id) {
  // Check if the request has any parameters
  auto result = req.get()->parameters().find("id");
  // If the request has no parameters, get all schools
  if (req.get()->parameters().empty()) {
    get_all_schools(req, std::move(callback));
    return;
  }

  // If the parameter is not "id" or the request has
  // more than one parameter, return an error
  if (result == req.get()->parameters().end() ||
      req.get()->parameters().size() > 1) {
    send_response(std::move(callback), k400BadRequest,
                  "Invalid parameters passed");
    return;
  }

  // Get the school from the database
  auto [status_code_response, json_response] =
      _environment_manager->get_school(school_id);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void EnvironmentAPIManager::get_all_schools(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  // Get all schools from the database
  auto [status_code_response, json_response] =
      _environment_manager->get_all_schools();

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void EnvironmentAPIManager::update_school(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback, int school_id) {
  // Check if the request has any parameters.
  auto result = req.get()->parameters().find("id");
  // If the parameter is not "id" or the request has
  // more than one parameter, return an error.
  if (result == req.get()->parameters().end() ||
      req.get()->parameters().size() != 1) {
    send_response(std::move(callback), k400BadRequest,
                  "Invalid parameters passed");
    return;
  }

  // Check if the request has a JSON body
  auto json = req->jsonObject();

  // If the request has no JSON body, return an error
  if (!json) {
    send_response(std::move(callback), k400BadRequest, "Invalid JSON");
    return;
  }

  // Check if the JSON body has the required fields and types
  if ((*json.get())["name"].empty() || (*json.get())["country_id"].empty() ||
      !(*json.get())["name"].isString() ||
      !(*json.get())["country_id"].isInt()) {
    send_response(std::move(callback), k400BadRequest, "Invalid JSON");
    return;
  }

  // Get the school name and country from the JSON body
  std::string school_name = (*json.get())["name"].asString();
  int school_country = (*json.get())["country_id"].asInt();
  std::string school_image_path = (*json.get())["image_path"].asString();

  // Update the school in the database
  auto [status_code_response, json_response] =
      _environment_manager->update_school(school_id, school_name,
                                          school_country, school_image_path);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void EnvironmentAPIManager::delete_school(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback, int school_id) {
  // Check if the request has any parameters.
  auto result = req.get()->parameters().find("id");
  // If the parameter is not "id" or the request has
  // more than one parameter, return an error.
  if (result == req.get()->parameters().end() ||
      req.get()->parameters().size() != 1) {
    send_response(std::move(callback), k400BadRequest,
                  "Invalid parameters passed");
    return;
  }

  // Delete the school from the database
  auto [status_code_response, json_response] =
      _environment_manager->delete_school(school_id);

  // Send the response
  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void EnvironmentAPIManager::create_country(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  // Get the JSON body
  auto json = req->getJsonObject();

  // Check if the JSON body is valid
  if (!json) {
    send_response(std::move(callback), k400BadRequest, "Invalid JSON");
    return;
  }

  // Check if the JSON body has the required field "name"
  if ((*json.get())["name"].empty() || (*json.get())["code"].empty() ||
      !(*json.get())["name"].isString() || !(*json.get())["code"].isString()) {
    send_response(std::move(callback), k400BadRequest,
                  "Missing required field");
    return;
  }

  std::string country_name = (*json.get())["name"].asString();
  std::string country_code = (*json.get())["code"].asString();

  // Add the country to the database
  auto [status_code_response, json_response] =
      _environment_manager->create_country(country_name, country_code);

  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void EnvironmentAPIManager::get_country(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback, int country_id) {
  // Check if the request has any parameters
  auto result = req.get()->parameters().find("id");
  // If the request has no parameters, get all schools
  if (req.get()->parameters().empty()) {
    get_all_countries(req, std::move(callback));
    return;
  }

  // If the parameter is not "id" or the request has
  // more than one parameter, return an error
  if (result == req.get()->parameters().end() ||
      req.get()->parameters().size() > 1) {
    send_response(std::move(callback), k400BadRequest,
                  "Invalid parameters passed");
    return;
  }

  // Get the specific country from the database
  auto [status_code_response, json_response] =
      _environment_manager->get_country(country_id);

  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void EnvironmentAPIManager::get_all_countries(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  // Get all countries from the database
  auto [status_code_response, json_response] =
      _environment_manager->get_all_countries();

  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void EnvironmentAPIManager::update_country(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback, int country_id) {
  // Check if the request has any parameters.
  auto result = req.get()->parameters().find("id");
  // If the parameter is not "id" or the request has
  // more than one parameter, return an error.
  if (result == req.get()->parameters().end() ||
      req.get()->parameters().size() != 1) {
    send_response(std::move(callback), k400BadRequest,
                  "Invalid parameters passed");
    return;
  }

  // Get the JSON body
  auto json = req->getJsonObject();

  // Check if the JSON body is valid
  if (!json) {
    send_response(std::move(callback), k400BadRequest, "Invalid JSON");
    return;
  }

  // Check if the JSON body has the required field "name"
  if ((*json.get())["name"].empty() || (*json.get())["code"].empty() ||
      !(*json.get())["name"].isString() || !(*json.get())["code"].isString()) {
    send_response(std::move(callback), k400BadRequest,
                  "Missing required field");
    return;
  }

  std::string country_name = (*json.get())["name"].asString();
  std::string country_code = (*json.get())["code"].asString();

  CountryObject country_object(country_id, country_name, country_code);

  // Update the country in the database
  auto [status_code_response, json_response] =
      _environment_manager->update_country(country_id, country_name,
                                           country_code);

  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void EnvironmentAPIManager::delete_country(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback, int country_id) {
  // Check if the request has any parameters.
  auto result = req.get()->parameters().find("id");
  // If the parameter is not "id" or the request has
  // more than one parameter, return an error.
  if (result == req.get()->parameters().end() ||
      req.get()->parameters().size() != 1) {
    send_response(std::move(callback), k400BadRequest,
                  "Invalid parameters passed");
    return;
  }

  // Delete the country from the database
  auto [status_code_response, json_response] =
      _environment_manager->delete_country(country_id);

  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void EnvironmentAPIManager::create_holiday(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback, int school_id) {
  // Check if the parameter is valid. It should be "id"
  if (req->parameters().size() != 1 ||
      req->parameters().find("school_id") == req->parameters().end()) {
    send_response(std::move(callback), k400BadRequest,
                  "Invalid parameters passed");
    return;
  }

  // Get the JSON body
  auto json = req->getJsonObject();

  // Check if the JSON body is valid
  if (!json) {
    send_response(std::move(callback), k400BadRequest, "Invalid JSON");
    return;
  }

  // Check if the JSON body has the required fields
  if ((*json.get())["date"].empty() || !(*json.get())["date"].isInt64()) {
    send_response(std::move(callback), k400BadRequest,
                  "Missing required field");
    return;
  }

  int date = (*json.get())["date"].asInt64();
  std::string name = (*json.get())["name"].asString();

  auto [status_code_response, json_response] =
      _environment_manager->create_holiday(school_id, HolidayType::CUSTOM, date,
                                           name);

  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void EnvironmentAPIManager::get_holidays(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback, int school_id) {
  // Check if the parameter is valid. It should be "id"
  if (req->parameters().size() != 1 ||
      req->parameters().find("school_id") == req->parameters().end()) {
    send_response(std::move(callback), k400BadRequest,
                  "Invalid parameters passed");
    return;
  }

  // Get the holidays from the database
  auto [status_code_response, json_response] =
      _environment_manager->get_holidays(school_id, HolidayType::CUSTOM);

  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}

void EnvironmentAPIManager::delete_holiday(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback, int school_id,
    time_t date) {
  // Check if the parameter is valid. It should be "id"
  if (req->parameters().size() != 2 ||
      req->parameters().find("school_id") == req->parameters().end() ||
      req->parameters().find("date") == req->parameters().end()) {
    send_response(std::move(callback), k400BadRequest,
                  "Invalid parameters passed");
    return;
  }

  // Delete the holiday from the database
  auto [status_code_response, json_response] =
      _environment_manager->delete_holiday(school_id, HolidayType::CUSTOM,
                                           date);

  auto resp = HttpResponse::newHttpJsonResponse(json_response);
  resp->setStatusCode(status_code_response);
  callback(resp);
}
