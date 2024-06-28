#include "relations_managers/grade_manager.hpp"

std::pair<drogon::HttpStatusCode, Json::Value> GradeManager::get_user_by_token(
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

std::pair<drogon::HttpStatusCode, Json::Value> GradeManager::add_grade(
    int school_id, const std::string &creator_token, const CassUuid &course_id,
    const CassUuid &user_id, const int grade, std::optional<int> out_of,
    std::optional<float> weight) {
  UserObject creator;

  auto [status, json_response] =
      get_user_by_token(school_id, creator_token, creator);

  if (status != drogon::HttpStatusCode::k200OK) {
    return {status, json_response};
  }

  // Check if the user is a teacher or admin
  if (creator._user_type != UserType::TEACHER &&
      creator._user_type != UserType::ADMIN) {
    json_response["error"] = "User is not a teacher or admin";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  }

  // Check if the course exists
  auto [cql_result0, course] =
      _courses_cql_manager->get_course_by_id(school_id, course_id);

  if (cql_result0.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Course does not exist";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  } else if (cql_result0.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  auto [cql_result1, course_users] =
      _users_by_course_cql_manager->get_users_by_course(school_id, course_id);

  if (cql_result1.code() != ResultCode::OK &&
      cql_result1.code() != ResultCode::NOT_FOUND) {
    json_response["error"] = "Could not get the users by course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the user is a teacher of the course
  if (creator._user_type == UserType::TEACHER) {
    bool is_teacher = false;
    for (const auto &course_user : course_users) {
      if (course_user.clock_seq_and_node ==
              creator._user_id.clock_seq_and_node &&
          course_user.time_and_version == creator._user_id.time_and_version) {
        is_teacher = true;
        break;
      }
    }

    if (!is_teacher) {
      json_response["error"] = "User is not a teacher of the course";
      return {drogon::HttpStatusCode::k400BadRequest, json_response};
    }
  }

  // Check if the user is a student of the course
  bool is_student = false;
  for (const auto &course_user : course_users) {
    if (course_user.clock_seq_and_node == user_id.clock_seq_and_node &&
        course_user.time_and_version == user_id.time_and_version) {
      is_student = true;
      break;
    }
  }

  if (!is_student) {
    json_response["error"] = "User is not a student of the course";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  }

  // Check if the grade is valid
  if (grade < 0) {
    json_response["error"] = "Grade is not valid";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  }

  // Check if the out of is valid
  if (out_of.has_value() && out_of.value() < 0) {
    json_response["error"] = "Out of is not valid";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  }

  // Check if the weight is valid
  if (weight.has_value() && weight.value() < 0) {
    json_response["error"] = "Weight is not valid";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  }

  if (out_of.has_value() && weight.has_value()) {
    if (grade > out_of.value()) {
      json_response["error"] = "Grade is greater than out of";
      return {drogon::HttpStatusCode::k400BadRequest, json_response};
    }
  }

  if (weight.has_value() && weight.value() > 1) {
    json_response["error"] = "Weight is greater than 1";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  }

  // Set default values
  if (!out_of.has_value()) {
    out_of.emplace(grade);
  }

  if (!weight.has_value()) {
    weight.emplace(-1.f);
  }

  // Add grade
  CassUuid grade_id = create_current_uuid();
  GradeObject grade_object(school_id, grade_id, user_id, creator._user_id,
                           course_id, grade, out_of.value(), time(nullptr),
                           weight.value());

  auto cql_result2 = _grades_cql_manager->create_grade(grade_object);

  if (cql_result2.code() != ResultCode::OK) {
    json_response["error"] = "Could not create the grade";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  Json::Value json_grade_object = grade_object.to_json(true);
  json_grade_object["course_name"] = course._name;

  return {drogon::HttpStatusCode::k201Created, json_grade_object};
}

std::pair<drogon::HttpStatusCode, Json::Value>
GradeManager::get_personal_grades(const int school_id,
                                  const std::string &token) {
  UserObject user;

  auto [status, json_response] = get_user_by_token(school_id, token, user);

  if (status != drogon::HttpStatusCode::k200OK) {
    return {status, json_response};
  }

  auto [cql_result0, user_courses] =
      _courses_by_user_cql_manager->get_courses_by_user(school_id,
                                                        user._user_id);

  if (cql_result0.code() != ResultCode::OK &&
      cql_result0.code() != ResultCode::NOT_FOUND) {
    json_response["error"] = "Could not get the users by course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  Json::Value json_grades(Json::arrayValue);

  for (const auto &user_course : user_courses) {
    auto [cql_result1, course_grades] =
        _grades_cql_manager->get_grades_by_user_and_course(
            school_id, user._user_id, user_course);

    if (cql_result1.code() != ResultCode::OK &&
        cql_result1.code() != ResultCode::NOT_FOUND) {
      json_response["error"] = "Could not get the grades by course";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    // Read the course
    auto [cql_result2, course] =
        _courses_cql_manager->get_course_by_id(school_id, user_course);

    if (cql_result2.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the course";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    Json::Value json_course = course.to_json(true);

    Json::Value json_course_grades(Json::arrayValue);
    for (auto &course_grade : course_grades) {
      Json::Value json_course_grade = course_grade.to_json(true);

      // Get the evaluator name
      auto [cql_result3, evaluator] =
          _users_cql_manager->get_user(school_id, course_grade._evaluator_id);

      if (cql_result3.code() != ResultCode::OK) {
        json_response["error"] = "Could not get the evaluator";
        return {drogon::HttpStatusCode::k500InternalServerError, json_response};
      }

      json_course_grade["evaluator_name"] =
          evaluator._first_name + " " + evaluator._last_name;

      json_course_grade["evaluated_name"] =
          user._first_name + " " + user._last_name;

      json_course_grades.append(json_course_grade);
    }

    json_course["grades"] = json_course_grades;

    json_grades.append(json_course);
  }

  return {drogon::HttpStatusCode::k200OK, json_grades};
}

std::pair<drogon::HttpStatusCode, Json::Value> GradeManager::get_user_grades(
    const int school_id, const std::string &token, const CassUuid &user_id) {
  UserObject temp_user;

  auto [status, json_response] = get_user_by_token(school_id, token, temp_user);

  if (status != drogon::HttpStatusCode::k200OK) {
    return {status, json_response};
  }

  // Check if the user is a teacher or an admin or the user is the same as the
  // user_id
  if (temp_user._user_type != UserType::TEACHER &&
      temp_user._user_type != UserType::ADMIN &&
      temp_user._user_id.clock_seq_and_node != user_id.clock_seq_and_node) {
    json_response["error"] = "User is not a teacher or admin";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  }

  auto [cql_result0, user_courses] =
      _courses_by_user_cql_manager->get_courses_by_user(school_id, user_id);

  if (cql_result0.code() != ResultCode::OK &&
      cql_result0.code() != ResultCode::NOT_FOUND) {
    json_response["error"] = "Could not get the users by course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  Json::Value json_grades(Json::arrayValue);

  for (const auto &user_course : user_courses) {
    auto [cql_result1, course_grades] =
        _grades_cql_manager->get_grades_by_user_and_course(school_id, user_id,
                                                           user_course);

    if (cql_result1.code() != ResultCode::OK &&
        cql_result1.code() != ResultCode::NOT_FOUND) {
      json_response["error"] = "Could not get the grades by course";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    // Read the course
    auto [cql_result2, course] =
        _courses_cql_manager->get_course_by_id(school_id, user_course);

    if (cql_result2.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the course";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    Json::Value json_course = course.to_json(true);

    Json::Value json_course_grades(Json::arrayValue);
    for (auto &course_grade : course_grades) {
      json_course_grades.append(course_grade.to_json(true));
    }

    json_course["grades"] = json_course_grades;

    json_grades.append(json_course);
  }

  return {drogon::HttpStatusCode::k200OK, json_grades};
}

std::pair<drogon::HttpStatusCode, Json::Value> GradeManager::get_course_grades(
    const int school_id, const std::string &token, const CassUuid &course_id) {
  UserObject temp_user;

  auto [status, json_response] = get_user_by_token(school_id, token, temp_user);

  if (status != drogon::HttpStatusCode::k200OK) {
    return {status, json_response};
  }

  // Check if the user is a teacher or an admin
  if (temp_user._user_type != UserType::TEACHER &&
      temp_user._user_type != UserType::ADMIN) {
    json_response["error"] = "User is not a teacher or admin";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  }

  auto [cql_result0, course_grades] =
      _grades_cql_manager->get_grades_by_course_id(school_id, course_id);

  if (cql_result0.code() != ResultCode::OK &&
      cql_result0.code() != ResultCode::NOT_FOUND) {
    json_response["error"] = "Could not get the grades by course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Read the course
  auto [cql_result1, course] =
      _courses_cql_manager->get_course_by_id(school_id, course_id);

  if (cql_result1.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the course";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  Json::Value json_course = course.to_json(true);

  for (auto &course_grade : course_grades) {
    Json::Value json_grade = course_grade.to_json(true);

    // Read the evaluated user
    auto [cql_result2, evaluated] =
        _users_cql_manager->get_user(school_id, course_grade._evaluated_id);

    if (cql_result2.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the evaluated user";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    json_grade["evaluated_name"] =
        evaluated._first_name + " " + evaluated._last_name;

    // Read the evaluator user
    auto [cql_result3, evaluator] =
        _users_cql_manager->get_user(school_id, course_grade._evaluator_id);

    if (cql_result3.code() != ResultCode::OK) {
      json_response["error"] = "Could not get the evaluator user";
      return {drogon::HttpStatusCode::k500InternalServerError, json_response};
    }

    json_grade["evaluator_name"] =
        evaluator._first_name + " " + evaluator._last_name;

    json_course["grades"].append(json_grade);
  }

  return {drogon::HttpStatusCode::k200OK, json_course};
}

std::pair<drogon::HttpStatusCode, Json::Value> GradeManager::edit_grade(
    const int school_id, const std::string &creator_token,
    const CassUuid &grade_id, const int value, std::optional<int> out_of,
    std::optional<int> weight) {
  UserObject temp_user;

  auto [status, json_response] =
      get_user_by_token(school_id, creator_token, temp_user);

  if (status != drogon::HttpStatusCode::k200OK) {
    return {status, json_response};
  }

  // Check if the user is a teacher or an admin
  if (temp_user._user_type != UserType::TEACHER &&
      temp_user._user_type != UserType::ADMIN) {
    json_response["error"] = "User is not a teacher or admin";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  }

  // Read the grade
  auto [cql_result0, grade] =
      _grades_cql_manager->get_grade_by_id(school_id, grade_id);

  if (cql_result0.code() == ResultCode::NOT_FOUND) {
    json_response["error"] = "Could not get the grade";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_result0.code() != ResultCode::OK) {
    json_response["error"] = "Could not get the grade";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  // Check if the grade is valid
  if (value < 0) {
    json_response["error"] = "Grade is not valid";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  }

  // Check if the out of is valid
  if (out_of.has_value() && out_of.value() < 0) {
    json_response["error"] = "Out of is not valid";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  }

  // Check if the weight is valid
  if (weight.has_value() && weight.value() < 0) {
    json_response["error"] = "Weight is not valid";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  }

  if (out_of.has_value() && weight.has_value()) {
    if (value > out_of.value()) {
      json_response["error"] = "Grade is greater than out of";
      return {drogon::HttpStatusCode::k400BadRequest, json_response};
    }
  }

  if (weight.has_value() && weight.value() > 1) {
    json_response["error"] = "Weight is greater than 1";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  }

  // Update the grade
  auto cql_result1 = _grades_cql_manager->update_grade(
      school_id, grade_id, value, grade._out_of, time(nullptr), grade._weight);

  if (cql_result1.code() != ResultCode::OK) {
    json_response["error"] = "Could not update the grade";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  return {drogon::HttpStatusCode::k200OK, json_response};
}

std::pair<drogon::HttpStatusCode, Json::Value> GradeManager::delete_grade(
    const int school_id, const std::string &creator_token,
    const CassUuid &grade_id) {
  UserObject temp_user;

  auto [status, json_response] =
      get_user_by_token(school_id, creator_token, temp_user);

  if (status != drogon::HttpStatusCode::k200OK) {
    return {status, json_response};
  }

  // Check if the user is a teacher or an admin
  if (temp_user._user_type != UserType::TEACHER &&
      temp_user._user_type != UserType::ADMIN) {
    json_response["error"] = "User is not a teacher or admin";
    return {drogon::HttpStatusCode::k400BadRequest, json_response};
  }

  // Delete the grade
  auto cql_result0 = _grades_cql_manager->delete_grade(school_id, grade_id);

  if (cql_result0.code() == ResultCode::NOT_APPLIED) {
    json_response["error"] = "Could not delete the grade";
    return {drogon::HttpStatusCode::k404NotFound, json_response};
  } else if (cql_result0.code() != ResultCode::OK) {
    json_response["error"] = "Could not delete the grade";
    return {drogon::HttpStatusCode::k500InternalServerError, json_response};
  }

  return {drogon::HttpStatusCode::k200OK, json_response};
}
