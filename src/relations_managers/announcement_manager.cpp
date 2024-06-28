#include "relations_managers/announcement_manager.hpp"

std::pair<drogon::HttpStatusCode, Json::Value>
AnnouncementManager::get_user_by_token(const int school_id,
                                       const std::string &user_token,
                                       UserObject &user) {
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

std::pair<drogon::HttpStatusCode, Json::Value>
AnnouncementManager::create_announcement(const int school_id,
                                         const std::string &creator_token,
                                         const std::string &title,
                                         const std::string &content,
                                         const bool allow_answers) {
  // Get the user by token
  UserObject temp_user;

  auto [status_code, json_response] =
      get_user_by_token(school_id, creator_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check if the user is a teacher or an admin
  if (temp_user._user_type != UserType::TEACHER &&
      temp_user._user_type != UserType::ADMIN) {
    json_response["error"] = "You are not allowed to create an announcement";
    return {drogon::HttpStatusCode::k403Forbidden, json_response};
  }

  // Create the announcement
  CassUuid announcement_id = create_current_uuid();
  const time_t created_at = time(nullptr);
  AnnouncementObject announcement(school_id, announcement_id, created_at,
                                  temp_user._user_id, title, content,
                                  allow_answers, {});

  auto cql_result =
      _announcements_cql_manager->create_announcement(announcement);

  if (cql_result.code() != ResultCode::OK) {
    json_response["error"] = "Could not create the announcement";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  json_response["id"] = get_uuid_string(announcement_id);
  json_response["title"] = title;
  json_response["content"] = content;
  json_response["allow_answers"] = allow_answers;
  json_response["files"] = Json::arrayValue;
  json_response["created_at"] = (Json::Value::Int64)created_at;
  json_response["created_by_user_name"] =
      temp_user._first_name + " " + temp_user._last_name;
  json_response["created_by_user_id"] = get_uuid_string(temp_user._user_id);

  return {drogon::HttpStatusCode::k201Created, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
AnnouncementManager::get_announcements(const int school_id,
                                       const std::string &token) {
  // Get the user by token
  UserObject temp_user;

  auto [status_code, json_response] =
      get_user_by_token(school_id, token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Update the last time online variable
  CqlResult cql_result0 = _users_cql_manager->update_user(
      temp_user._school_id, temp_user._user_id, temp_user._email,
      temp_user._password, temp_user._user_type, temp_user._changed_password,
      temp_user._first_name, temp_user._last_name, temp_user._phone_number,
      time(nullptr));

  if (cql_result0.code() != ResultCode::OK) {
    json_response["error"] = "Error while updating the requester's status";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  std::vector<AnnouncementObject> announcements;

  if (temp_user._user_type != UserType::ADMIN) {
    // Get the tags of the user
    auto [cql_result1, tags_ids] = _tags_by_user_cql_manager->get_tags_by_user(
        school_id, temp_user._user_id);

    if (cql_result1.code() != ResultCode::OK &&
        cql_result1.code() != ResultCode::NOT_FOUND) {
      json_response["error"] = "Could not get the tags of the user";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    // Get the announcements by the tags
    std::vector<CassUuid> announcements_ids;
    for (const auto &tag_id : tags_ids) {
      auto [cql_result2, temp_announcements_ids] =
          _announcements_by_tag_cql_manager->get_announcements_by_tag(school_id,
                                                                      tag_id);

      if (cql_result2.code() != ResultCode::OK &&
          cql_result2.code() != ResultCode::NOT_FOUND) {
        json_response["error"] = "Could not get the announcements by the tags";
        return {drogon::HttpStatusCode::k500InternalServerError, json_response};
      }

      if (cql_result2.code() == ResultCode::NOT_FOUND) {
        continue;
      }

      announcements_ids.insert(announcements_ids.end(),
                               temp_announcements_ids.begin(),
                               temp_announcements_ids.end());
    }

    // Get the announcements
    for (const auto &announcement_id : announcements_ids) {
      auto [cql_result3, temp_announcement] =
          _announcements_cql_manager->get_announcement_by_id(school_id,
                                                             announcement_id);

      if (cql_result3.code() == ResultCode::NOT_FOUND) {
        continue;
      } else if (cql_result3.code() != ResultCode::OK) {
        json_response["error"] = "Could not get the announcements";
        return {drogon::HttpStatusCode::k500InternalServerError, json_response};
      }

      announcements.push_back(temp_announcement);
    }

  } else {
    // Get the announcements
    auto [cql_result3, temp_announcements] =
        _announcements_cql_manager->get_announcement_school_id(school_id);

    if (cql_result3.code() == ResultCode::NOT_FOUND) {
      json_response["error"] = "No announcements found";
      return {drogon::HttpStatusCode::k404NotFound, json_response};
    } else if (cql_result3.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the announcements";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    announcements = temp_announcements;
  }

  // Build the json response
  for (const auto &announcement : announcements) {
    Json::Value json_announcement;
    json_announcement["id"] = get_uuid_string(announcement._id);
    json_announcement["title"] = announcement._title;
    json_announcement["content"] = announcement._content;
    json_announcement["allow_answers"] = announcement._allow_answers;
    json_announcement["files"] = Json::arrayValue;
    for (const auto &file_id : announcement._files) {
      auto [response_code, file_json] = this->get_file_json(school_id, file_id);

      if (response_code != drogon::HttpStatusCode::k200OK) {
        return {response_code, file_json};
      }

      json_announcement["files"].append(file_json);
    }
    json_announcement["created_at"] =
        (Json::Value::Int64)announcement._created_at;
    json_announcement["created_by_user_id"] =
        get_uuid_string(announcement._created_by);

    // Get the user that created the announcement
    auto [cql_result4, ann_creator] =
        _users_cql_manager->get_user(school_id, announcement._created_by);

    if (cql_result4.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the announcement's creator";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    json_announcement["created_by_user_name"] =
        ann_creator._first_name + " " + ann_creator._last_name;

    // Get the announcement answers
    auto [http_response, json_answers] =
        get_answers(school_id, announcement._id);
    if (http_response != drogon::HttpStatusCode::k200OK) {
      return {http_response, json_answers};
    }

    json_announcement["answers"] = json_answers;

    json_response.append(json_announcement);
  }

  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
AnnouncementManager::delete_announcement(const int school_id,
                                         const std::string &token,
                                         const CassUuid &announcement_id) {
  // Get the user by token
  UserObject temp_user;

  auto [status_code, json_response] =
      get_user_by_token(school_id, token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Get the announcement
  auto [cql_result0, announcement] =
      _announcements_cql_manager->get_announcement_by_id(school_id,
                                                         announcement_id);

  if (cql_result0.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The announcement does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_result0.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the announcement";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the user is an admin or the creator of the announcement
  if ((temp_user._user_id.clock_seq_and_node !=
           announcement._created_by.clock_seq_and_node ||
       temp_user._user_id.time_and_version !=
           announcement._created_by.time_and_version) &&
      temp_user._user_type != UserType::ADMIN) {
    json_response["error"] = "You are not allowed to delete this announcement";
    return {drogon::HttpStatusCode::k403Forbidden, json_response};
  }

  // Delete the announcement
  auto cql_result1 = _announcements_cql_manager->delete_announcement_by_id(
      school_id, announcement_id, announcement._created_at);

  if (cql_result1.code() != ResultCode::OK) {
    json_response["error"] = "Could not delete the announcement";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Get the tags that are related to the announcement
  auto [cql_result2, tags_ids] =
      _announcements_by_tag_cql_manager->get_tags_by_announcement(
          school_id, announcement_id);

  if (cql_result2.code() != ResultCode::OK &&
      cql_result2.code() != ResultCode::NOT_FOUND) {
    json_response["error"] = "Could not get the tags of the announcement";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Delete the announcement from the tags
  for (const auto &tag_id : tags_ids) {
    auto cql_result3 = _announcements_by_tag_cql_manager->delete_relationship(
        school_id, tag_id, announcement_id);

    if (cql_result3.code() != ResultCode::OK) {
      json_response["error"] =
          "Could not delete the announcement from the tags";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }
  }

  // Get the answers that are related to the announcement
  auto [cql_result4, answers_ids] =
      _answers_by_announcement_or_question_cql_manager
          ->get_answers_by_announcement_or_question(school_id, announcement_id,
                                                    0);

  if (cql_result4.code() != ResultCode::OK &&
      cql_result4.code() != ResultCode::NOT_FOUND) {
    json_response["error"] = "Could not get the answers of the announcement";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Delete the answers
  for (const auto &answer_id : answers_ids) {
    // Get the answers (unfortunately we need it to delete the answer... thanks
    // cassandra)
    auto [cql_result5, answer] =
        _answers_cql_manager->get_answer_by_id(school_id, answer_id);

    if (cql_result5.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the answer";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    // Delete the answer
    auto cql_result6 = _answers_cql_manager->delete_answer(school_id, answer_id,
                                                           answer._created_at);

    if (cql_result6.code() != ResultCode::OK) {
      json_response["error"] = "Could not delete the answers";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }
  }

  // Delete the answers by question relation
  auto cql_result7 = _answers_by_announcement_or_question_cql_manager
                         ->delete_relationships_by_announcement_or_question(
                             school_id, announcement_id, 0);

  if (cql_result7.code() != ResultCode::OK) {
    json_response["error"] = "Could not delete the answers by question";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Delete the files
  for (const auto &file_id : announcement._files) {
    auto cql_result8 = _files_cql_manager->delete_file(school_id, file_id);

    if (cql_result8.code() != ResultCode::OK) {
      json_response["error"] = "Could not delete the files";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }
  }

  json_response["path"] = "../files/schools/" + std::to_string(school_id) +
                          "/announcements/" + get_uuid_string(announcement._id);

  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
AnnouncementManager::add_tag_to_announcement(const int school_id,
                                             const std::string &token,
                                             const CassUuid &announcement_id,
                                             const CassUuid &tag_id) {
  // Get the user by token
  UserObject temp_user;

  auto [status_code, json_response] =
      get_user_by_token(school_id, token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check if the announcement exists
  auto [cql_result1, announcement] =
      _announcements_cql_manager->get_announcement_by_id(school_id,
                                                         announcement_id);

  if (cql_result1.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The announcement does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_result1.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the announcement";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the tag exists
  auto [cql_result2, tag] = _tags_cql_manager->get_tag_by_id(school_id, tag_id);

  if (cql_result2.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The tag does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_result2.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the tag";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the user is an admin or created the announcement
  if (temp_user._user_type != UserType::ADMIN &&
      temp_user._user_id.clock_seq_and_node !=
          announcement._created_by.clock_seq_and_node &&
      temp_user._user_id.time_and_version !=
          announcement._created_by.time_and_version) {
    json_response["error"] =
        "You are not allowed to add tags to this announcement";
    return {drogon::HttpStatusCode::k403Forbidden, json_response};
  }

  // Add the tag to the announcement
  auto cql_result3 = _announcements_by_tag_cql_manager->create_relationship(
      school_id, tag_id, announcement_id);

  if (cql_result3.code() == ResultCode::NOT_APPLIED) {
    json_response["error"] = "The tag is already added to the announcement";
    return {drogon::HttpStatusCode::k409Conflict, json_response};
  } else if (cql_result3.code() != ResultCode::OK) {
    json_response["error"] = "Could not add the tag to the announcement";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
AnnouncementManager::get_announcement_tags(const int school_id,
                                           const std::string &token,
                                           const CassUuid &announcement_id) {
  // Get the user by token
  UserObject temp_user;

  auto [status_code, json_response] =
      get_user_by_token(school_id, token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check if the announcement exists
  auto [cql_result1, announcement] =
      _announcements_cql_manager->get_announcement_by_id(school_id,
                                                         announcement_id);

  if (cql_result1.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The announcement does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_result1.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the announcement";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the user is an admin or created the announcement
  if (temp_user._user_type != UserType::ADMIN &&
      temp_user._user_id.clock_seq_and_node !=
          announcement._created_by.clock_seq_and_node &&
      temp_user._user_id.time_and_version !=
          announcement._created_by.time_and_version) {
    json_response["error"] =
        "You are not allowed to get the tags of this announcement";
    return {drogon::HttpStatusCode::k403Forbidden, json_response};
  }

  // Get the tags of the announcement
  auto [cql_result2, tags_ids] =
      _announcements_by_tag_cql_manager->get_tags_by_announcement(
          school_id, announcement_id);

  if (cql_result2.code() != ResultCode::OK &&
      cql_result2.code() != ResultCode::NOT_FOUND) {
    json_response["error"] = "Could not get the tags of the announcement";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Get the tags
  std::vector<TagObject> tags;

  for (const auto &tag_id : tags_ids) {
    auto [cql_result3, tag] =
        _tags_cql_manager->get_tag_by_id(school_id, tag_id);

    if (cql_result3.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the tag";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    tags.push_back(tag);
  }

  // Return the tags
  for (const auto &tag : tags) {
    Json::Value tag_json;

    tag_json["id"] = get_uuid_string(tag._id);
    tag_json["name"] = tag._name;
    tag_json["colour"] = tag._colour;

    json_response.append(tag_json);
  }

  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
AnnouncementManager::remove_tag_from_announcement(
    const int school_id, const std::string &token,
    const CassUuid &announcement_id, const CassUuid &tag_id) {
  // Get the user by token
  UserObject temp_user;

  auto [status_code, json_response] =
      get_user_by_token(school_id, token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check if the announcement exists
  auto [cql_result1, announcement] =
      _announcements_cql_manager->get_announcement_by_id(school_id,
                                                         announcement_id);

  if (cql_result1.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The announcement does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_result1.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the announcement";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the tag exists
  auto [cql_result2, tag] = _tags_cql_manager->get_tag_by_id(school_id, tag_id);

  if (cql_result2.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The tag does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_result2.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the tag";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the user is an admin or created the announcement
  if (temp_user._user_type != UserType::ADMIN &&
      temp_user._user_id.clock_seq_and_node !=
          announcement._created_by.clock_seq_and_node &&
      temp_user._user_id.time_and_version !=
          announcement._created_by.time_and_version) {
    json_response["error"] =
        "You are not allowed to remove tags to this announcement";
    return {drogon::HttpStatusCode::k403Forbidden, json_response};
  }

  // Remove the tag from the announcement
  auto cql_result3 = _announcements_by_tag_cql_manager->delete_relationship(
      school_id, tag_id, announcement_id);

  if (cql_result3.code() != ResultCode::OK &&
      cql_result3.code() != ResultCode::NOT_APPLIED) {
    json_response["error"] = "Could not remove the tag from the announcement";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
AnnouncementManager::create_announcement_file(const int school_id,
                                              const std::string &user_token,
                                              const CassUuid announcement_id,
                                              const std::string &file_name,
                                              std::string file_extension) {
  // Get the user by token
  UserObject temp_user;

  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check if the announcement exists
  auto [cql_result0, announcement] =
      _announcements_cql_manager->get_announcement_by_id(school_id,
                                                         announcement_id);

  if (cql_result0.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The announcement does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_result0.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the announcement";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the user is an admin or created the announcement
  if (temp_user._user_type != UserType::ADMIN &&
      temp_user._user_id.clock_seq_and_node !=
          announcement._created_by.clock_seq_and_node &&
      temp_user._user_id.time_and_version !=
          announcement._created_by.time_and_version) {
    json_response["error"] =
        "You are not allowed to create files for this announcement";
    return {drogon::HttpStatusCode::k403Forbidden, json_response};
  }

  // Create the file
  CassUuid temp_file_id = create_current_uuid();
  std::string file_path = drogon::app().getUploadPath() + "/schools/" +
                          std::to_string(school_id) + "/announcements/" +
                          get_uuid_string(announcement_id) + "/" +
                          get_uuid_string(temp_file_id) + file_extension;
  FileObject temp_file(school_id, temp_file_id, CustomFileType::FILE, file_name,
                       {}, file_path, 100, temp_user._user_id, true, false);

  // Add the file to the database
  auto cql_result1 = _files_cql_manager->create_file(temp_file);

  if (cql_result1.code() != ResultCode::OK) {
    json_response["error"] = "Could not create the file";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Add the file to the announcement
  announcement._files.push_back(temp_file_id);

  auto cql_result2 = _announcements_cql_manager->update_announcement(
      announcement._school_id, announcement._id, announcement._created_at,
      announcement._created_by, announcement._title, announcement._content,
      announcement._allow_answers, announcement._files);

  if (cql_result2.code() != ResultCode::OK) {
    json_response["error"] = "Could not add the file to the announcement";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Return the file
  json_response = temp_file.to_json(true);
  json_response["created_by_user_name"] =
      temp_user._first_name + " " + temp_user._last_name;
  json_response["path_to_file"] = file_path;

  return {drogon::HttpStatusCode::k201Created, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
AnnouncementManager::delete_announcement_file(const int school_id,
                                              const std::string &user_token,
                                              const CassUuid announcement_id,
                                              const CassUuid file_id) {
  // Get the user by token
  UserObject temp_user;

  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check if the announcement exists
  auto [cql_result0, announcement] =
      _announcements_cql_manager->get_announcement_by_id(school_id,
                                                         announcement_id);

  if (cql_result0.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The announcement does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  }
  if (cql_result0.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the announcement";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the user is an admin or the creator of the announcement
  if ((temp_user._user_id.clock_seq_and_node !=
           announcement._created_by.clock_seq_and_node ||
       temp_user._user_id.time_and_version !=
           announcement._created_by.time_and_version) &&
      temp_user._user_type != UserType::ADMIN) {
    json_response["error"] = "You do not have permission to delete this file";
    return {drogon::HttpStatusCode::k403Forbidden, json_response};
  }

  // Check if the file exists in the announcement
  // also get the index of the file in the announcement
  bool file_exists = false;
  int file_index = 0;

  for (int i = 0; i < announcement._files.size(); i++) {
    if (announcement._files[i].clock_seq_and_node ==
            file_id.clock_seq_and_node &&
        announcement._files[i].time_and_version == file_id.time_and_version) {
      file_exists = true;
      file_index = i;
      break;
    }
  }

  if (!file_exists) {
    json_response["error"] = "The file does not exist in the announcement";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  }

  // Get the file
  auto [cql_result1, file] =
      _files_cql_manager->get_file_by_id(school_id, file_id);

  if (cql_result1.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the file";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Delete the file
  auto cql_result2 = _files_cql_manager->delete_file(school_id, file_id);

  if (cql_result2.code() != ResultCode::OK) {
    json_response["error"] = "Could not delete the file";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Remove the file from the announcement (we have the index)
  announcement._files.erase(announcement._files.begin() + file_index);

  auto cql_result3 = _announcements_cql_manager->update_announcement(
      announcement._school_id, announcement._id, announcement._created_at,
      announcement._created_by, announcement._title, announcement._content,
      announcement._allow_answers, announcement._files);

  if (cql_result3.code() != ResultCode::OK) {
    json_response["error"] = "Could not remove the file from the announcement";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  if (announcement._files.empty()) {
    // Put the path of the announcement folder in the response
    // It it the path of the file - the file name
    json_response["path"] =
        file._path_to_file.erase(file._path_to_file.find_last_of("/"));
  } else
    json_response["path"] = file._path_to_file;

  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
AnnouncementManager::has_permission_to_get_file(const int school_id,
                                                const std::string &user_token,
                                                const CassUuid announcement_id,
                                                const CassUuid &file_id) {
  // Get the user by token
  UserObject temp_user;

  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check if the announcement exists
  auto [cql_result0, announcement] =
      _announcements_cql_manager->get_announcement_by_id(school_id,
                                                         announcement_id);

  if (cql_result0.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The announcement does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_result0.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the announcement";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the user has access to the announcement
  auto [http_response, json_response2] =
      user_has_access_to_announcement(school_id, temp_user, announcement_id);

  if (http_response != drogon::HttpStatusCode::k200OK) {
    return {http_response, json_response2};
  }

  // Check if the file is in the announcement
  // also get the index
  bool file_exists = false;

  for (int i = 0; i < announcement._files.size(); i++) {
    if (announcement._files[i].clock_seq_and_node ==
            file_id.clock_seq_and_node &&
        announcement._files[i].time_and_version == file_id.time_and_version) {
      file_exists = true;
      break;
    }
  }

  if (!file_exists) {
    json_response["error"] = "The file does not exist in the announcement";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  }

  // Put the path of the file in the response
  // Get the file
  auto [cql_result1, file] =
      _files_cql_manager->get_file_by_id(school_id, file_id);

  if (cql_result1.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the file";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  json_response["path"] = file._path_to_file;

  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
AnnouncementManager::create_answer(const int school_id,
                                   const std::string &user_token,
                                   const CassUuid announcement_id,
                                   const std::string &content) {
  // Get the user by token
  UserObject temp_user;

  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check if the announcement exists
  auto [cql_result0, announcement] =
      _announcements_cql_manager->get_announcement_by_id(school_id,
                                                         announcement_id);

  if (cql_result0.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The announcement does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_result0.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the announcement";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the user has access to the announcement
  auto [http_response, json_response2] =
      user_has_access_to_announcement(school_id, temp_user, announcement_id);

  if (http_response != drogon::HttpStatusCode::k200OK) {
    return {http_response, json_response2};
  }

  // Check if the announcement allows answers
  if (!announcement._allow_answers) {
    json_response["error"] = "The announcement does not allow answers";
    return {drogon::HttpStatusCode::k403Forbidden, json_response};
  }

  // Create the answer
  CassUuid answer_id = create_current_uuid();
  AnswerObject answer(school_id, answer_id, time(nullptr), temp_user._user_id,
                      content);

  auto cql_result1 = _answers_cql_manager->create_answer(answer);

  if (cql_result1.code() != ResultCode::OK) {
    json_response["error"] = "Could not create the answer";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Create the relation between the answer and the announcement
  auto cql_result2 =
      _answers_by_announcement_or_question_cql_manager->create_relationship(
          school_id, announcement_id, 0, answer_id);

  if (cql_result2.code() != ResultCode::OK) {
    json_response["error"] =
        "Could not create the relationship between the answer and the "
        "announcement";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Return the answer
  json_response["id"] = get_uuid_string(answer_id);
  json_response["created_at"] = (Json::Value::Int64)answer._created_at;
  json_response["created_by_user_name"] =
      temp_user._first_name + " " + temp_user._last_name;
  json_response["created_by_user_id"] = get_uuid_string(temp_user._user_id);
  json_response["content"] = answer._content;

  return {drogon::HttpStatusCode::k201Created, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
AnnouncementManager::user_has_access_to_announcement(
    const int school_id, const UserObject &user,
    const CassUuid &announcement_id) {
  Json::Value json_response;

  // Check if the user is an admin
  if (user._user_type == UserType::ADMIN) {
    return {drogon::HttpStatusCode::k200OK, json_response};
  }

  // Get all the user's tags
  auto [cql_result0, user_tags] =
      _tags_by_user_cql_manager->get_tags_by_user(school_id, user._user_id);

  if (cql_result0.code() == ResultCode::NOT_FOUND) {
    return {drogon::HttpStatusCode::k200OK, json_response};
  } else if (cql_result0.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the user's tags";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Get the tag's announcements
  for (const auto &tag : user_tags) {
    auto [cql_result1, tag_announcements] =
        _announcements_by_tag_cql_manager->get_announcements_by_tag(school_id,
                                                                    tag);

    if (cql_result1.code() == ResultCode::NOT_FOUND) {
      continue;
    } else if (cql_result1.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the tag's announcements";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    // Check if the announcement is in the tag's announcements
    for (const auto &tag_announcement : tag_announcements) {
      if (tag_announcement.clock_seq_and_node ==
              announcement_id.clock_seq_and_node &&
          tag_announcement.time_and_version ==
              announcement_id.time_and_version) {
        return {drogon::HttpStatusCode::k200OK, json_response};
      }
    }
  }

  // Return 403
  json_response["error"] = "The user does not have access to the announcement";
  return {drogon::HttpStatusCode::k403Forbidden, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> AnnouncementManager::get_answers(
    const int school_id, const CassUuid announcement_id) {
  Json::Value json_response;

  auto [cql_result0, answers_ids] =
      _answers_by_announcement_or_question_cql_manager
          ->get_answers_by_announcement_or_question(school_id, announcement_id,
                                                    0);
  if (cql_result0.code() == ResultCode::NOT_FOUND) {
    json_response = Json::arrayValue;
    return {drogon::HttpStatusCode::k200OK, json_response};
  } else if (cql_result0.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the answers";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Get the answers
  std::vector<AnswerObject> answers;
  for (const auto &answer_id : answers_ids) {
    auto [cql_result1, temp_answer] =
        _answers_cql_manager->get_answer_by_id(school_id, answer_id);

    if (cql_result1.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the answers";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    answers.push_back(temp_answer);
  }

  for (const auto &answer : answers) {
    Json::Value json_answer;
    json_answer["id"] = get_uuid_string(answer._id);
    json_answer["content"] = answer._content;
    json_answer["created_at"] = (Json::Value::Int64)answer._created_at;
    json_answer["created_by_user_id"] = get_uuid_string(answer._created_by);

    // Get the user
    auto [cql_result2, user] =
        _users_cql_manager->get_user(school_id, answer._created_by);

    if (cql_result2.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the user";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    json_answer["created_by_user_name"] =
        user._first_name + " " + user._last_name;

    json_response.append(json_answer);
  }

  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
AnnouncementManager::delete_answer(const int school_id,
                                   const std::string &user_token,
                                   const CassUuid announcement_id,
                                   const CassUuid answer_id) {
  // Get the user by token
  UserObject temp_user;

  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check if the announcement exists
  auto [cql_result0, announcement] =
      _announcements_cql_manager->get_announcement_by_id(school_id,
                                                         announcement_id);

  if (cql_result0.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The announcement does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_result0.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the announcement";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the answer exists
  auto [cql_result1, answer] =
      _answers_cql_manager->get_answer_by_id(school_id, answer_id);

  if (cql_result1.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The answer does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_result1.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the answer";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the user is an admin or the answer's creator
  if (temp_user._user_type != UserType::ADMIN &&
      temp_user._user_id.clock_seq_and_node !=
          answer._created_by.clock_seq_and_node &&
      temp_user._user_id.time_and_version !=
          answer._created_by.time_and_version) {
    json_response["error"] =
        "The user does not have access to delete this answer";
    return {drogon::HttpStatusCode::k403Forbidden, json_response};
  }

  // Delete the answer
  auto cql_result2 = _answers_cql_manager->delete_answer(school_id, answer_id,
                                                         answer._created_at);

  if (cql_result2.code() != ResultCode::OK) {
    json_response["error"] = "Could not delete the answer";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Delete the answer from the announcement's answers
  auto cql_result3 =
      _answers_by_announcement_or_question_cql_manager->delete_relationship(
          school_id, announcement_id, 0, answer_id);

  if (cql_result3.code() != ResultCode::OK) {
    json_response["error"] =
        "Could not delete the answer from the announcement's answers";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
AnnouncementManager::get_file_json(const int school_id,
                                   const CassUuid &file_id) {
  Json::Value json_response;

  // Get the file
  auto [cql_result0, file] =
      _files_cql_manager->get_file_by_id(school_id, file_id);

  if (cql_result0.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "The file does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_result0.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the file";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  json_response = file.to_json(true);

  // Get the creator
  auto [cql_result1, user] =
      _users_cql_manager->get_user(school_id, file._added_by_user);

  if (cql_result1.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the user";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  json_response["created_by_user_name"] =
      user._first_name + " " + user._last_name;

  return {drogon::HttpStatusCode::k200OK, json_response};
}
