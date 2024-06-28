/**
 * @file course_api_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief The purpose of this file is to define the manager of
 * the apis for the course related objects
 * @version 0.1
 * @date 2023-02-04
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef COURSE_API_MANAGER_H
#define COURSE_API_MANAGER_H

#include <filesystem>
#include <unordered_map>

#include "../relations_managers/course_manager.hpp"
#include "jwt-cpp/jwt.h"

using namespace drogon;

class CourseAPIManager : public HttpController<CourseAPIManager, false> {
 public:
  CourseAPIManager(CourseManager *course_manager, std::string public_key)
      : _course_manager(course_manager), _public_key(public_key) {
    // For the actual course
    app().registerHandler("/api/courses", &CourseAPIManager::create_course,
                          {Post});
    app().registerHandler("/api/course/{course-id}",
                          &CourseAPIManager::get_course, {Get});
    app().registerHandler("/api/course/{course-id}/users",
                          &CourseAPIManager::get_courses_users, {Get});
    app().registerHandler("/api/user_courses",
                          &CourseAPIManager::get_user_courses, {Get});
    app().registerHandler("/api/course/{course-id}",
                          &CourseAPIManager::update_course, {Put});
    app().registerHandler("/api/course/{course-id}",
                          &CourseAPIManager::delete_course, {Delete});

    // For the course thumbnail
    app().registerHandler("/api/course/{course-id}/thumbnail",
                          &CourseAPIManager::create_course_thumbnail, {Post});
    app().registerHandler(
        "/api/course/{course-id}/"
        "thumbnail?user_token={user-token}",
        &CourseAPIManager::get_course_thumbnail, {Get});
    app().registerHandler("/api/course/{course-id}/thumbnail",
                          &CourseAPIManager::delete_course_thumbnail, {Delete});

    // For the course files
    app().registerHandler("/api/course/{course-id}/files",
                          &CourseAPIManager::create_course_file, {Post});
    app().registerHandler("/api/course/{course-id}/files",
                          &CourseAPIManager::get_course_files, {Get});
    app().registerHandler("/api/course/{course-id}/files?file_id={file-id}",
                          &CourseAPIManager::get_course_file, {Get});
    app().registerHandler("/api/course/{course-id}/files?file_id={file-id}",
                          &CourseAPIManager::update_course_file, {Put});
    app().registerHandler("/api/course/{course-id}/files?file_id={file-id}",
                          &CourseAPIManager::delete_course_file, {Delete});

    // For the course related users
    app().registerHandler("/api/course/{course-id}/users",
                          &CourseAPIManager::add_users_to_course, {Post});
    app().registerHandler("/api/course/{course-id}/users",
                          &CourseAPIManager::remove_users_from_course,
                          {Delete});

    // For questions and answers
    app().registerHandler("/api/course/{course-id}/questions",
                          &CourseAPIManager::create_course_question, {Post});
    app().registerHandler("/api/course/{course-id}/questions",
                          &CourseAPIManager::get_course_questions, {Get});
    app().registerHandler("/api/course/{course-id}/questions/{question-id}",
                          &CourseAPIManager::delete_course_question, {Delete});
    app().registerHandler(
        "/api/course/{course-id}/questions/{question-id}/answers",
        &CourseAPIManager::create_course_answer, {Post});
    app().registerHandler(
        "/api/course/{course-id}/questions/{question-id}/answers/{answer-id}",
        &CourseAPIManager::delete_course_answer, {Delete});
  };

  ~CourseAPIManager(){};
  METHOD_LIST_BEGIN
  METHOD_LIST_END

 protected:
  ///
  /// Functions that would be used to manage the courses
  ///

  void create_course(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);
  void get_course(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback,
                  std::string course_id);
  void get_courses_users(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      std::string course_id);
  void get_user_courses(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback);
  void update_course(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback,
                     std::string course_id);
  void delete_course(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback,
                     std::string course_id);

  ///
  /// Functions that would be able to manage the courses thumbnails
  ///
  void create_course_thumbnail(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      std::string course_id);
  void get_course_thumbnail(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      std::string course_id, std::string user_token);
  void delete_course_thumbnail(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      std::string course_id);

  ///
  /// Functions that would be able to manage the courses files
  ///
  void create_course_file(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      std::string course_id);
  void get_course_files(const HttpRequestPtr &req,
                        std::function<void(const HttpResponsePtr &)> &&callback,
                        std::string course_id);
  void get_course_file(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback,
                       std::string course_id, std::string file_id);
  void update_course_file(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      std::string course_id, std::string file_id);
  void delete_course_file(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      std::string course_id, std::string file_id);

  ///
  /// Functions that would be able to manage the courses users
  ///
  void add_users_to_course(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      std::string course_id);
  void remove_users_from_course(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      std::string course_id);

  ///
  /// Functions that would be able to manage the courses questions and answers
  ///
  void create_course_question(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      std::string course_id);
  void get_course_questions(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      std::string course_id);
  void delete_course_question(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      std::string course_id, std::string question_id);
  void create_course_answer(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      std::string course_id, std::string question_id);
  void delete_course_answer(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      std::string course_id, std::string question_id, std::string answer_id);

 private:
  CourseManager *_course_manager;
  std::string _public_key;

  void send_response(std::function<void(const HttpResponsePtr &)> &&callback,
                     const drogon::HttpStatusCode status_code,
                     const std::string &message);

  bool is_file_name_valid(const std::string file_name);

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

  // Suported mime types (order alphabetically)
  const std::unordered_map<std::string, std::string> _mime_types{
      {".jpg", "image/jpeg"},
      {".jpeg", "image/jpeg"},
      {".png", "image/png"},
      {".gif", "image/gif"},
      {".bmp", "image/bmp"},
      {".txt", "text/plain"},
      {".doc", "application/msword"},
      {".docx",
       "application/"
       "vnd.openxmlformats-officedocument.wordprocessingml.document"},
      {".pdf", "application/pdf"},
      {".xls", "application/vnd.ms-excel"},
      {".xlsx",
       "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
      {".ppt", "application/vnd.ms-powerpoint"},
      {".pptx",
       "application/"
       "vnd.openxmlformats-officedocument.presentationml.presentation"},
      {".html", "text/html"},
      {".htm", "text/html"},
      {".xml", "application/xml"},
      {".mp3", "audio/mpeg"},
      {".wav", "audio/x-wav"},
      {".mp4", "video/mp4"},
      {".mov", "video/quicktime"},
      {".flv", "video/x-flv"},
      {".zip", "application/zip"},
      {".rar", "application/x-rar-compressed"},
      {".tar", "application/x-tar"},
      {".gz", "application/gzip"},
      {".swf", "application/x-shockwave-flash"},
      {".js", "application/javascript"},
      {".css", "text/css"},
      {".rtf", "application/rtf"},
      {".psd", "image/vnd.adobe.photoshop"},
      {".ai", "application/postscript"},
      {".eps", "application/postscript"},
      {".tiff", "image/tiff"},
      {".tif", "image/tiff"},
      {".svg", "image/svg+xml"},
      {".eot", "application/vnd.ms-fontobject"},
      {".ttf", "application/x-font-ttf"},
      {".otf", "application/x-font-otf"},
      {".woff", "application/x-font-woff"},
      {".woff2", "application/x-font-woff2"},
      {".ico", "image/x-icon"},
      {".midi", "audio/midi"},
      {".mid", "audio/midi"},
      {".amr", "audio/amr"},
      {".aif", "audio/x-aiff"},
      {".aiff", "audio/x-aiff"},
      {".m4a", "audio/x-m4a"},
      {".m4v", "video/x-m4v"},
      {".3gp", "video/3gpp"},
      {".3g2", "video/3gpp2"},
      {".ogv", "video/ogg"},
      {".webm", "video/webm"},
      {".mkv", "video/x-matroska"}};
};
#endif  // COURSE_API_MANAGER_H