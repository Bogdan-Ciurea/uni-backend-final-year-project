#include "relations_managers/course_manager.hpp"

std::pair<drogon::HttpStatusCode, Json::Value> CourseManager::get_user_by_token(
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

std::pair<drogon::HttpStatusCode, Json::Value> CourseManager::get_folder_files(
    const int school_id, const std::vector<CassUuid> files,
    const UserType user_type) {
  Json::Value json_response = Json::Value(Json::arrayValue);

  // Get files from the folder

  for (auto &file_id : files) {
    auto [cql_answer, file] =
        _files_cql_manager->get_file_by_id(school_id, file_id);

    if (cql_answer.code() == ResultCode::NOT_FOUND) {
      Json::Value json_response1;
      json_response1["error"] = "Invalid file id";
      return {drogon::HttpStatusCode::k400BadRequest, json_response1};
    } else if (cql_answer.code() != ResultCode::OK) {
      Json::Value json_response1;
      json_response1["error"] = "Could not get the file from the file id";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response1};
    }

    // Check if the file is public
    if (user_type == UserType::STUDENT && !file._visible_to_students) {
      continue;
    }

    json_response.append(file.to_json(true));
  }

  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> CourseManager::create_course(
    const int school_id, const std::string &creator_token,
    const std::string &name, const time_t start_date, const time_t end_date) {
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, creator_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check if user is a teacher or admin
  if (temp_user._user_type != UserType::TEACHER &&
      temp_user._user_type != UserType::ADMIN) {
    json_response["error"] = "User is not a teacher or admin";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  }

  // Build the course object
  CassUuid course_id = create_current_uuid();
  CourseObject course(school_id, course_id, name, "", time(nullptr), start_date,
                      end_date, {});

  // Add the course to the database
  auto cql_result0 = _courses_cql_manager->create_course(course);

  if (cql_result0.code() != ResultCode::OK) {
    json_response["error"] = "Could not create the course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  if (temp_user._user_type == UserType::ADMIN) {
    // Return 201 Created
    json_response = course.to_json(false);
    return {drogon::HttpStatusCode::k201Created, json_response};
  }

  // Add the teacher to the course
  auto cql_result1 = _users_by_course_cql_manager->create_relationship(
      school_id, course._id, temp_user._user_id);

  if (cql_result1.code() != ResultCode::OK) {
    json_response["error"] = "Could not add the teacher to the course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  auto cql_result2 = _courses_by_user_cql_manager->create_relationship(
      school_id, temp_user._user_id, course._id);

  if (cql_result2.code() != ResultCode::OK) {
    json_response["error"] = "Could not add the course to the teacher";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Return 200 OK
  json_response["course_id"] = get_uuid_string(course._id);
  return {drogon::HttpStatusCode::k201Created, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> CourseManager::get_course(
    const int school_id, const std::string &user_token,
    const CassUuid course_id) {
  UserObject temp_user;
  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Get the course
  auto [cql_code1, course] =
      _courses_cql_manager->get_course_by_id(school_id, course_id);

  if (cql_code1.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Invalid course id";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  } else if (cql_code1.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the course from the course id";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  if (temp_user._user_type != UserType::ADMIN) {
    // Get user's courses
    auto [cql_code2, user_courses] =
        _users_by_course_cql_manager->get_users_by_course(school_id, course_id);

    if (cql_code2.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the user's courses";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    // Check if the user is in the course
    bool user_in_course = false;
    for (auto &user_course : user_courses) {
      if (user_course.clock_seq_and_node ==
              temp_user._user_id.clock_seq_and_node &&
          user_course.time_and_version == temp_user._user_id.time_and_version) {
        user_in_course = true;
        break;
      }
    }

    if (!user_in_course) {
      json_response["error"] = "You do not have access to this course";
      return {drogon::HttpStatusCode::k401Unauthorized, json_response};
    }
  }

  json_response["id"] = get_uuid_string(course._id);
  json_response["name"] = course._name;
  json_response["thumbnail"] = "";
  json_response["created_at"] = (Json::Value::Int64)course._created_at;
  json_response["start_date"] = (Json::Value::Int64)course._start_date;
  json_response["end_date"] = (Json::Value::Int64)course._end_date;

  return {drogon::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
CourseManager::get_all_user_courses(const int school_id,
                                    const std::string &user_token) {
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  std::vector<CourseObject> courses;

  // The admin has access to all courses
  if (temp_user._user_type != UserType::ADMIN) {
    // Get user's courses
    auto [cql_result0, courses_ids] =
        _courses_by_user_cql_manager->get_courses_by_user(school_id,
                                                          temp_user._user_id);

    if (cql_result0.code() == ResultCode::NOT_FOUND) {
      json_response = Json::Value(Json::arrayValue);
      return {drogon::HttpStatusCode::k200OK, json_response};
    } else if (cql_result0.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the user's courses";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    // Get the courses
    for (auto &course_id : courses_ids) {
      auto [cql_result1, course] =
          _courses_cql_manager->get_course_by_id(school_id, course_id);

      if (cql_result1.code() != ResultCode::OK) {
        json_response["error"] = "Could not get the course";
        return {drogon::HttpStatusCode::k500InternalServerError, json_response};
      }

      courses.push_back(course);
    }
  } else {
    auto [cql_result0, courses_list] =
        _courses_cql_manager->get_courses_by_school(school_id);

    if (cql_result0.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the courses";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    courses = std::move(courses_list);
  }

  if (courses.empty()) {
    json_response = Json::Value(Json::arrayValue);
    return {drogon::HttpStatusCode::k200OK, json_response};
  }

  // Build the json response
  for (auto &course : courses) {
    json_response.append(course.to_json(false));
  }

  // Return 200 OK
  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> CourseManager::get_courses_users(
    const int school_id, const CassUuid course_id,
    const std::string &user_token) {
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Get users in this course
  auto [cql_result0, users] =
      _users_by_course_cql_manager->get_users_by_course(school_id, course_id);

  if (cql_result0.code() == ResultCode::NOT_FOUND) {
    json_response = Json::Value(Json::arrayValue);
    return {drogon::HttpStatusCode::k200OK, json_response};
  } else if (cql_result0.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the users in this course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  json_response = Json::Value(Json::arrayValue);

  // Get the users
  for (auto &user_id : users) {
    auto [cql_result1, user] = _users_cql_manager->get_user(school_id, user_id);

    if (cql_result1.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the user";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    Json::Value user_json;
    user_json["user_id"] = get_uuid_string(user._user_id);
    user_json["type"] = (int)user._user_type;
    user_json["first_name"] = user._first_name;
    user_json["last_name"] = user._last_name;

    json_response.append(user_json);
  }

  // Return 200 OK
  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> CourseManager::update_course(
    const int school_id, const std::string &user_token,
    const CassUuid course_id, const std::optional<std::string> &title,
    const std::optional<time_t> start_date,
    const std::optional<time_t> end_date) {
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check if user is a teacher or admin
  if (temp_user._user_type != UserType::ADMIN &&
      temp_user._user_type != UserType::TEACHER) {
    json_response["error"] = "You cannot update a course";
    return {drogon::HttpStatusCode::k401Unauthorized, json_response};
  }

  // Get the course
  auto [cql_code0, course] =
      _courses_cql_manager->get_course_by_id(school_id, course_id);

  if (cql_code0.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Course not found";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_code0.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Update the course
  if (title.has_value()) {
    course._name = title.value();
  }
  if (start_date.has_value()) {
    course._start_date = start_date.value();
  }
  if (end_date.has_value()) {
    course._end_date = end_date.value();
  }

  auto cql_code1 = _courses_cql_manager->update_course(
      school_id, course_id, course._name, course._course_thumbnail,
      time(nullptr), course._start_date, course._end_date, course._files);

  if (cql_code1.code() != ResultCode::OK) {
    json_response["error"] = "Could not update the course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  json_response = course.to_json(true);

  // Return 200 OK
  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> CourseManager::delete_course(
    const int school_id, const std::string &user_token,
    const CassUuid course_id) {
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check if user is a teacher or admin
  if (temp_user._user_type != UserType::ADMIN &&
      temp_user._user_type != UserType::TEACHER) {
    json_response["error"] = "You cannot delete a course";
    return {drogon::HttpStatusCode::k401Unauthorized, json_response};
  }

  // Get the course
  auto [cql_code0, course] =
      _courses_cql_manager->get_course_by_id(school_id, course_id);

  if (cql_code0.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Course not found";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_code0.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Get all users' ids in the course
  auto [cql_code1, users_ids] =
      _users_by_course_cql_manager->get_users_by_course(school_id, course_id);

  if (cql_code1.code() != ResultCode::OK &&
      cql_code1.code() != ResultCode::NOT_FOUND) {
    json_response["error"] = "Could not get the users' ids";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Delete the relationship between the users and the course

  auto cql_code2 = _users_by_course_cql_manager->delete_relationships_by_course(
      school_id, course_id);

  if (cql_code2.code() != ResultCode::OK) {
    json_response["error"] =
        "Could not delete the relationship between the users and the course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  for (auto &user_id : users_ids) {
    auto cql_code3 = _courses_by_user_cql_manager->delete_relationship(
        school_id, user_id, course_id);
    if (cql_code2.code() != ResultCode::OK) {
      json_response["error"] =
          "Could not delete the relationship between the users and the course";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }
  }

  // Delete the related files
  for (auto &file_id : course._files) {
    // Get the file
    auto [cql_code4, file_object] =
        _files_cql_manager->get_file_by_id(school_id, file_id);

    if (cql_code4.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the file";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    // If it is not folder, delete it
    if (file_object._type == CustomFileType::FILE) {
      auto cql_code5 = _files_cql_manager->delete_file(school_id, file_id);

      if (cql_code5.code() != ResultCode::OK) {
        json_response["error"] = "Could not delete the file";
        return {drogon::HttpStatusCode::k500InternalServerError, json_response};
      }

      continue;
    }

    // If it is folder, delete all the files in it
    for (auto &file_id : file_object._files) {
      // No need to check if it is folder or file, because it is already checked
      // No need to ger the file, the cql manager will check if it exists

      auto cql_code6 = _files_cql_manager->delete_file(school_id, file_id);

      /*if (cql_code6.code() == ResultCode::NOT_APPLIED) {
        json_response["error"] = "Could not delete the file";
        return {drogon::HttpStatusCode::k500InternalServerError, json_response};
      }*/

      if (cql_code6.code() != ResultCode::OK) {
        json_response["error"] = "Could not delete the file";
        return {drogon::HttpStatusCode::k500InternalServerError, json_response};
      }
    }

    // Delete the folder
    auto cql_code7 = _files_cql_manager->delete_file(school_id, file_id);

    if (cql_code7.code() != ResultCode::OK) {
      json_response["error"] = "Could not delete the file";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }
  }

  // Get all the grades of the course
  auto [cql_code8, grades] =
      _grades_cql_manager->get_grades_by_course_id(school_id, course_id);

  // Delete the grades of the course
  for (auto &grade : grades) {
    auto cql_code9 = _grades_cql_manager->delete_grade(school_id, grade._id);

    if (cql_code9.code() != ResultCode::OK) {
      json_response["error"] = "Could not delete the grade";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }
  }

  // Delete the course
  auto cql_code10 =
      _courses_cql_manager->delete_course_by_id(school_id, course_id);

  if (cql_code10.code() != ResultCode::OK) {
    json_response["error"] = "Could not delete the course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  json_response["path"] = "../files/schools/" + std::to_string(school_id) +
                          "/courses/" + get_uuid_string(course_id);

  // Return 200 OK
  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
CourseManager::set_course_thumbnail(const int school_id,
                                    const std::string &user_token,
                                    const CassUuid course_id,
                                    const std::string &file_extension) {
  // Get the user by token
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check if the course exists
  auto [cql_code, course] =
      _courses_cql_manager->get_course_by_id(school_id, course_id);

  if (cql_code.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Course does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_code.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the user is an admin or (a teacher and is in course)
  if (temp_user._user_type != UserType::ADMIN) {
    if (temp_user._user_type != UserType::TEACHER) {
      json_response["error"] = "User is not a teacher";
      return {drogon::HttpStatusCode::k403Forbidden, json_response};
    }

    auto [cql_response, users_id] =
        _users_by_course_cql_manager->get_users_by_course(school_id, course_id);

    if (cql_response.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the users";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    bool user_in_course = false;
    for (auto &user_id : users_id) {
      if (temp_user._user_id.clock_seq_and_node == user_id.clock_seq_and_node &&
          temp_user._user_id.time_and_version == user_id.time_and_version) {
        user_in_course = true;
        break;
      }
    }
    if (!user_in_course) {
      json_response["error"] = "User is not in the course";
      return {drogon::HttpStatusCode::k403Forbidden, json_response};
    }
  }

  std::string thumbnail_path = "../files/schools/" + std::to_string(school_id) +
                               "/courses/" + get_uuid_string(course_id) +
                               "/thumbnail" + file_extension;

  // Update the course thumbnail
  auto cql_code2 = _courses_cql_manager->update_course(
      school_id, course_id, course._name, thumbnail_path, course._created_at,
      course._start_date, course._end_date, course._files);

  // Return 200 OK
  json_response["path"] = thumbnail_path;
  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
CourseManager::get_course_thumbnail(const int school_id,
                                    const std::string &user_token,
                                    const CassUuid course_id) {
  // Get the user by token
  // Get the user by token
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check if the course exists
  auto [cql_code, course] =
      _courses_cql_manager->get_course_by_id(school_id, course_id);

  if (cql_code.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Course does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_code.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the user is in course or is an admin
  if (temp_user._user_type != UserType::ADMIN) {
    auto [cql_response, users_id] =
        _users_by_course_cql_manager->get_users_by_course(school_id, course_id);

    if (cql_response.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the users";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    bool user_in_course = false;
    for (auto &user_id : users_id) {
      if (temp_user._user_id.clock_seq_and_node == user_id.clock_seq_and_node &&
          temp_user._user_id.time_and_version == user_id.time_and_version) {
        user_in_course = true;
        break;
      }
    }
    if (!user_in_course) {
      json_response["error"] = "User is not in the course";
      return {drogon::HttpStatusCode::k403Forbidden, json_response};
    }
  }

  // Return 200 OK
  json_response["path"] = course._course_thumbnail;
  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
CourseManager::delete_course_thumbnail(const int school_id,
                                       const std::string &user_token,
                                       const CassUuid course_id) {
  // Get the user by token
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check if the course exists
  auto [cql_code, course] =
      _courses_cql_manager->get_course_by_id(school_id, course_id);

  if (cql_code.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Course does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_code.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the user is an admin or (a teacher and is in course)
  if (temp_user._user_type != UserType::ADMIN) {
    if (temp_user._user_type != UserType::TEACHER) {
      json_response["error"] = "User is not a teacher";
      return {drogon::HttpStatusCode::k403Forbidden, json_response};
    }

    auto [cql_response, users_id] =
        _users_by_course_cql_manager->get_users_by_course(school_id, course_id);

    if (cql_response.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the users";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    bool user_in_course = false;
    for (auto &user_id : users_id) {
      if (temp_user._user_id.clock_seq_and_node == user_id.clock_seq_and_node &&
          temp_user._user_id.time_and_version == user_id.time_and_version) {
        user_in_course = true;
        break;
      }
    }
    if (!user_in_course) {
      json_response["error"] = "User is not in the course";
      return {drogon::HttpStatusCode::k403Forbidden, json_response};
    }
  }

  std::string thumbnail_path = "";

  // Update the course thumbnail
  auto cql_code2 = _courses_cql_manager->update_course(
      school_id, course_id, course._name, thumbnail_path, course._created_at,
      course._start_date, course._end_date, course._files);

  // Return 200 OK
  json_response["path"] = course._course_thumbnail;
  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
CourseManager::create_course_file(
    const int school_id, const std::string &user_token,
    const CassUuid course_id, const std::string &file_name,
    const CustomFileType file_type, std::string file_extension,
    const std::optional<CassUuid> file_owner, const bool visible_to_students,
    const bool students_can_add) {
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Get the course
  auto [cql_code, course] =
      _courses_cql_manager->get_course_by_id(school_id, course_id);

  if (cql_code.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  /*
    if not has owner:
      check if user is a teacher or admin

      write the file to the database

      add the file to the course

      return 200 OK

    if has owner get owner from database

    check if the owner is a folder

    check if the file is not a folder

    check if (user is student and owner is folder and  owner.students_can_add)
    or user is teacher or admin

    write the file to the database

    add the file to the owner

    return 200 OK
  */
  // if not has owner aka is a file that is not in a folder
  if (!file_owner.has_value()) {
    // Check if user is a teacher or admin

    if (temp_user._user_type != UserType::TEACHER &&
        temp_user._user_type != UserType::ADMIN) {
      json_response["error"] =
          "Only teachers and admins can create files without owner";
      return {drogon::HttpStatusCode::k403Forbidden, json_response};
    }

    // Create the file object
    CassUuid file_id = create_current_uuid();
    std::string path_to_file = drogon::app().getUploadPath() + "/schools/" +
                               std::to_string(school_id) + "/courses/" +
                               get_uuid_string(course_id) + "/files/" +
                               get_uuid_string(file_id) + file_extension;
    FileObject file_object(school_id, file_id, file_type, file_name, {},
                           path_to_file, 1000, temp_user._user_id,
                           visible_to_students, students_can_add);

    // Write the file to the database
    auto cql_code0 = _files_cql_manager->create_file(file_object);

    if (cql_code0.code() != ResultCode::OK) {
      json_response["error"] = "Could not create the file";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    // Add the file to the course
    course._files.push_back(file_id);

    // Update the course
    auto cql_code2 = _courses_cql_manager->update_course(
        course._school_id, course._id, course._name, course._course_thumbnail,
        time(nullptr), course._start_date, course._end_date, course._files);

    if (cql_code2.code() != ResultCode::OK) {
      json_response["error"] = "Could not update the course";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    // Return 201 Created
    json_response = file_object.to_json(true);
    json_response["created_by_user_name"] =
        temp_user._first_name + " " + temp_user._last_name;
    return {drogon::HttpStatusCode::k201Created, json_response};
  }

  // Get the owner from the database
  auto [cql_code3, owner] =
      _files_cql_manager->get_file_by_id(school_id, file_owner.value());

  if (cql_code3.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Could not get the owner";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_code3.code() != ResultCode::OK) {
    json_response["error"] = "Internal server error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the owner is a folder
  if (owner._type != CustomFileType::FOLDER) {
    json_response["error"] =
        "You cannot add a file to a file; only to a folder";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  }

  // Check if the file is not a folder
  if (file_type == CustomFileType::FOLDER) {
    json_response["error"] =
        "You cannot add a folder to a folder; only to a file";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  }

  // Check if (user is student  and  owner.students_can_add) or user is teacher
  // or admin
  if (temp_user._user_type == UserType::STUDENT && !owner._students_can_add) {
    json_response["error"] = "You cannot add a file to this folder";
    return {drogon::HttpStatusCode::k403Forbidden, json_response};
  }

  // Create the file object
  CassUuid file_id = create_current_uuid();
  std::string path_to_file = drogon::app().getUploadPath() + "/schools/" +
                             std::to_string(school_id) + "/courses/" +
                             get_uuid_string(course_id) + "/files/" +
                             get_uuid_string(owner._id) + "/" +
                             get_uuid_string(file_id) + file_extension;
  FileObject file_object(school_id, file_id, file_type, file_name, {},
                         path_to_file, 1000, temp_user._user_id,
                         visible_to_students, students_can_add);

  // Write the file to the database
  auto cql_code4 = _files_cql_manager->create_file(file_object);

  if (cql_code4.code() != ResultCode::OK) {
    json_response["error"] = "Could not create the file";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Add the file to the owner
  owner._files.push_back(file_id);

  // Update the owner
  auto cql_code5 = _files_cql_manager->update_file(
      owner._school_id, owner._id, owner._type, owner._name, owner._files,
      owner._path_to_file, owner._size, owner._added_by_user,
      owner._visible_to_students, owner._students_can_add);

  if (cql_code5.code() != ResultCode::OK) {
    json_response["error"] = "Could not update the owner";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Return 201 Created
  json_response = file_object.to_json(true);
  json_response["created_by_user_name"] =
      temp_user._first_name + " " + temp_user._last_name;
  return {drogon::HttpStatusCode::k201Created, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> CourseManager::get_course_files(
    const int school_id, const CassUuid course_id,
    const std::string &user_token) {
  UserObject temp_user;
  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Get the course
  auto [cql_code1, course] =
      _courses_cql_manager->get_course_by_id(school_id, course_id);

  if (cql_code1.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Invalid course id";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  } else if (cql_code1.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the course from the course id";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  if (temp_user._user_type != UserType::ADMIN) {
    // Get user's courses
    auto [cql_code2, user_courses] =
        _users_by_course_cql_manager->get_users_by_course(school_id, course_id);

    if (cql_code2.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the user's courses";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    // Check if the user is in the course
    bool user_in_course = false;
    for (auto &user_course : user_courses) {
      if (user_course.clock_seq_and_node ==
              temp_user._user_id.clock_seq_and_node &&
          user_course.time_and_version == temp_user._user_id.time_and_version) {
        user_in_course = true;
        break;
      }
    }

    if (!user_in_course) {
      json_response["error"] = "You do not have access to this course";
      return {drogon::HttpStatusCode::k401Unauthorized, json_response};
    }
  }

  json_response = Json::arrayValue;

  // Get the files
  std::vector<FileObject> files;
  for (auto &file_id : course._files) {
    auto [cql_response1, file] =
        _files_cql_manager->get_file_by_id(school_id, file_id);

    if (cql_response1.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the course's files";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    files.push_back(file);
  }

  // Build the json response
  for (auto &file : files) {
    if (temp_user._user_type == UserType::STUDENT &&
        !file._visible_to_students) {
      continue;
    }

    Json::Value file_json = file.to_json(true);

    if (file._type == CustomFileType::FOLDER) {
      auto [status_code, json_files_of_folder] =
          get_folder_files(school_id, file._files, temp_user._user_type);

      if (status_code != drogon::HttpStatusCode::k200OK) {
        continue;
      }

      file_json["files"] = json_files_of_folder;
    }

    // Get the user that created the file
    auto [cql_response2, user] =
        _users_cql_manager->get_user(school_id, file._added_by_user);

    if (cql_response2.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the user that created the file";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    file_json["created_by_user_name"] =
        user._first_name + " " + user._last_name;

    json_response.append(file_json);
  }

  // Return 200 OK
  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
CourseManager::update_course_files(
    const int school_id, const std::string &user_token,
    const CassUuid course_id, const CassUuid file_id,
    const std::optional<std::string> file_name,
    const std::optional<std::vector<CassUuid>> file_ids,
    const std::optional<bool> visible_to_students,
    const std::optional<bool> students_can_add) {
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check if user is a teacher or admin
  if (temp_user._user_type != UserType::TEACHER &&
      temp_user._user_type != UserType::ADMIN) {
    json_response["error"] = "You are not allowed to update files";
    return {drogon::HttpStatusCode::k403Forbidden, json_response};
  }

  // If user is a teacher
  if (temp_user._user_type == UserType::TEACHER) {
    //  Get the course's users
    auto [cql_response0, course_users] =
        _users_by_course_cql_manager->get_users_by_course(school_id, course_id);

    if (cql_response0.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the course's users";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    //  Check if the teacher is in the course's users
    bool teacher_is_in_course = false;
    for (auto &user_id : course_users) {
      if (user_id.clock_seq_and_node == temp_user._user_id.clock_seq_and_node &&
          user_id.time_and_version == temp_user._user_id.time_and_version) {
        teacher_is_in_course = true;
        break;
      }
    }

    if (!teacher_is_in_course) {
      json_response["error"] = "Teacher is not in the course";
      return {drogon::HttpStatusCode::k403Forbidden, json_response};
    }
  }

  // Get the file
  auto [cql_response1, file] =
      _files_cql_manager->get_file_by_id(school_id, file_id);

  if (cql_response1.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the file";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Replace the file's name if the name parameter is not empty
  if (file_name.has_value()) {
    file._name = file_name.value();
  }

  // Replace the file's visible_to_students if the visible_to_students parameter
  // is not empty
  if (visible_to_students.has_value()) {
    file._visible_to_students = visible_to_students.value();
  }

  // Replace the file's students_can_add if the students_can_add parameter is
  // not empty
  if (file._type == CustomFileType::FOLDER && students_can_add.has_value()) {
    file._students_can_add = students_can_add.value();
  }

  // Replace the file's file_ids if the file_ids parameter is not empty and the
  // file is a folder
  if (file._type == CustomFileType::FOLDER && file_ids.has_value()) {
    // Check if the number of files is the same
    if (file._files.size() != file_ids.value().size()) {
      json_response["error"] = "The number of files is not the same";
      return {drogon::HttpStatusCode::k400BadRequest, json_response};
    }

    // Check if the files are the same but in different order
    std::vector<bool> file_ids_copy;
    for (auto &file_id : file_ids.value()) {
      file_ids_copy.push_back(false);
    }

    for (auto &file_id : file._files) {
      bool found = false;
      for (int i = 0; i < file_ids.value().size(); i++) {
        if (file_id.clock_seq_and_node ==
                file_ids.value()[i].clock_seq_and_node &&
            file_id.time_and_version == file_ids.value()[i].time_and_version) {
          file_ids_copy[i] = true;
          found = true;
          break;
        }
      }

      if (!found) {
        json_response["error"] = "The files are not the same";
        return {drogon::HttpStatusCode::k400BadRequest, json_response};
      }
    }

    file._files = file_ids.value();
  }

  // Update the file
  auto cql_response2 = _files_cql_manager->update_file(
      school_id, file._id, file._type, file._name, file._files,
      file._path_to_file, file._size, file._added_by_user,
      file._visible_to_students, file._students_can_add);

  if (cql_response2.code() != ResultCode::OK) {
    json_response["error"] = "Could not update the file";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Return 200 OK
  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
CourseManager::delete_course_file(const int school_id,
                                  const std::string &user_token,
                                  const CassUuid course_id,
                                  const CassUuid file_id_to_delete) {
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check if user is a teacher or admin
  if (temp_user._user_type != UserType::TEACHER &&
      temp_user._user_type != UserType::ADMIN) {
    json_response["error"] = "User is not a teacher or admin";
    return {drogon::HttpStatusCode::k403Forbidden, json_response};
  }

  // If user is a teacher
  if (temp_user._user_type == UserType::TEACHER) {
    //  Get the course's users
    auto [cql_response0, course_users] =
        _users_by_course_cql_manager->get_users_by_course(school_id, course_id);

    if (cql_response0.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the course's users";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    //  Check if the teacher is in the course's users
    bool teacher_is_in_course = false;
    for (auto &user_id : course_users) {
      if (user_id.clock_seq_and_node == temp_user._user_id.clock_seq_and_node &&
          user_id.time_and_version == temp_user._user_id.time_and_version) {
        teacher_is_in_course = true;
        break;
      }
    }

    if (!teacher_is_in_course) {
      json_response["error"] = "Teacher is not in the course";
      return {drogon::HttpStatusCode::k403Forbidden, json_response};
    }
  }

  // Get the file
  auto [cql_response1, file] =
      _files_cql_manager->get_file_by_id(school_id, file_id_to_delete);

  if (cql_response1.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the file";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the file is in the course
  // Get the course
  auto [cql_response2, course] =
      _courses_cql_manager->get_course_by_id(school_id, course_id);

  if (cql_response2.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Could not get the course";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_response2.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the file is in the course
  bool file_is_in_course = false;
  for (auto &file_id : course._files) {
    if (file_id.clock_seq_and_node == file_id_to_delete.clock_seq_and_node &&
        file_id.time_and_version == file_id_to_delete.time_and_version) {
      file_is_in_course = true;
      break;
    }
  }

  if (!file_is_in_course) {  // The file in inside of a folder
    // Get the files inside the course
    for (auto &file_id : course._files) {
      // Get the file
      auto [cql_response3, course_file] =
          _files_cql_manager->get_file_by_id(school_id, file_id);

      if (cql_response3.code() != ResultCode::OK) {
        json_response["error"] = "Could not get the file";
        return {drogon::HttpStatusCode::k500InternalServerError, json_response};
      }

      if (course_file._type != CustomFileType::FOLDER) {
        continue;
      }

      // Check if the file is in the folder
      for (auto &folder_file : course_file._files) {
        if (file_id_to_delete.clock_seq_and_node ==
            folder_file.clock_seq_and_node) {
          file_is_in_course = true;
          // Delete the file from the folder by removing it from the folder's
          // files
          std::vector<CassUuid> files;
          for (auto &file_id : file._files) {
            if (file_id_to_delete.clock_seq_and_node !=
                    folder_file.clock_seq_and_node &&
                file_id_to_delete.time_and_version !=
                    folder_file.time_and_version) {
              files.push_back(file_id);
            }
          }

          auto cql_response4 = _files_cql_manager->update_file(
              school_id, course_file._id, course_file._type, course_file._name,
              files, course_file._path_to_file, course_file._size,
              course_file._added_by_user, course_file._visible_to_students,
              course_file._students_can_add);

          if (cql_response4.code() != ResultCode::OK) {
            json_response["error"] = "Could not delete the file";
            return {drogon::HttpStatusCode::k500InternalServerError,
                    json_response};
          }

          break;
        }
      }
    }
  } else {
    // Delete the file from the course by removing it from the course's files
    std::vector<CassUuid> files;
    for (auto &file_id : course._files) {
      if (file_id.clock_seq_and_node != file_id_to_delete.clock_seq_and_node &&
          file_id.time_and_version != file_id_to_delete.time_and_version) {
        files.push_back(file_id);
      }
    }

    auto cql_response4 = _courses_cql_manager->update_course(
        school_id, course._id, course._name, course._course_thumbnail,
        course._created_at, course._start_date, course._end_date, files);

    if (cql_response4.code() != ResultCode::OK) {
      json_response["error"] = "Could not delete the file";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }
  }

  if (!file_is_in_course) {
    json_response["error"] = "File is not in the course";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  }

  // Delete the file
  auto cql_response3 =
      _files_cql_manager->delete_file(school_id, file_id_to_delete);

  if (cql_response3.code() != ResultCode::OK) {
    json_response["error"] = "Could not delete the file";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Return 200 OK
  json_response["file_path"] = file._path_to_file;
  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
CourseManager::has_permission_to_get_file(const int school_id,
                                          const std::string &user_token,
                                          const CassUuid course_id,
                                          const CassUuid &file_id) {
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Get the user's courses
  auto [cql_response1, user_courses] =
      _courses_by_user_cql_manager->get_courses_by_user(school_id,
                                                        temp_user._user_id);

  if (cql_response1.code() == ResultCode::NOT_FOUND) {
    if (temp_user._user_type != UserType::ADMIN) {
      json_response["error"] = "User does not have permission to get the file";
      return {drogon::HttpStatusCode::k403Forbidden, json_response};
    }
  } else if (cql_response1.code() != ResultCode::OK) {
    json_response["error"] = "Internal server error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the course is in the users' courses
  bool course_is_in_user_courses = false;
  for (auto &course : user_courses) {
    if (course.clock_seq_and_node == course_id.clock_seq_and_node &&
        course.time_and_version == course_id.time_and_version) {
      course_is_in_user_courses = true;
      break;
    }
  }
  if (!course_is_in_user_courses && temp_user._user_type != UserType::ADMIN) {
    json_response["error"] = "User does not have permission to get the file";
    return {drogon::HttpStatusCode::k403Forbidden, json_response};
  }

  // Get the course
  auto [cql_response2, course] =
      _courses_cql_manager->get_course_by_id(school_id, course_id);

  if (cql_response2.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Internal server error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  } else if (cql_response2.code() != ResultCode::OK) {
    json_response["error"] = "Internal server error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the file exists
  auto [cql_response3, file] =
      _files_cql_manager->get_file_by_id(school_id, file_id);

  if (cql_response3.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "File does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_response3.code() != ResultCode::OK) {
    json_response["error"] = "Internal server error";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the file is not a folder
  if (file._type == CustomFileType::FOLDER) {
    json_response["error"] = "Cannot download a folder!";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  }

  // Check if the file is in the course's files
  bool file_is_in_course_files = false;
  std::string file_path;

  for (auto &temp_file_id : course._files) {
    // Get the file
    auto [cql_response4, temp_file] =
        _files_cql_manager->get_file_by_id(school_id, temp_file_id);

    if (cql_response4.code() != ResultCode::OK) {
      json_response["error"] = "Internal server error";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    if (temp_user._user_type == UserType::STUDENT &&
        temp_file._visible_to_students == false)
      continue;

    if (temp_file._type == CustomFileType::FILE) {
      if (temp_file._id.clock_seq_and_node == file_id.clock_seq_and_node &&
          temp_file._id.time_and_version == file_id.time_and_version) {
        file_is_in_course_files = true;
        file_path = temp_file._path_to_file;
        break;
      }
    } else if (temp_file._type == CustomFileType::FOLDER) {
      // Check if the file is in the folder

      for (auto &folder_file_id : temp_file._files) {
        auto [cql_response5, folder_file] =
            _files_cql_manager->get_file_by_id(school_id, folder_file_id);

        if (cql_response5.code() != ResultCode::OK) {
          json_response["error"] = "Internal server error";
          return {drogon::HttpStatusCode::k500InternalServerError,
                  json_response};
        }

        if (folder_file._id.clock_seq_and_node == file_id.clock_seq_and_node &&
            folder_file._id.time_and_version == file_id.time_and_version) {
          file_is_in_course_files = true;
          file_path = folder_file._path_to_file;
          break;
        }
      }
    }
  }

  if (!file_is_in_course_files) {
    json_response["error"] = "File does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  }

  json_response["file_path"] = file_path;
  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> CourseManager::add_users(
    const int school_id, const std::string &user_token,
    const CassUuid course_id, const std::vector<CassUuid> &users,
    const std::vector<CassUuid> &tags) {
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check if the course exists
  auto [cql_response0, course] =
      _courses_cql_manager->get_course_by_id(school_id, course_id);

  if (cql_response0.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Course does not exist";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_response0.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if user is a teacher or admin
  if (temp_user._user_type != UserType::TEACHER &&
      temp_user._user_type != UserType::ADMIN) {
    json_response["error"] = "User is not a teacher or admin";
    return {drogon::HttpStatusCode::k403Forbidden, json_response};
  }

  // If user is a teacher check if he is in the course's users
  if (temp_user._user_type == UserType::TEACHER) {
    //  Get the course's users
    auto [cql_response1, course_users] =
        _users_by_course_cql_manager->get_users_by_course(school_id, course_id);

    if (cql_response1.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the course's users";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    //  Check if the teacher is in the course's users
    bool teacher_is_in_course = false;
    for (auto &user_id : course_users) {
      if (user_id.clock_seq_and_node == temp_user._user_id.clock_seq_and_node &&
          user_id.time_and_version == temp_user._user_id.time_and_version) {
        teacher_is_in_course = true;
        break;
      }
    }

    if (!teacher_is_in_course) {
      json_response["error"] = "Teacher is not in the course";
      return {drogon::HttpStatusCode::k403Forbidden, json_response};
    }
  }

  // Check if the users that we want to add exist
  for (auto &user_id : users) {
    auto [cql_response2, user] =
        _users_cql_manager->get_user(school_id, user_id);

    if (cql_response2.code() == ResultCode::NOT_FOUND) {
      json_response["error"] = "User does not exist";
      return {drogon::HttpStatusCode::k404NotFound, json_response};
    } else if (cql_response2.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the user";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }
  }

  // Check if the tags that we want to add exist
  for (auto &tag_id : tags) {
    auto [cql_response3, tag] =
        _tags_cql_manager->get_tag_by_id(school_id, tag_id);

    if (cql_response3.code() == ResultCode::NOT_FOUND) {
      json_response["error"] = "Tag does not exist";
      return {drogon::HttpStatusCode::k404NotFound, json_response};
    } else if (cql_response3.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the tag";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }
  }

  // For each user in the list:
  //   Add the list to the users by courses
  //   Add the list to the courses by users
  for (auto &user_id : users) {
    // Add the list to the users by courses
    auto cql_response4 = _users_by_course_cql_manager->create_relationship(
        school_id, course_id, user_id);

    if (cql_response4.code() != ResultCode::OK &&
        cql_response4.code() != ResultCode::NOT_APPLIED) {
      json_response["error"] = "Could not add the user to the course";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    // Add the list to the courses by users
    auto cql_response5 = _courses_by_user_cql_manager->create_relationship(
        school_id, user_id, course_id);

    if (cql_response5.code() != ResultCode::OK &&
        cql_response5.code() != ResultCode::NOT_APPLIED) {
      json_response["error"] = "Could not add the course to the user";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }
  }

  // For each tag in the list:
  //   Get the users by tag
  //   For each user in the list:
  //     Add the list to the users by courses
  //     Add the list to the courses by users
  for (auto &tag_id : tags) {
    auto [cql_response6, users_by_tag] =
        _users_by_tag_cql_manager->get_users_by_tag(school_id, tag_id);

    if (cql_response6.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the users by tag";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    for (auto &user_id : users_by_tag) {
      // Add the list to the users by courses
      auto cql_response7 = _users_by_course_cql_manager->create_relationship(
          school_id, course_id, user_id);

      if (cql_response7.code() != ResultCode::OK &&
          cql_response7.code() != ResultCode::NOT_APPLIED) {
        json_response["error"] = "Could not add the user to the course";
        return {drogon::HttpStatusCode::k500InternalServerError, json_response};
      }

      // Add the list to the courses by users
      auto cql_response8 = _courses_by_user_cql_manager->create_relationship(
          school_id, user_id, course_id);

      if (cql_response8.code() != ResultCode::OK ||
          cql_response8.code() != ResultCode::NOT_APPLIED) {
        json_response["error"] = "Could not add the course to the user";
        return {drogon::HttpStatusCode::k500InternalServerError, json_response};
      }
    }
  }

  // Return 200 OK
  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> CourseManager::remove_users(
    const int school_id, const std::string &user_token,
    const CassUuid course_id, const std::vector<CassUuid> &users,
    const std::vector<CassUuid> &tags) {
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check if user is a teacher or admin
  if (temp_user._user_type != UserType::TEACHER &&
      temp_user._user_type != UserType::ADMIN) {
    json_response["error"] = "User is not a teacher or admin";
    return {drogon::HttpStatusCode::k403Forbidden, json_response};
  }

  // If user is a teacher
  if (temp_user._user_type == UserType::TEACHER) {
    //  Get the course's users
    auto [cql_response0, course_users] =
        _users_by_course_cql_manager->get_users_by_course(school_id, course_id);

    if (cql_response0.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the course's users";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    //  Check if the teacher is in the course's users
    bool teacher_is_in_course = false;
    for (auto &user_id : course_users) {
      if (user_id.clock_seq_and_node == temp_user._user_id.clock_seq_and_node &&
          user_id.time_and_version == temp_user._user_id.time_and_version) {
        teacher_is_in_course = true;
        break;
      }
    }

    if (!teacher_is_in_course) {
      json_response["error"] = "Teacher is not in the course";
      return {drogon::HttpStatusCode::k403Forbidden, json_response};
    }
  }

  // Check if the users exist
  for (auto &user_id : users) {
    auto [cql_response1, user] =
        _users_cql_manager->get_user(school_id, user_id);

    if (cql_response1.code() == ResultCode::NOT_FOUND) {
      json_response["error"] = "User does not exist";
      return {drogon::HttpStatusCode::k404NotFound, json_response};
    } else if (cql_response1.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the user";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }
  }

  // Check if the tags exist
  for (auto &tag_id : tags) {
    auto [cql_response2, tag] =
        _tags_cql_manager->get_tag_by_id(school_id, tag_id);

    if (cql_response2.code() == ResultCode::NOT_FOUND) {
      json_response["error"] = "Tag does not exist";
      return {drogon::HttpStatusCode::k404NotFound, json_response};
    } else if (cql_response2.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the tag";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }
  }

  // For each user in the list
  // Remove the list from the users by courses
  // Remove the list from the courses by users
  for (auto &user_id : users) {
    // Remove the list to the users by courses
    auto cql_response3 = _users_by_course_cql_manager->delete_relationship(
        school_id, course_id, user_id);

    if (cql_response3.code() != ResultCode::OK &&
        cql_response3.code() != ResultCode::NOT_APPLIED) {
      json_response["error"] = "Could not remove the user from the course";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    // Remove the list to the courses by users
    auto cql_response4 = _courses_by_user_cql_manager->delete_relationship(
        school_id, user_id, course_id);

    if (cql_response4.code() != ResultCode::OK &&
        cql_response4.code() != ResultCode::NOT_APPLIED) {
      json_response["error"] = "Could not remove the course from the user";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    auto [http_code, json_error] =
        delete_questions_and_answers_of_user(school_id, course_id, user_id);

    if (http_code != drogon::HttpStatusCode::k200OK) {
      return {http_code, json_error};
    }
  }

  // For each tag in the list
  // Get the users by tag
  // For each user in the list
  // Remove the list from the users by courses
  // Remove the list from the courses by users
  for (auto &tag_id : tags) {
    auto [cql_response5, users_by_tag] =
        _users_by_tag_cql_manager->get_users_by_tag(school_id, tag_id);

    if (cql_response5.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the users by tag";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    for (auto &user_id : users_by_tag) {
      // Remove the list from the users by courses
      auto cql_response6 = _users_by_course_cql_manager->delete_relationship(
          school_id, course_id, user_id);

      if (cql_response6.code() != ResultCode::OK &&
          cql_response6.code() != ResultCode::NOT_APPLIED) {
        json_response["error"] = "Could not remove the user from the course";
        return {drogon::HttpStatusCode::k500InternalServerError, json_response};
      }

      // Remove the list from the courses by users
      auto cql_response7 = _courses_by_user_cql_manager->delete_relationship(
          school_id, user_id, course_id);

      if (cql_response7.code() != ResultCode::OK ||
          cql_response7.code() != ResultCode::NOT_APPLIED) {
        json_response["error"] = "Could not remove the course from the user";
        return {drogon::HttpStatusCode::k500InternalServerError, json_response};
      }

      auto [http_code, json_error] =
          delete_questions_and_answers_of_user(school_id, course_id, user_id);

      if (http_code != drogon::HttpStatusCode::k200OK) {
        return {http_code, json_error};
      }
    }
  }

  // Return 200 OK
  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> CourseManager::create_question(
    const int school_id, const std::string &user_token,
    const CassUuid course_id, std::string &content) {
  // Get the user by token
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Anyone can create a question

  if (temp_user._user_type != UserType::ADMIN) {
    // Check if the user is in the course
    auto [cql_response1, users_by_course] =
        _users_by_course_cql_manager->get_users_by_course(school_id, course_id);

    if (cql_response1.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the users by course";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    bool user_in_course = false;
    for (auto &user : users_by_course) {
      if (user.clock_seq_and_node == temp_user._user_id.clock_seq_and_node &&
          user.time_and_version == temp_user._user_id.time_and_version) {
        user_in_course = true;
        break;
      }
    }

    if (!user_in_course) {
      json_response["error"] = "User is not in the course";
      return {drogon::HttpStatusCode::k403Forbidden, json_response};
    }
  }

  CassUuid question_id = create_current_uuid();
  QuestionObject new_question(school_id, question_id, content, time(nullptr),
                              temp_user._user_id);

  // Create the question
  auto cql_response2 = _questions_cql_manager->create_question(new_question);

  if (cql_response2.code() != ResultCode::OK) {
    json_response["error"] = "Could not create the question";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Add the question to the course
  auto cql_response3 = _questions_by_course_cql_manager->create_relationship(
      school_id, course_id, question_id);

  if (cql_response3.code() != ResultCode::OK) {
    json_response["error"] = "Could not add the question to the course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  json_response = new_question.to_json(true);
  json_response["created_by_user_name"] =
      temp_user._first_name + " " + temp_user._last_name;
  json_response["answers"] = Json::arrayValue;
  return {drogon::HttpStatusCode::k201Created, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
CourseManager::get_questions_by_course(const int school_id,
                                       const std::string &user_token,
                                       const CassUuid course_id) {
  // Get the user by token
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Check if the user is in the course
  auto [cql_response1, users_by_course] =
      _users_by_course_cql_manager->get_users_by_course(school_id, course_id);

  if (cql_response1.code() != ResultCode::OK &&
      cql_response1.code() != ResultCode::NOT_FOUND) {
    json_response["error"] = "Could not get the users by course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  bool user_in_course = false;
  for (auto &user : users_by_course) {
    if (user.clock_seq_and_node == temp_user._user_id.clock_seq_and_node &&
        user.time_and_version == temp_user._user_id.time_and_version) {
      user_in_course = true;
      break;
    }
  }

  if (!user_in_course && temp_user._user_type != UserType::ADMIN) {
    json_response["error"] = "User is not in the course";
    return {drogon::HttpStatusCode::k403Forbidden, json_response};
  }

  // Get the questions by course
  auto [cql_response2, questions_by_course] =
      _questions_by_course_cql_manager->get_questions_by_course(school_id,
                                                                course_id);

  if (cql_response2.code() != ResultCode::OK &&
      cql_response2.code() != ResultCode::NOT_FOUND) {
    json_response["error"] = "Could not get the questions by course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  if (cql_response2.code() == ResultCode::NOT_FOUND) {
    Json::Value json_questions = Json::arrayValue;
    return {drogon::HttpStatusCode::k200OK, json_questions};
  }

  // Get the questions
  std::vector<QuestionObject> questions;
  for (auto &question_id : questions_by_course) {
    auto [cql_response3, question_object] =
        _questions_cql_manager->get_question_by_id(school_id, question_id);

    if (cql_response3.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the question";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    questions.push_back(question_object);
  }

  // Get the answers
  for (auto &question : questions) {
    Json::Value json_question;

    json_question["id"] = get_uuid_string(question._question_id);
    json_question["content"] = question._text;
    json_question["created_at"] = (Json::Value::Int64)question._time_added;

    // Get the user
    auto [cql_response4, user_object] =
        _users_cql_manager->get_user(school_id, question._added_by_user_id);

    if (cql_response4.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the user";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    json_question["created_by_user_name"] =
        user_object._first_name + " " + user_object._last_name;
    json_question["created_by_user_id"] = get_uuid_string(user_object._user_id);

    // Get the answers
    auto [http_response, json_answers] =
        this->get_answers(school_id, course_id, question._question_id);

    if (http_response != drogon::HttpStatusCode::k200OK) {
      return {http_response, json_answers};
    }

    if (json_answers.size() == 0) {
      json_question["answers"] = Json::Value(Json::arrayValue);
    } else {
      json_question["answers"] = json_answers;
    }
    json_response.append(json_question);
  }

  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> CourseManager::delete_question(
    const int school_id, const std::string &user_token,
    const CassUuid course_id, const CassUuid question_id) {
  // Get the user by token
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Get the question
  auto [cql_response1, question_object] =
      _questions_cql_manager->get_question_by_id(school_id, question_id);

  if (cql_response1.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the question";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the question is in the course
  auto [cql_response2, questions_by_course] =
      _questions_by_course_cql_manager->get_questions_by_course(school_id,
                                                                course_id);

  if (cql_response2.code() != ResultCode::OK &&
      cql_response2.code() != ResultCode::NOT_FOUND) {
    json_response["error"] = "Could not get the questions by course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  bool question_in_course = false;
  for (auto &question : questions_by_course) {
    if (question.clock_seq_and_node == question_id.clock_seq_and_node &&
        question.time_and_version == question_id.time_and_version) {
      question_in_course = true;
      break;
    }
  }

  if (!question_in_course) {
    json_response["error"] = "Question is not in the course";
    return {drogon::HttpStatusCode::k403Forbidden, json_response};
  }

  // Check if the user is the creator of the question
  if (question_object._added_by_user_id.clock_seq_and_node !=
          temp_user._user_id.clock_seq_and_node ||
      question_object._added_by_user_id.time_and_version !=
          temp_user._user_id.time_and_version) {
    if (temp_user._user_type != UserType::ADMIN) {
      json_response["error"] = "User is not the creator of the question";
      return {drogon::HttpStatusCode::k403Forbidden, json_response};
    }
  }

  // Delete the question's answers
  auto [cql_response3, answers_by_question] =
      _answers_by_announcement_or_question_cql_manager
          ->get_answers_by_announcement_or_question(school_id, question_id, 1);

  if (cql_response3.code() != ResultCode::OK &&
      cql_response3.code() != ResultCode::NOT_FOUND) {
    json_response["error"] = "Could not get the answers by question";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  for (auto &answer : answers_by_question) {
    auto [cql_response4, answer_object] =
        _answers_cql_manager->get_answer_by_id(school_id, answer);

    if (cql_response4.code() == ResultCode::NOT_FOUND) {
      continue;
    }

    if (cql_response4.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the answer";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    auto cql_response5 = _answers_cql_manager->delete_answer(
        school_id, answer_object._id, answer_object._created_at);

    if (cql_response5.code() != ResultCode::OK) {
      json_response["error"] = "Could not delete the answer";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }
  }

  // Delete the question
  auto cql_response6 =
      _questions_cql_manager->delete_question(school_id, question_id);

  if (cql_response6.code() != ResultCode::OK) {
    json_response["error"] = "Could not delete the question";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  auto cql_response7 = _answers_by_announcement_or_question_cql_manager
                           ->delete_relationships_by_announcement_or_question(
                               school_id, question_id, 1);

  if (cql_response7.code() != ResultCode::OK) {
    json_response["error"] = "Could not delete the answers by question";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  auto cql_response8 = _questions_by_course_cql_manager->delete_relationship(
      school_id, course_id, question_id);

  if (cql_response8.code() != ResultCode::OK) {
    json_response["error"] = "Could not delete the question by course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> CourseManager::create_answer(
    const int school_id, const std::string &user_token,
    const CassUuid course_id, const CassUuid question_id,
    const std::string &content) {
  // Get the user by token
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Get the question
  auto [cql_response1, question_object] =
      _questions_cql_manager->get_question_by_id(school_id, question_id);

  if (cql_response1.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the question";
    return {drogon::HttpStatusCode::k401Unauthorized, json_response};
  }

  if (temp_user._user_type != UserType::ADMIN) {
    // Check if the user is in the course
    auto [cql_response2, course_users] =
        _users_by_course_cql_manager->get_users_by_course(school_id, course_id);

    if (cql_response2.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the course";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    bool is_in_course = false;
    for (auto &user_id : course_users) {
      if (user_id.clock_seq_and_node == temp_user._user_id.clock_seq_and_node &&
          user_id.time_and_version == temp_user._user_id.time_and_version) {
        is_in_course = true;
        break;
      }
    }

    if (!is_in_course) {
      json_response["error"] = "User is not in the course";
      return {drogon::HttpStatusCode::k403Forbidden, json_response};
    }
  }

  // Add the answer
  CassUuid answer_id = create_current_uuid();
  AnswerObject answer_object(school_id, answer_id, time(nullptr),
                             temp_user._user_id, content);

  auto cql_response3 = _answers_cql_manager->create_answer(answer_object);

  if (cql_response3.code() != ResultCode::OK) {
    json_response["error"] = "Could not add the answer";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Add the answer to the question
  auto cql_response4 =
      _answers_by_announcement_or_question_cql_manager->create_relationship(
          school_id, question_id, 1, answer_id);

  if (cql_response4.code() != ResultCode::OK) {
    json_response["error"] = "Could not add the answer to the question";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Return 201 Created
  json_response = answer_object.to_json(true);
  json_response["created_by_user_name"] =
      temp_user._first_name + " " + temp_user._last_name;
  json_response["created_by_user_id"] = get_uuid_string(temp_user._user_id);
  json_response.removeMember("created_by");
  json_response["question_id"] = get_uuid_string(question_id);
  return {drogon::HttpStatusCode::k201Created, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> CourseManager::get_answers(
    const int school_id, const CassUuid course_id, const CassUuid question_id) {
  Json::Value json_response;

  // Get the answers
  auto [cql_response1, answers_by_question] =
      _answers_by_announcement_or_question_cql_manager
          ->get_answers_by_announcement_or_question(school_id, question_id, 1);

  if (cql_response1.code() != ResultCode::OK &&
      cql_response1.code() != ResultCode::NOT_FOUND) {
    json_response["error"] = "Could not get the answers by question";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Get the answers
  std::vector<AnswerObject> answers;
  for (auto &answer_id : answers_by_question) {
    auto [cql_response2, answer_object] =
        _answers_cql_manager->get_answer_by_id(school_id, answer_id);

    if (cql_response2.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the answer";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    answers.push_back(answer_object);
  }

  // Get the users' name that created the answers
  for (auto &answer : answers) {
    auto [cql_response3, user_object] =
        _users_cql_manager->get_user(school_id, answer._created_by);

    if (cql_response3.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the user";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    Json::Value answer_json;

    answer_json["created_at"] = (Json::Value::Int64)answer._created_at;
    answer_json["content"] = answer._content;
    answer_json["created_by_user_id"] = get_uuid_string(answer._created_by);
    answer_json["created_by_user_name"] =
        user_object._first_name + " " + user_object._last_name;
    answer_json["id"] = get_uuid_string(answer._id);
    answer_json["question_id"] = get_uuid_string(question_id);

    json_response.append(answer_json);
  }

  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> CourseManager::delete_answer(
    const int school_id, const std::string &user_token,
    const CassUuid question_id, const CassUuid answer_id) {
  // Get the user by token
  UserObject temp_user;

  // Get user by token
  auto [status_code, json_response] =
      get_user_by_token(school_id, user_token, temp_user);

  if (status_code != drogon::HttpStatusCode::k200OK) {
    return {status_code, json_response};
  }

  // Get the answer
  auto [cql_response1, answer_object] =
      _answers_cql_manager->get_answer_by_id(school_id, answer_id);

  if (cql_response1.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the answer";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  }

  // Check if the user is the creator of the answer or an admin
  if (temp_user._user_id.clock_seq_and_node !=
          answer_object._created_by.clock_seq_and_node ||
      temp_user._user_id.time_and_version !=
          answer_object._created_by.time_and_version) {
    if (temp_user._user_type != UserType::ADMIN) {
      json_response["error"] =
          "User is not the creator of the answer or an admin";
      return {drogon::HttpStatusCode::k403Forbidden, json_response};
    }
  }

  // Delete the answer
  auto cql_response2 = _answers_cql_manager->delete_answer(
      school_id, answer_id, answer_object._created_at);

  if (cql_response2.code() != ResultCode::OK &&
      cql_response2.code() != ResultCode::NOT_FOUND) {
    json_response["error"] = "Could not delete the answer";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Delete the answer from the question
  auto cql_response3 =
      _answers_by_announcement_or_question_cql_manager->delete_relationship(
          school_id, question_id, 1, answer_id);

  if (cql_response3.code() != ResultCode::OK &&
      cql_response3.code() != ResultCode::NOT_FOUND) {
    json_response["error"] = "Could not delete the answer from the question";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value>
CourseManager::delete_questions_and_answers_of_user(const int school_id,
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
