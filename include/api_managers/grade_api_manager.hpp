/**
 * @file grade_api_manager.cpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief The purpose of this file is to define the manager of
 * the apis for the grades related objects
 * @version 0.1
 * @date 2023-04-08
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef GRADE_API_MANAGER_H
#define GRADE_API_MANAGER_H

#include "../cql_helpers/student_references_cql_manager.hpp"
#include "../relations_managers/grade_manager.hpp"
#include "email/email_manager.hpp"
#include "jwt-cpp/jwt.h"

using namespace drogon;

class GradeAPIManager : public HttpController<GradeAPIManager, false> {
 public:
  GradeAPIManager(GradeManager *grade_manager, std::string public_key,
                  EmailManager *email_manager,
                  StudentReferencesCqlManager *student_references_cql_manager)
      : _grade_manager(grade_manager),
        _public_key(public_key),
        _email_manager(email_manager),
        _student_references_cql_manager(student_references_cql_manager) {
    app().registerHandler("/api/grades", &GradeAPIManager::create_grade,
                          {Post});
    app().registerHandler("/api/grades", &GradeAPIManager::get_grades, {Get});
    app().registerHandler("/api/user/{user-id}/grades",
                          &GradeAPIManager::get_user_grades, {Get});
    app().registerHandler("/api/course/{course-id}/grades",
                          &GradeAPIManager::get_course_grades, {Get});
    app().registerHandler("/api/grades/{grade-id}",
                          &GradeAPIManager::update_grade, {Put});
    app().registerHandler("/api/grades/{grade-id}",
                          &GradeAPIManager::delete_grade, {Delete});
  }
  ~GradeAPIManager(){};

  METHOD_LIST_BEGIN
  METHOD_LIST_END

 private:
  void create_grade(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);
  void get_grades(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback);
  void get_user_grades(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback,
                       std::string user_id);
  void get_course_grades(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      std::string course_id);
  void update_grade(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    std::string grade_id);
  void delete_grade(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    std::string grade_id);

  GradeManager *_grade_manager;
  std::string _public_key;
  EmailManager *_email_manager;
  StudentReferencesCqlManager *_student_references_cql_manager;

  void send_response(std::function<void(const HttpResponsePtr &)> &&callback,
                     const drogon::HttpStatusCode status_code,
                     const std::string &message);

  /**
   * @brief Get the credentials from the authorization header.
   * It will return a pair of the school id and the token.
   * If there was an error, the school id will be -1 and the token will be
   * empty.
   *
   * @param authorization_header - The authorization header.
   * @return std::pair<int, std::string>
   */
  std::pair<int, std::string> get_credentials(
      const std::string authorization_header);
};

#endif  // GRADE_API_MANAGER_H