#include "relations_managers/tag_manager.hpp"

std::pair<drogon::HttpStatusCode, Json::Value> TagManager::get_user_by_token(
    const int school_id, const std::string &user_token, UserObject &user) {
  Json::Value json_response;

  // Get user by token
  auto [cql_result0, user_id] =
      _tokens_cql_manager->get_user_from_token(school_id, user_token);

  if (cql_result0.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Invalid token";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  } else if (cql_result0.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the user from the token";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Get user from user id
  auto [cql_result1, temp_user] =
      _users_cql_manager->get_user(school_id, user_id);

  if (cql_result1.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Invalid user id";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  } else if (cql_result1.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the user from the user id";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  user = temp_user;

  return {drogon::HttpStatusCode::k200OK, json_response};
}

int TagManager::validate_tag(const std::string &value,
                             const std::string &colour) {
  if (value.empty() || value.size() > 50) {
    return 1;
  }
  bool is_allowed_colour = false;
  for (const auto &allowed_colour : allowed_colours) {
    if (allowed_colour == colour) {
      is_allowed_colour = true;
      break;
    }
  }
  if (!is_allowed_colour) {
    return 2;
  }
  return 0;
}

std::pair<drogon::HttpStatusCode, Json::Value> TagManager::create_tag(
    const int school_id, const std::string &creator_token,
    const std::string &value, const std::string &colour) {
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, creator_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check to see if the user is admin or teacher
  if (temp_user._user_type != UserType::ADMIN &&
      temp_user._user_type != UserType::TEACHER) {
    return {drogon::HttpStatusCode::k403Forbidden,
            Json::Value("You are not allowed to create tags")};
  }

  // Check the value and colour
  int validation_result = validate_tag(value, colour);

  if (validation_result == 1) {
    json_response["error"] = "Invalid tag value";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  } else if (validation_result == 2) {
    json_response["error"] = "Invalid tag colour";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  }

  // Create the tag
  TagObject tag_object(school_id, create_current_uuid(), value, colour);

  // Write the tag to the database
  auto cql_result = _tags_cql_manager->create_tag(tag_object);

  if (cql_result.code() == ResultCode::NOT_APPLIED) {
    json_response["error"] = "Internal server error";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  } else if (cql_result.code() == ResultCode::NOT_APPLIED) {
    json_response["error"] = "Internal server error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  json_response = tag_object.to_json(true);
  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> TagManager::get_tag(
    const int school_id, const CassUuid tag_id, const std::string &user_token) {
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check to see if the user is admin or teacher
  if (temp_user._user_type != UserType::ADMIN &&
      temp_user._user_type != UserType::TEACHER) {
    return {drogon::HttpStatusCode::k403Forbidden,
            Json::Value("You are not allowed to get tags")};
  }

  // Get the tag
  auto [cql_result, tag_object] =
      _tags_cql_manager->get_tag_by_id(school_id, tag_id);

  if (cql_result.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Invalid tag id";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  } else if (cql_result.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the tag from the tag id";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Add the tag to the response
  json_response = tag_object.to_json(true);

  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> TagManager::get_all_tags(
    const int school_id, const std::string &user_token) {
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check to see if the user is admin or teacher
  if (temp_user._user_type != UserType::ADMIN &&
      temp_user._user_type != UserType::TEACHER) {
    return {drogon::HttpStatusCode::k403Forbidden,
            Json::Value("You are not allowed to get tags")};
  }

  // Get all the tags from the database
  auto [cql_result, tags] = _tags_cql_manager->get_tags_by_school_id(school_id);

  if (cql_result.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the tags from the school id";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Add the tags to the response
  for (auto &tag : tags) {
    json_response.append(tag.to_json(true));
  }

  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> TagManager::update_tag(
    const int school_id, const CassUuid tag_id, const std::string &user_token,
    const std::optional<std::string> &value,
    const std::optional<std::string> &colour) {
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check to see if the user is admin or teacher
  if (temp_user._user_type != UserType::ADMIN &&
      temp_user._user_type != UserType::TEACHER) {
    return {drogon::HttpStatusCode::k403Forbidden,
            Json::Value("You are not allowed to update tags")};
  }

  if (!value.has_value() && !colour.has_value()) {
    json_response["error"] = "No value or colour provided";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  }

  auto [cql_result0, tag_object] =
      _tags_cql_manager->get_tag_by_id(school_id, tag_id);

  if (cql_result0.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Invalid tag id";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  } else if (cql_result0.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the tag from the tag id";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Set the value and colour
  tag_object._name = value ? value.value() : tag_object._name;
  tag_object._colour = colour ? colour.value() : tag_object._colour;

  // Check the value and colour
  int validation_result = validate_tag(tag_object._name, tag_object._colour);

  if (validation_result == 1) {
    json_response["error"] = "Invalid tag value";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  } else if (validation_result == 2) {
    json_response["error"] = "Invalid tag colour";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  }

  // Update the tag
  auto cql_result1 = _tags_cql_manager->update_tag(
      school_id, tag_id, tag_object._name, tag_object._colour);

  if (cql_result1.code() == ResultCode::NOT_APPLIED) {
    json_response["error"] = "Invalid tag id";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  } else if (cql_result1.code() != ResultCode::OK) {
    json_response["error"] = "Could not update the tag";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Return 200 OK
  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> TagManager::delete_tag(
    const int school_id, const CassUuid tag_id, const std::string &user_token) {
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check to see if the user is admin or teacher
  if (temp_user._user_type != UserType::ADMIN &&
      temp_user._user_type != UserType::TEACHER) {
    return {drogon::HttpStatusCode::k403Forbidden,
            Json::Value("You are not allowed to create tags")};
  }

  // Check if the tag exists
  auto [cql_result, tag_object] =
      _tags_cql_manager->get_tag_by_id(school_id, tag_id);

  if (cql_result.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Invalid tag id";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  } else if (cql_result.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the tag from the tag id";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Get all the users that have this tag
  auto [cql_result0, users_id] =
      _users_by_tag_cql_manager->get_users_by_tag(school_id, tag_id);

  if (cql_result0.code() != ResultCode::OK &&
      cql_result0.code() != ResultCode::NOT_FOUND) {
    json_response["error"] = "Could not get the users by tag";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Delete all the relations between the users and the tag
  for (const auto &user_id : users_id) {
    auto cql_result1 = _tags_by_user_cql_manager->delete_relationship(
        school_id, user_id, tag_id);

    if (cql_result1.code() != ResultCode::OK) {
      json_response["error"] = "Could not delete the user by tag";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }
  }

  // Delete all the relations between the tag and the users
  auto cql_result2 =
      _users_by_tag_cql_manager->delete_relationships_by_tag(school_id, tag_id);

  if (cql_result2.code() != ResultCode::OK) {
    json_response["error"] = "Could not delete the users by tag";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Delete the tag
  auto cql_result3 = _tags_cql_manager->delete_tag(school_id, tag_id);

  if (cql_result3.code() != ResultCode::OK) {
    json_response["error"] = "Could not delete the tag";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Return 200 OK
  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
TagManager::create_tag_user_relation(const int school_id,
                                     const std::string &user_token,
                                     const CassUuid tag_id,
                                     const CassUuid user_id) {
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check to see if the user is admin or teacher
  if (temp_user._user_type != UserType::ADMIN &&
      temp_user._user_type != UserType::TEACHER) {
    return {drogon::HttpStatusCode::k403Forbidden,
            Json::Value("You are not allowed to create tags")};
  }

  // Check if the user exists
  auto [cql_result0, user_object] =
      _users_cql_manager->get_user(school_id, user_id);

  if (cql_result0.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Invalid user id";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  } else if (cql_result0.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the user from the user id";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the tag exists
  auto [cql_result1, tag_object] =
      _tags_cql_manager->get_tag_by_id(school_id, tag_id);

  if (cql_result1.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Invalid tag id";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  } else if (cql_result1.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the tag from the tag id";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Add the relation
  auto cql_result2 = _users_by_tag_cql_manager->create_relationship(
      school_id, tag_id, user_id);

  if (cql_result2.code() == ResultCode::NOT_APPLIED) {
    json_response["error"] = "The user already has this tag";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  } else if (cql_result2.code() != ResultCode::OK) {
    json_response["error"] = "Could not add the user by tag";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Add the relation
  auto cql_result3 = _tags_by_user_cql_manager->create_relationship(
      school_id, user_id, tag_id);

  if (cql_result3.code() == ResultCode::NOT_APPLIED) {
    json_response["error"] = "The user already has this tag";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  } else if (cql_result3.code() != ResultCode::OK) {
    json_response["error"] = "Could not add the tag by user";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Return 200 OK
  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> TagManager::get_users_by_tag(
    const int school_id, const std::string &user_token, const CassUuid tag_id) {
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check to see if the user is admin or teacher
  if (temp_user._user_type != UserType::ADMIN &&
      temp_user._user_type != UserType::TEACHER) {
    return {drogon::HttpStatusCode::k403Forbidden,
            Json::Value("You are not allowed to create tags")};
  }

  // Check if the tag exists
  auto [cql_result0, tag_object] =
      _tags_cql_manager->get_tag_by_id(school_id, tag_id);

  if (cql_result0.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Invalid tag id";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  } else if (cql_result0.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the tag from the tag id";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Get all the users that have this tag
  auto [cql_result1, users_id] =
      _users_by_tag_cql_manager->get_users_by_tag(school_id, tag_id);

  if (cql_result1.code() == ResultCode::NOT_FOUND) {
    json_response = Json::Value(Json::arrayValue);
    return {drogon::HttpStatusCode::k200OK, json_response};
  } else if (cql_result1.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the users by tag";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Get all the users
  std::vector<UserObject> users;

  for (const auto &user_id : users_id) {
    auto [cql_result2, user] = _users_cql_manager->get_user(school_id, user_id);

    if (cql_result2.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the user from the user id";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    users.push_back(user);
  }

  // Create the json response
  for (const auto &user : users) {
    Json::Value user_json;
    user_json["user_id"] = get_uuid_string(user._user_id);
    user_json["first_name"] = user._first_name;
    user_json["last_name"] = user._last_name;
    json_response.append(user_json);
  }

  // Return 200 OK
  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> TagManager::get_tags_by_user(
    const int school_id, const std::string &user_token,
    const std::optional<CassUuid> user_id) {
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // If there is no user id, get the user id from the token
  if (!user_id.has_value()) {
    auto [cql_result1, tags_ids] = _tags_by_user_cql_manager->get_tags_by_user(
        school_id, temp_user._user_id);

    if (cql_result1.code() == ResultCode::NOT_FOUND) {
      json_response = Json::Value(Json::arrayValue);
      return {drogon::HttpStatusCode::k200OK, json_response};
    } else if (cql_result1.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the tags by user";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    // Get all the tags
    std::vector<TagObject> tags;
    json_response = Json::Value(Json::arrayValue);

    for (const auto &tag_id : tags_ids) {
      auto [cql_result2, tag] =
          _tags_cql_manager->get_tag_by_id(school_id, tag_id);

      if (cql_result2.code() != ResultCode::OK) {
        Json::Value json_response2;
        json_response2["error"] = "Could not get the tag from the tag id";
        return {drogon::HttpStatusCode::k500InternalServerError,
                json_response2};
      }

      tags.push_back(tag);
    }

    // Create the json response
    for (const auto &tag : tags) {
      Json::Value tag_json;
      tag_json["tag_name"] = tag._name;
      tag_json["colour"] = tag._colour;
      json_response.append(tag_json);
    }
  } else {
    // Check if the user is a teacher or admin
    if (temp_user._user_type != UserType::ADMIN &&
        temp_user._user_type != UserType::TEACHER) {
      return {drogon::HttpStatusCode::k403Forbidden,
              Json::Value("You are not allowed to get the tags by user")};
    }

    // Check if the user exists
    auto [cql_result3, user] =
        _users_cql_manager->get_user(school_id, user_id.value());

    if (cql_result3.code() == ResultCode::NOT_FOUND) {
      json_response["error"] = "Invalid user id";
      return {drogon::HttpStatusCode::k400BadRequest, json_response};
    } else if (cql_result3.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the user from the user id";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    auto [cql_result4, tags_ids] =
        _tags_by_user_cql_manager->get_tags_by_user(school_id, user._user_id);

    if (cql_result4.code() != ResultCode::OK &&
        cql_result4.code() != ResultCode::NOT_FOUND) {
      json_response["error"] = "Could not get the tags by user";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    // Get all the tags
    std::vector<TagObject> tags;
    json_response = Json::Value(Json::arrayValue);

    for (const auto &tag_id : tags_ids) {
      auto [cql_result5, tag] =
          _tags_cql_manager->get_tag_by_id(school_id, tag_id);

      if (cql_result5.code() != ResultCode::OK) {
        Json::Value json_response2;
        json_response2["error"] = "Could not get the tag from the tag id";
        return {drogon::HttpStatusCode::k500InternalServerError,
                json_response2};
      }

      tags.push_back(tag);
    }

    // Create the json response
    for (const auto &tag : tags) {
      Json::Value tag_json;
      tag_json["name"] = tag._name;
      tag_json["colour"] = tag._colour;
      tag_json["id"] = get_uuid_string(tag._id);
      json_response.append(tag_json);
    }
  }

  // Return 200 OK
  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
TagManager::delete_tag_user_relation(const int school_id,
                                     const std::string &user_token,
                                     const CassUuid tag_id,
                                     const CassUuid user_id) {
  // Get the user from the token
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check to see if the user is admin or teacher
  if (temp_user._user_type != UserType::ADMIN &&
      temp_user._user_type != UserType::TEACHER) {
    return {drogon::HttpStatusCode::k403Forbidden,
            Json::Value("You are not allowed to create tags")};
  }

  // Delete from users_by_tag & tags_by_user tables
  auto cql_result0 = _users_by_tag_cql_manager->delete_relationship(
      school_id, tag_id, user_id);

  if (cql_result0.code() == ResultCode::NOT_APPLIED) {
    json_response["error"] = "No relation found";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  } else if (cql_result0.code() != ResultCode::OK) {
    json_response["error"] = "Could not delete the user by tag";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Delete from users_by_tag & tags_by_user tables
  auto cql_result1 = _tags_by_user_cql_manager->delete_relationship(
      school_id, user_id, tag_id);

  if (cql_result1.code() == ResultCode::NOT_APPLIED) {
    json_response["error"] = "No relation found";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  } else if (cql_result1.code() != ResultCode::OK) {
    json_response["error"] = "Could not delete the user by tag";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Return 200 OK
  return {drogon::HttpStatusCode::k200OK, json_response};
}
