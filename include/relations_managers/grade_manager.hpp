/**
 * @file grade_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief The purpose of this class is to manage the grades relations
 * in the database.
 * @version 0.1
 * @date 2023-04-08
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef GRADES_MANAGER_H
#define GRADES_MANAGER_H

#include "../cql_helpers/courses_by_user_cql_manager.hpp"
#include "../cql_helpers/courses_cql_manager.hpp"
#include "../cql_helpers/grades_cql_manager.hpp"
#include "../cql_helpers/tokens_cql_manager.hpp"
#include "../cql_helpers/users_by_course_cql_manager.hpp"
#include "../cql_helpers/users_cql_manager.hpp"
#include "drogon/drogon.h"

class GradeManager {
 public:
  GradeManager(GradesCqlManager *gradesCqlManager,
               UsersCqlManager *usersCqlManager,
               UsersByCourseCqlManager *usersByCourseCqlManager,
               CoursesByUserCqlManager *coursesByUserCqlManager,
               TokensCqlManager *tokensCqlManager,
               CoursesCqlManager *coursesCqlManager)
      : _grades_cql_manager(gradesCqlManager),
        _users_cql_manager(usersCqlManager),
        _users_by_course_cql_manager(usersByCourseCqlManager),
        _courses_by_user_cql_manager(coursesByUserCqlManager),
        _tokens_cql_manager(tokensCqlManager),
        _courses_cql_manager(coursesCqlManager) {}
  ~GradeManager() {}

  std::pair<drogon::HttpStatusCode, Json::Value> add_grade(
      int school_id, const std::string &creator_token,
      const CassUuid &course_id, const CassUuid &user_id, const int grade,
      std::optional<int> out_of, std::optional<float> weight);

  std::pair<drogon::HttpStatusCode, Json::Value> get_personal_grades(
      const int school_id, const std::string &token);

  std::pair<drogon::HttpStatusCode, Json::Value> get_user_grades(
      const int school_id, const std::string &token, const CassUuid &user_id);

  std::pair<drogon::HttpStatusCode, Json::Value> get_course_grades(
      const int school_id, const std::string &token, const CassUuid &course_id);

  std::pair<drogon::HttpStatusCode, Json::Value> edit_grade(
      const int school_id, const std::string &creator_token,
      const CassUuid &grade_id, const int value, std::optional<int> out_of,
      std::optional<int> weight);

  std::pair<drogon::HttpStatusCode, Json::Value> delete_grade(
      const int school_id, const std::string &creator_token,
      const CassUuid &grade_id);

 private:
  GradesCqlManager *_grades_cql_manager;
  UsersCqlManager *_users_cql_manager;
  UsersByCourseCqlManager *_users_by_course_cql_manager;
  CoursesByUserCqlManager *_courses_by_user_cql_manager;
  TokensCqlManager *_tokens_cql_manager;
  CoursesCqlManager *_courses_cql_manager;

  /**
   * @brief Get the user by token string
   *
   * @param school_id  - The id of the school.
   * @param user_token - The token of the user.
   * @param user       - The user object that we want to fill.
   * @return std::pair<drogon::HttpStatusCode, Json::Value>
   */
  std::pair<drogon::HttpStatusCode, Json::Value> get_user_by_token(
      const int school_id, const std::string &user_token, UserObject &user);
};

#endif