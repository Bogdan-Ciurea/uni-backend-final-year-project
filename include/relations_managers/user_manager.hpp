/**
 * @file user_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief The purpose of this class is to manage the users and the relations
 * between them and the tokens.
 * @version 0.1
 * @date 2023-01-26
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include <random>

#include "../cql_helpers/answers_by_announcement_or_question_cql_manager.hpp"
#include "../cql_helpers/answers_cql_manager.hpp"
#include "../cql_helpers/courses_by_user_cql_manager.hpp"
#include "../cql_helpers/grades_cql_manager.hpp"
#include "../cql_helpers/questions_by_course_cql_manager.hpp"
#include "../cql_helpers/questions_cql_manager.hpp"
#include "../cql_helpers/school_cql_manager.hpp"
#include "../cql_helpers/tags_by_user_cql_manager.hpp"
#include "../cql_helpers/todos_by_user_cql_manager.hpp"
#include "../cql_helpers/todos_cql_manager.hpp"
#include "../cql_helpers/tokens_cql_manager.hpp"
#include "../cql_helpers/users_by_course_cql_manager.hpp"
#include "../cql_helpers/users_by_tag_cql_manager.hpp"
#include "../cql_helpers/users_cql_manager.hpp"
#include "bcrypt.h"
#include "drogon/drogon.h"

class UserManager {
 public:
  UserManager(UsersCqlManager *usersCqlManager,
              TokensCqlManager *tokensCqlManager,
              SchoolCqlManager *schoolCqlManager,
              UsersByCourseCqlManager *usersByCourseCqlManager,
              CoursesByUserCqlManager *coursesByUserCqlManager,
              TagsByUserCqlManager *tagsByUserCqlManager,
              UsersByTagCqlManager *usersByTagCqlManager,
              TodosByUserCqlManager *todosByUserCqlManager,
              TodosCqlManager *todosCqlManager,
              GradesCqlManager *gradesCqlManager,
              QuestionsCqlManager *questionsCqlManager,
              AnswersCqlManager *answersCqlManager,
              AnswersByAnnouncementOrQuestionCqlManager
                  *answersByAnnouncementOrQuestionCqlManager,
              QuestionsByCourseCqlManager *questionsByCourseCqlManager)
      : _users_cql_manager(usersCqlManager),
        _tokens_cql_manager(tokensCqlManager),
        _school_cql_manager(schoolCqlManager),
        _users_by_course_cql_manager(usersByCourseCqlManager),
        _courses_by_user_cql_manager(coursesByUserCqlManager),
        _users_by_tag_cql_manager(usersByTagCqlManager),
        _tags_by_user_cql_manager(tagsByUserCqlManager),
        _todos_by_user_cql_manager(todosByUserCqlManager),
        _todos_cql_manager(todosCqlManager),
        _grades_cql_manager(gradesCqlManager),
        _questions_cql_manager(questionsCqlManager),
        _answers_cql_manager(answersCqlManager),
        _questions_by_course_cql_manager(questionsByCourseCqlManager),
        _answers_by_announcement_or_question_cql_manager(
            answersByAnnouncementOrQuestionCqlManager) {}
  ~UserManager() {}

  ///
  /// The methods bellow are used to manage the users and the relations
  /// between them and the tokens. They are used to create, read, update and
  /// delete the users and the tokens. They are also used to validate the users
  /// and the tokens. If the user or the token is valid, the method will return
  /// a pair with the HTTP status code 200 or 201 and an empty JSON object. If
  /// the user or the token is not valid, the method will return a pair with
  /// the HTTP status code 400 or 404 and a JSON object containing the error
  /// message. If the user or the token could not be created, read, updated or
  /// deleted due to internal errors, the method will return a pair with the
  /// HTTP status code 500 and a JSON object containing the error message.
  ///

  // User related methods
  std::pair<drogon::HttpStatusCode, Json::Value> create_user(
      const int school_id, const std::string &creator_token,
      const std::string &email, UserType user_type,
      const std::string &first_name, const std::string &last_name,
      const std::string phone_number);
  std::pair<drogon::HttpStatusCode, Json::Value> get_user(
      const int school_id, const std::string &token, const CassUuid &user_id);
  std::pair<drogon::HttpStatusCode, Json::Value> get_all_users(
      const int school_id, const std::string &token);
  std::pair<drogon::HttpStatusCode, Json::Value> update_user(
      const int school_id, const std::string &editor_token,
      const CassUuid &user_id, const std::optional<std::string> &email,
      const std::optional<std::string> &password,
      const std::optional<UserType> user_type,
      const std::optional<std::string> &first_name,
      const std::optional<std::string> &last_name,
      const std::optional<std::string> &phone_number);
  std::pair<drogon::HttpStatusCode, Json::Value> delete_user(
      const int school_id, const std::string &token, const CassUuid &user_id);

  // Token related methods
  std::pair<drogon::HttpStatusCode, Json::Value> log_in(
      const int school_id, const std::string &email,
      const std::string &password);
  std::pair<drogon::HttpStatusCode, Json::Value> log_out(
      const int school_id, const std::string &token);

 private:
  UsersCqlManager *_users_cql_manager;
  TokensCqlManager *_tokens_cql_manager;
  SchoolCqlManager *_school_cql_manager;
  UsersByCourseCqlManager *_users_by_course_cql_manager;
  CoursesByUserCqlManager *_courses_by_user_cql_manager;
  TagsByUserCqlManager *_tags_by_user_cql_manager;
  UsersByTagCqlManager *_users_by_tag_cql_manager;
  TodosByUserCqlManager *_todos_by_user_cql_manager;
  TodosCqlManager *_todos_cql_manager;
  GradesCqlManager *_grades_cql_manager;
  QuestionsCqlManager *_questions_cql_manager;
  AnswersCqlManager *_answers_cql_manager;
  AnswersByAnnouncementOrQuestionCqlManager
      *_answers_by_announcement_or_question_cql_manager;
  QuestionsByCourseCqlManager *_questions_by_course_cql_manager;

  std::string generate_token(const int school_id);
  std::string generate_password();
  bool token_is_unique(const int school, const std::string token);

  std::pair<drogon::HttpStatusCode, Json::Value> delete_relation_courses(
      const int school_id, const CassUuid &user_id);

  std::pair<drogon::HttpStatusCode, Json::Value> delete_relation_tags(
      const int school_id, const CassUuid &user_id);

  std::pair<drogon::HttpStatusCode, Json::Value> delete_relation_todos(
      const int school_id, const CassUuid &user_id);

  std::pair<drogon::HttpStatusCode, Json::Value> delete_relation_grades(
      const int school_id, const CassUuid &user_id, const UserType user_type);

  std::pair<drogon::HttpStatusCode, Json::Value>
  delete_question_and_answers_of_user(const int school_id,
                                      const CassUuid course_id,
                                      const CassUuid user_id);
};

#endif