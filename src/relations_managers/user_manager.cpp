#include "relations_managers/user_manager.hpp"

std::pair<drogon::HttpStatusCode, Json::Value> UserManager::create_user(
    const int school_id, const std::string &creator_token,
    const std::string &email, UserType user_type, const std::string &first_name,
    const std::string &last_name, const std::string phone_number = "") {
  Json::Value json_response;

  // Check if the school exists
  auto [cql_response, school] = _school_cql_manager->get_school(school_id);

  if (cql_response.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "School not found";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_response.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the creator token is valid and the user is an admin
  auto [cql_response1, admin_token] =
      _tokens_cql_manager->get_user_from_token(school_id, creator_token);

  if (cql_response1.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Creator token not found";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_response1.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Get the user from the database based on the token
  auto [cql_response2, admin] =
      _users_cql_manager->get_user(school_id, admin_token);

  if (cql_response2.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Creator not found";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_response2.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the creator is an admin
  if (admin._user_type != UserType::ADMIN) {
    json_response["error"] = "Creator is not an admin";
    return {drogon::HttpStatusCode::k401Unauthorized, json_response};
  }

  // Check if the email is already in use
  auto [cql_response4, check_user] =
      _users_cql_manager->get_user_by_email(school_id, email);

  if (cql_response4.code() == ResultCode::OK) {
    json_response["error"] = "Email already in use";
    return {drogon::HttpStatusCode::k409Conflict, json_response};
  } else if (cql_response4.code() != ResultCode::NOT_FOUND) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Add the user to the database
  CassUuid user_id = create_current_uuid();
  std::string password = generate_password();
  std::string hashed_password = bcrypt::generateHash(password);

  UserObject user(school_id, user_id, email, hashed_password, user_type, false,
                  first_name, last_name, phone_number, time(nullptr));
  auto cql_response3 = _users_cql_manager->create_user(user);

  if (cql_response3.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  json_response = user.to_json(true);
  json_response["password"] = password;

  // Return 200 OK
  return {drogon::HttpStatusCode::k201Created, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> UserManager::get_user(
    const int school_id, const std::string &token, const CassUuid &user_id) {
  Json::Value json_response;

  // Check if the school exists
  auto [cql_response, school] = _school_cql_manager->get_school(school_id);

  if (cql_response.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "School not found";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_response.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the token is valid
  auto [cql_response1, user_token] =
      _tokens_cql_manager->get_user_from_token(school_id, token);

  if (cql_response1.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Token not found";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_response1.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Get the user from the database based on the token
  auto [cql_response2, user] =
      _users_cql_manager->get_user(school_id, user_token);

  if (cql_response2.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "User not found";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_response2.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the user is an admin
  if (user._user_type != UserType::ADMIN &&
      user._user_id.clock_seq_and_node != user_id.clock_seq_and_node) {
    json_response["error"] = "User is not an admin";
    return {drogon::HttpStatusCode::k401Unauthorized, json_response};
  }

  // Get the user from the database based on the user id
  auto [cql_response3, user1] =
      _users_cql_manager->get_user(school_id, user_id);

  if (cql_response3.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "User not found";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_response3.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Return the user1
  json_response["last_time_online"] =
      (Json::Value::Int64)user1._last_time_online;
  json_response["changed_password"] = user1._changed_password;
  json_response["phone_number"] = user1._phone_number;
  json_response["email"] = user1._email;
  json_response["user_id"] = get_uuid_string(user1._user_id);
  json_response["user_type"] = static_cast<int>(user1._user_type);
  json_response["first_name"] = user1._first_name;
  json_response["last_name"] = user1._last_name;

  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> UserManager::get_all_users(
    const int school_id, const std::string &token) {
  Json::Value json_response;

  // Check if the school exists
  auto [cql_response, school] = _school_cql_manager->get_school(school_id);

  if (cql_response.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "School not found";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_response.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the token is valid
  auto [cql_response1, user_token] =
      _tokens_cql_manager->get_user_from_token(school_id, token);

  if (cql_response1.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Token not found";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_response1.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Get the user from the database based on the token
  auto [cql_response2, user] =
      _users_cql_manager->get_user(school_id, user_token);

  if (cql_response2.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "User not found";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_response2.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the user is an admin or a teacher
  if (user._user_type != UserType::ADMIN &&
      user._user_type != UserType::TEACHER) {
    json_response["error"] = "User is not an admin or a teacher";
    return {drogon::HttpStatusCode::k401Unauthorized, json_response};
  }

  // Get all the users from the database
  auto [cql_response3, users] =
      _users_cql_manager->get_users_by_school(school_id);

  if (cql_response3.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Return the users
  for (auto &user : users) {
    json_response.append(user.to_json(true));
  }

  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> UserManager::update_user(
    const int school_id, const std::string &editor_token,
    const CassUuid &user_id, const std::optional<std::string> &email,
    const std::optional<std::string> &password,
    const std::optional<UserType> user_type,
    const std::optional<std::string> &first_name,
    const std::optional<std::string> &last_name,
    const std::optional<std::string> &phone_number) {
  Json::Value json_response;

  // Check if the school exists
  auto [cql_response, school] = _school_cql_manager->get_school(school_id);

  if (cql_response.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "School not found";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_response.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the token is valid
  auto [cql_response1, user_token] =
      _tokens_cql_manager->get_user_from_token(school_id, editor_token);

  if (cql_response1.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Invalid token";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_response1.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Get the editor from the database based on the token
  auto [cql_response2, editor] =
      _users_cql_manager->get_user(school_id, user_token);

  if (cql_response2.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Editor not found";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_response2.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the user is an admin or the editor is the same as the user to be
  // updated

  if (editor._user_type != UserType::ADMIN &&
      editor._user_id.clock_seq_and_node != user_id.clock_seq_and_node) {
    json_response["error"] = "Unauthorized";
    return {drogon::HttpStatusCode::k401Unauthorized, json_response};
  }

  // Get the user from the database based on the user id
  auto [cql_response3, user] = _users_cql_manager->get_user(school_id, user_id);

  if (cql_response3.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "User not found";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_response3.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Update the user
  if (email.has_value()) {
    user._email = email.value();
  }
  if (password.has_value()) {
    // Only the user can change his password

    if (editor._user_id.clock_seq_and_node != user_id.clock_seq_and_node ||
        editor._user_id.time_and_version != user_id.time_and_version) {
      json_response["error"] = "Unauthorized";
      return {drogon::HttpStatusCode::k401Unauthorized, json_response};
    }

    user._password = bcrypt::generateHash(password.value());
    user._changed_password = true;
  }
  if (user_type.has_value()) {
    // Only an admin can change the user type

    if (editor._user_type != UserType::ADMIN) {
      json_response["error"] = "Unauthorized";
      return {drogon::HttpStatusCode::k401Unauthorized, json_response};
    }

    user._user_type = user_type.value();
  }
  if (first_name.has_value()) {
    user._first_name = first_name.value();
  }
  if (last_name.has_value()) {
    user._last_name = last_name.value();
  }
  if (phone_number.has_value()) {
    user._phone_number = phone_number.value();
  }

  // Edit the user in the database based on the user id
  auto cql_response4 = _users_cql_manager->update_user(
      school_id, user_id, user._email, user._password, user._user_type,
      user._changed_password, user._first_name, user._last_name,
      user._phone_number, user._last_time_online);

  if (cql_response4.code() == ResultCode::NOT_APPLIED) {
    json_response["error"] = "User not found";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_response4.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Return 200 OK
  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> UserManager::delete_user(
    const int school_id, const std::string &token, const CassUuid &user_id) {
  Json::Value json_response;

  // Check if the token is valid
  auto [cql_response1, admin_id] =
      _tokens_cql_manager->get_user_from_token(school_id, token);

  if (cql_response1.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Invalid token";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_response1.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Get the admin from the database based on the token
  auto [cql_response2, admin_user] =
      _users_cql_manager->get_user(school_id, admin_id);

  if (cql_response2.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "User not found";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_response2.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the user is an admin
  // Only an admins can delete a user
  if (admin_user._user_type != UserType::ADMIN) {
    json_response["error"] = "Unauthorized";
    return {drogon::HttpStatusCode::k401Unauthorized, json_response};
  }

  // Check if the user to be deleted exists
  auto [cql_response3, user] = _users_cql_manager->get_user(school_id, user_id);

  if (cql_response3.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "User not found";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_response3.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Delete the answers and questions posted by the user in a course
  auto [cql_response4, courses_ids] =
      _courses_by_user_cql_manager->get_courses_by_user(school_id, user_id);

  if (cql_response4.code() != ResultCode::OK &&
      cql_response4.code() != ResultCode::NOT_FOUND) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  for (const auto &course_id : courses_ids) {
    auto [response_code, response_json] =
        delete_question_and_answers_of_user(school_id, course_id, user_id);
    if (response_code != drogon::HttpStatusCode::k200OK) {
      return {response_code, response_json};
    }
  }

  // Delete the relations with the courses
  auto [response_code1, response_json1] =
      delete_relation_courses(school_id, user_id);
  if (response_code1 != drogon::HttpStatusCode::k200OK) {
    return {response_code1, response_json1};
  }

  // Delete the relations with the tags
  auto [response_code2, response_json2] =
      delete_relation_tags(school_id, user_id);
  if (response_code2 != drogon::HttpStatusCode::k200OK) {
    return {response_code2, response_json2};
  }

  // TODO: Delete the announcements created by the user

  // Delete the user's todos
  auto [response_code4, response_json4] =
      delete_relation_todos(school_id, user_id);
  if (response_code4 != drogon::HttpStatusCode::k200OK) {
    return {response_code4, response_json4};
  }

  // Delete the grades assigned or received by the user
  auto [response_code5, response_json5] =
      delete_relation_grades(school_id, user_id, user._user_type);
  if (response_code5 != drogon::HttpStatusCode::k200OK) {
    return {response_code5, response_json5};
  }

  // Delete the user's tokens (not necessary because it will remain active
  // for just 3 months)

  // Delete the user in the database based on the user id
  auto cql_response5 = _users_cql_manager->delete_user(school_id, user_id);

  if (cql_response5.code() == ResultCode::NOT_APPLIED) {
    json_response["error"] = "User not found";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_response5.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Return 200 OK
  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> UserManager::log_in(
    const int school_id, const std::string &email,
    const std::string &password) {
  Json::Value json_response;

  // Search for the user in the database
  auto [cql_response, user] =
      _users_cql_manager->get_user_by_email(school_id, email);

  if (cql_response.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Invalid credentials";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_response.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the password is correct
  if (!bcrypt::validatePassword(password, user._password)) {
    json_response["error"] = "Invalid credentials";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  }

  // Generate a token
  std::string token = generate_token(school_id);

  // Insert the token in the database
  auto cql_response1 =
      _tokens_cql_manager->create_token(school_id, token, user._user_id);

  if (cql_response1.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Update the user's last time online
  // Do we want this? we only use the 'last_time_online' in order to make new
  // announcements easy to see. The correct way of doing this would be to update
  // the variable each time the user accesses the /api/announcements {GET}. auto
  // cql_response2 = _users_cql_manager->update_user(
  //     school_id, user._user_id, user._email, user._password, user._user_type,
  //     user._changed_password, user._first_name, user._last_name,
  //     user._phone_number, time(nullptr));

  // if (cql_response2.code() != ResultCode::OK) {
  //   json_response["error"] = "Internal error";
  //   return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  // }

  // Return 200 OK + token + user
  json_response["token"] = token;
  json_response["last_time_online"] =
      (Json::Value::Int64)user._last_time_online;
  json_response["changed_password"] = user._changed_password;
  json_response["phone_number"] = user._phone_number;
  json_response["email"] = user._email;
  json_response["user_id"] = get_uuid_string(user._user_id);
  json_response["user_type"] = static_cast<int>(user._user_type);
  json_response["first_name"] = user._first_name;
  json_response["last_name"] = user._last_name;

  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> UserManager::log_out(
    const int school_id, const std::string &token) {
  Json::Value json_response;

  // Delete the token from the database
  auto cql_response = _tokens_cql_manager->delete_token(school_id, token);

  if (cql_response.code() == ResultCode::NOT_APPLIED) {
    json_response["error"] = "Invalid token";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_response.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Return 200 OK
  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::string UserManager::generate_token(const int school_id) {
  std::random_device rd;
  std::uniform_int_distribution<int> dis(0, 94);

  // Allowed characters for the token
  // Alphanumeric characters: a-z, A-Z, and 0-9
  const char *allowed_characters =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";

  std::string token = "";

  do {
    for (int i = 0; i < 32; i++) {
      token += allowed_characters[dis(rd)];
    }

    std::string token = "";
  } while (token_is_unique(school_id, token));

  return token;
}

std::string UserManager::generate_password() {
  std::random_device rd;
  std::uniform_int_distribution<int> dis(0, 72);

  // Allowed characters for the password
  const char *allowed_characters =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"
      "!@#$%^&*()";

  std::string password = "";

  for (int i = 0; i < 8; i++) {
    password += allowed_characters[dis(rd)];
  }

  return password;
}

bool UserManager::token_is_unique(const int school, const std::string token) {
  auto [cql_response, tokens] =
      _tokens_cql_manager->get_user_from_token(school, token);

  if (cql_response.code() != ResultCode::OK) {
    return false;
  }

  return true;
}

std::pair<drogon::HttpStatusCode, Json::Value>
UserManager::delete_relation_courses(const int school_id,
                                     const CassUuid &user_id) {
  Json::Value json_response;

  auto [cql_response1, courses_id] =
      _courses_by_user_cql_manager->get_courses_by_user(school_id, user_id);

  if (cql_response1.code() != ResultCode::OK &&
      cql_response1.code() != ResultCode::NOT_FOUND) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  for (auto course_id : courses_id) {
    auto cql_response2 = _users_by_course_cql_manager->delete_relationship(
        school_id, course_id, user_id);

    if (cql_response2.code() != ResultCode::OK) {
      json_response["error"] = "Internal error";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }
  }

  auto cql_response3 =
      _courses_by_user_cql_manager->delete_all_relationships_of_user(school_id,
                                                                     user_id);

  if (cql_response3.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
UserManager::delete_relation_tags(const int school_id,
                                  const CassUuid &user_id) {
  Json::Value json_response;

  auto [cql_response1, tags_id] =
      _tags_by_user_cql_manager->get_tags_by_user(school_id, user_id);

  if (cql_response1.code() != ResultCode::OK &&
      cql_response1.code() != ResultCode::NOT_FOUND) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  for (auto tag_id : tags_id) {
    auto cql_response2 = _users_by_tag_cql_manager->delete_relationship(
        school_id, tag_id, user_id);

    if (cql_response2.code() != ResultCode::OK) {
      json_response["error"] = "Internal error";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }
  }

  auto cql_response3 = _tags_by_user_cql_manager->delete_relationships_by_user(
      school_id, user_id);

  if (cql_response3.code() != ResultCode::OK) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  return {drogon::HttpStatusCode::k201Created, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
UserManager::delete_relation_todos(const int school_id,
                                   const CassUuid &user_id) {
  Json::Value json_response;

  auto [cql_response1, todos_id] =
      _todos_by_user_cql_manager->get_todos_by_user(school_id, user_id);

  if (cql_response1.code() != ResultCode::OK &&
      cql_response1.code() != ResultCode::NOT_FOUND) {
    json_response["error"] = "Internal error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  for (auto todo_id : todos_id) {
    auto cql_response2 = _todos_cql_manager->delete_todo(school_id, todo_id);

    if (cql_response2.code() != ResultCode::OK) {
      json_response["error"] = "Internal error";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }
  }

  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
UserManager::delete_relation_grades(const int school_id,
                                    const CassUuid &user_id,
                                    const UserType user_type) {
  Json::Value json_response;

  if (UserType::STUDENT == user_type) {
    auto [cql_response1, grades] =
        _grades_cql_manager->get_grades_by_student_id(school_id, user_id);

    if (cql_response1.code() != ResultCode::OK &&
        cql_response1.code() != ResultCode::NOT_FOUND) {
      json_response["error"] = "Internal error";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    for (auto grade : grades) {
      auto cql_response2 =
          _grades_cql_manager->delete_grade(school_id, grade._id);

      if (cql_response2.code() != ResultCode::OK) {
        json_response["error"] = "Internal error";
        return {drogon::HttpStatusCode::k500InternalServerError, json_response};
      }
    }
  } else if (UserType::TEACHER == user_type) {
    auto [cql_response1, grades] =
        _grades_cql_manager->get_grades_by_student_id(school_id, user_id);

    if (cql_response1.code() != ResultCode::OK &&
        cql_response1.code() != ResultCode::NOT_FOUND) {
      json_response["error"] = "Internal error";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    for (auto grade : grades) {
      auto cql_response2 =
          _grades_cql_manager->delete_grade(school_id, grade._id);

      if (cql_response2.code() != ResultCode::OK) {
        json_response["error"] = "Internal error";
        return {drogon::HttpStatusCode::k500InternalServerError, json_response};
      }
    }
  }

  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
UserManager::delete_question_and_answers_of_user(const int school_id,
                                                 const CassUuid course_id,
                                                 const CassUuid user_id) {
  Json::Value json_response;

  // Get the questions
  auto [cql_response1, questions_ids] =
      _questions_by_course_cql_manager->get_questions_by_course(school_id,
                                                                course_id);

  if (cql_response1.code() != ResultCode::OK &&
      cql_response1.code() != ResultCode::NOT_FOUND) {
    json_response["error"] = "Could not get the questions by course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Delete the questions if is the creator of the question is the user
  for (auto &question_id : questions_ids) {
    auto [cql_response2, question_object] =
        _questions_cql_manager->get_question_by_id(school_id, question_id);

    if (cql_response2.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the question";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    // Check if we have to delete the question and the answers
    if (question_object._added_by_user_id.clock_seq_and_node ==
            user_id.clock_seq_and_node &&
        question_object._added_by_user_id.time_and_version ==
            user_id.time_and_version) {
      auto [cql_response3, answers_ids] =
          _answers_by_announcement_or_question_cql_manager
              ->get_answers_by_announcement_or_question(school_id, question_id,
                                                        1);

      if (cql_response3.code() != ResultCode::OK &&
          cql_response3.code() != ResultCode::NOT_FOUND) {
        json_response["error"] = "Could not get the answers by question";
        return {drogon::HttpStatusCode::k500InternalServerError, json_response};
      }

      // Delete the answers
      for (auto &answer_id : answers_ids) {
        auto [cql_response4, answer_object] =
            _answers_cql_manager->get_answer_by_id(school_id, answer_id);

        if (cql_response4.code() != ResultCode::OK) {
          json_response["error"] = "Could not get the answer";
          return {drogon::HttpStatusCode::k500InternalServerError,
                  json_response};
        }

        // Delete the answer
        auto cql_response5 = _answers_cql_manager->delete_answer(
            school_id, answer_id, answer_object._created_at);

        if (cql_response5.code() != ResultCode::OK &&
            cql_response5.code() != ResultCode::NOT_FOUND) {
          json_response["error"] = "Could not delete the answer";
          return {drogon::HttpStatusCode::k500InternalServerError,
                  json_response};
        }
      }

      // Delete the question
      auto cql_response6 =
          _questions_cql_manager->delete_question(school_id, question_id);

      if (cql_response6.code() != ResultCode::OK &&
          cql_response6.code() != ResultCode::NOT_FOUND) {
        json_response["error"] = "Could not delete the question";
        return {drogon::HttpStatusCode::k500InternalServerError, json_response};
      }

      // Delete the relationship between the question and the answers
      auto cql_response7 =
          _answers_by_announcement_or_question_cql_manager
              ->delete_relationships_by_announcement_or_question(
                  school_id, question_id, 1);

      if (cql_response7.code() != ResultCode::OK) {
        json_response["error"] =
            "Could not delete the relationship between the question and the "
            "answers";
        return {drogon::HttpStatusCode::k500InternalServerError, json_response};
      }

      // Delete the question from the course
      auto cql_response8 =
          _questions_by_course_cql_manager->delete_relationship(
              school_id, course_id, question_id);

      if (cql_response8.code() != ResultCode::OK &&
          cql_response8.code() != ResultCode::NOT_FOUND) {
        json_response["error"] =
            "Could not delete the question from the course";
        return {drogon::HttpStatusCode::k500InternalServerError, json_response};
      }
    }
    // Check if we have to delete the answers
    else {
      auto [cql_response9, answers_ids] =
          _answers_by_announcement_or_question_cql_manager
              ->get_answers_by_announcement_or_question(school_id, question_id,
                                                        1);

      if (cql_response9.code() != ResultCode::OK &&
          cql_response9.code() != ResultCode::NOT_FOUND) {
        json_response["error"] = "Could not get the answers by question";
        return {drogon::HttpStatusCode::k500InternalServerError, json_response};
      }

      // Delete the answers
      for (auto &answer_id : answers_ids) {
        auto [cql_response10, answer_object] =
            _answers_cql_manager->get_answer_by_id(school_id, answer_id);

        if (cql_response10.code() != ResultCode::OK) {
          json_response["error"] = "Could not get the answer";
          return {drogon::HttpStatusCode::k500InternalServerError,
                  json_response};
        }

        // If the answer is not created by the user, we don't have to delete it
        if (answer_object._created_by.clock_seq_and_node !=
                user_id.clock_seq_and_node &&
            answer_object._created_by.time_and_version !=
                user_id.time_and_version) {
          continue;
        }

        // Delete the answer
        auto cql_response11 = _answers_cql_manager->delete_answer(
            school_id, answer_id, answer_object._created_at);

        if (cql_response11.code() != ResultCode::OK &&
            cql_response11.code() != ResultCode::NOT_FOUND) {
          json_response["error"] = "Could not delete the answer";
          return {drogon::HttpStatusCode::k500InternalServerError,
                  json_response};
        }

        // Delete the answer from the question
        auto cql_response12 =
            _answers_by_announcement_or_question_cql_manager
                ->delete_relationship(school_id, question_id, 1, answer_id);

        if (cql_response12.code() != ResultCode::OK &&
            cql_response12.code() != ResultCode::NOT_FOUND) {
          json_response["error"] =
              "Could not delete the answer from the question";
          return {drogon::HttpStatusCode::k500InternalServerError,
                  json_response};
        }
      }
    }
  }

  return {drogon::HttpStatusCode::k200OK, json_response};
}
