/**
 * @file course_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief The purpose of this class is to manage the relations related to the
 * course objects in relation with the other objects in the database.
 * @version 0.1
 * @date 2023-02-01
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef COURSE_MANAGER_HPP
#define COURSE_MANAGER_HPP

#include <drogon/drogon.h>

#include "../cql_helpers/answers_by_announcement_or_question_cql_manager.hpp"
#include "../cql_helpers/answers_cql_manager.hpp"
#include "../cql_helpers/courses_by_user_cql_manager.hpp"
#include "../cql_helpers/courses_cql_manager.hpp"
#include "../cql_helpers/files_cql_manager.hpp"
#include "../cql_helpers/grades_cql_manager.hpp"
#include "../cql_helpers/lectures_cql_manager.hpp"
#include "../cql_helpers/questions_by_course_cql_manager.hpp"
#include "../cql_helpers/questions_cql_manager.hpp"
#include "../cql_helpers/tags_cql_manager.hpp"
#include "../cql_helpers/tokens_cql_manager.hpp"
#include "../cql_helpers/users_by_course_cql_manager.hpp"
#include "../cql_helpers/users_by_tag_cql_manager.hpp"
#include "../cql_helpers/users_cql_manager.hpp"

class CourseManager {
 public:
  CourseManager(UsersCqlManager *usersCqlManager,
                TokensCqlManager *tokensCqlManager,
                FilesCqlManager *filesCqlManager,
                GradesCqlManager *gradesCqlManager,
                CoursesCqlManager *coursesCqlManager,
                UsersByCourseCqlManager *usersByCourseCqlManager,
                CoursesByUserCqlManager *coursesByUserCqlManager,
                LecturesCqlManager *lecturesCqlManager,
                TagsCqlManager *tagsCqlManager,
                UsersByTagCqlManager *usersByTagCqlManager,
                QuestionsCqlManager *questionsCqlManager,
                AnswersCqlManager *answersCqlManager,
                AnswersByAnnouncementOrQuestionCqlManager
                    *answersByAnnouncementOrQuestionCqlManager,
                QuestionsByCourseCqlManager *questionsByCourseCqlManager)
      : _users_cql_manager(usersCqlManager),
        _tokens_cql_manager(tokensCqlManager),
        _files_cql_manager(filesCqlManager),
        _grades_cql_manager(gradesCqlManager),
        _courses_cql_manager(coursesCqlManager),
        _users_by_course_cql_manager(usersByCourseCqlManager),
        _courses_by_user_cql_manager(coursesByUserCqlManager),
        _lectures_cql_manager(lecturesCqlManager),
        _tags_cql_manager(tagsCqlManager),
        _users_by_tag_cql_manager(usersByTagCqlManager),
        _questions_cql_manager(questionsCqlManager),
        _answers_cql_manager(answersCqlManager),
        _questions_by_course_cql_manager(questionsByCourseCqlManager),
        _answers_by_announcement_or_question_cql_manager(
            answersByAnnouncementOrQuestionCqlManager) {}
  ~CourseManager(){};

  ///
  /// The methods bellow are used to manage the courses and the relations
  /// between them and the users. They are used to create, read, update and
  /// delete the courses and the relations between them and the users. If the
  /// course or the relation is valid, the method will return a pair with the
  /// HTTP status code 200 or 201 and an empty JSON object. If the course or the
  /// relation is not valid, the method will return a pair with the HTTP status
  /// code 400 or 404 and a JSON object containing the error message. If the
  /// course or the relation could not be created, read, updated or deleted due
  /// to internal errors, the method will return a pair with the HTTP status
  /// code 500 and a JSON object containing the error message.
  ///

  ///
  /// Functions that would be used to manage the courses
  ///

  // Will add the course to the database
  std::pair<drogon::HttpStatusCode, Json::Value> create_course(
      const int school_id, const std::string &creator_token,
      const std::string &name, const time_t start_date, const time_t end_date);
  // Will return the course with the given id (all data)
  std::pair<drogon::HttpStatusCode, Json::Value> get_course(
      const int school_id, const std::string &user_token,
      const CassUuid course_id);
  // Will return all the courses that the user is enrolled in (short course
  // format)
  std::pair<drogon::HttpStatusCode, Json::Value> get_all_user_courses(
      const int school_id, const std::string &user_token);
  // Will return all users that are enrolled in the course
  std::pair<drogon::HttpStatusCode, Json::Value> get_courses_users(
      const int school_id, const CassUuid course_id,
      const std::string &user_token);
  // Update the course information. Not the users enrolled in the course
  // or the files that are attached to the course
  std::pair<drogon::HttpStatusCode, Json::Value> update_course(
      const int school_id, const std::string &user_token,
      const CassUuid course_id, const std::optional<std::string> &title,
      const std::optional<time_t> start_date,
      const std::optional<time_t> end_date);
  // Delete the course from the database and all related content
  std::pair<drogon::HttpStatusCode, Json::Value> delete_course(
      const int school_id, const std::string &user_token,
      const CassUuid course_id);

  ///
  /// Functions that take care of the thumbnail of the course
  ///
  // Will set the thumbnail of the course
  std::pair<drogon::HttpStatusCode, Json::Value> set_course_thumbnail(
      const int school_id, const std::string &user_token,
      const CassUuid course_id, const std::string &file_extension);

  // Will return the thumbnail of the course
  std::pair<drogon::HttpStatusCode, Json::Value> get_course_thumbnail(
      const int school_id, const std::string &user_token,
      const CassUuid course_id);

  // Will delete the thumbnail of the course
  std::pair<drogon::HttpStatusCode, Json::Value> delete_course_thumbnail(
      const int school_id, const std::string &user_token,
      const CassUuid course_id);

  ///
  /// Functions that would be able to manage the courses files
  ///

  // Will add the file to the course (just the fact that the course has the file
  // attached to it, not the file itself)
  std::pair<drogon::HttpStatusCode, Json::Value> create_course_file(
      const int school_id, const std::string &user_token,
      const CassUuid course_id, const std::string &file_name,
      const CustomFileType file_type, std::string file_extension,
      const std::optional<CassUuid> file_owner, const bool visible_to_students,
      const bool students_can_add);

  // Will return the files that are attached to the course
  // not the actual files, just the information about them
  // (name, size, type, creator, students can add)
  /**
   * @brief Get the course files object also containing the file
   * files that are attached to the course
   *
   * @param school_id  - The id of the school
   * @param course_id  - The id of the course
   * @param token      - The user token
   * @return std::pair<drogon::HttpStatusCode, Json::Value>
   */
  std::pair<drogon::HttpStatusCode, Json::Value> get_course_files(
      const int school_id, const CassUuid course_id, const std::string &token);

  // Will update the information about the file that is attached to the course
  std::pair<drogon::HttpStatusCode, Json::Value> update_course_files(
      const int school_id, const std::string &user_token,
      const CassUuid course_id, const CassUuid file_id,
      const std::optional<std::string> file_name,
      const std::optional<std::vector<CassUuid>> file_ids,
      const std::optional<bool> visible_to_students,
      const std::optional<bool> students_can_add);

  /**
   * @brief Deletes the file from the course. Not the actual file, just the
   * information about it.
   *
   * @param school_id      - The id of the school
   * @param user_token     - The token of the user that is sending the request
   * @param course_id      - The id of the course
   * @param file_id        - The id of the file
   * @return std::pair<drogon::HttpStatusCode, Json::Value>
   */
  std::pair<drogon::HttpStatusCode, Json::Value> delete_course_file(
      const int school_id, const std::string &user_token,
      const CassUuid course_id, const CassUuid file_id);

  /**
   * @brief Checks if the user has access to the file.
   *
   * @param school_id  - The id of the school
   * @param user_token - The token of the user that is sending the request
   * @param course_id  - The id of the course
   * @param file_id    - The id of the file
   * @return true      - If the user has access to the file return the path
   * @return false     - If the user does not have access to the file or
   *                    the file does not exist or there was an error :)
   */
  std::pair<drogon::HttpStatusCode, Json::Value> has_permission_to_get_file(
      const int school_id, const std::string &user_token,
      const CassUuid course_id, const CassUuid &file_id);

  ///
  /// Functions that would be able to manage the courses relations with users
  ///

  // Will add the user to the course
  /**
   * @brief Will add the users and the users in tags to the course.
   *      If the user is already in the course, it will be ignored.
   *
   * @param school_id   - The id of the school
   * @param user_token  - The token of the user that is sending the request
   * @param course_id   - The id of the course
   * @param users       - The users that will be added to the course
   * @param tags        - The tags of the users that will be added to the course
   * @return std::pair<drogon::HttpStatusCode, Json::Value>
   */
  std::pair<drogon::HttpStatusCode, Json::Value> add_users(
      const int school_id, const std::string &user_token,
      const CassUuid course_id, const std::vector<CassUuid> &users,
      const std::vector<CassUuid> &tags);

  /**
   * @brief Will remove specfied users and the users in the specified tags from
   * the course. If the user is already in the course, it will be ignored.
   *
   * @param school_id   - The id of the school
   * @param user_token  - The token of the user that is sending the request
   * @param course_id   - The id of the course
   * @param users       - The users that will be removed from the course
   * @param tags        - The tags of the users that will be removed from the
   * course
   * @return std::pair<drogon::HttpStatusCode, Json::Value>
   */
  std::pair<drogon::HttpStatusCode, Json::Value> remove_users(
      const int school_id, const std::string &user_token,
      const CassUuid course_id, const std::vector<CassUuid> &users,
      const std::vector<CassUuid> &tags);

  ///
  /// Functions that will take care of the questions and answers
  ///
  std::pair<drogon::HttpStatusCode, Json::Value> create_question(
      const int school_id, const std::string &user_token,
      const CassUuid course_id, std::string &content);
  std::pair<drogon::HttpStatusCode, Json::Value> get_questions_by_course(
      const int school_id, const std::string &user_token,
      const CassUuid course_id);
  std::pair<drogon::HttpStatusCode, Json::Value> delete_question(
      const int school_id, const std::string &user_token,
      const CassUuid course_id, const CassUuid question_id);
  std::pair<drogon::HttpStatusCode, Json::Value> create_answer(
      const int school_id, const std::string &user_token,
      const CassUuid course_id, const CassUuid question_id,
      const std::string &content);
  std::pair<drogon::HttpStatusCode, Json::Value> get_answers(
      const int school_id, const CassUuid course_id,
      const CassUuid question_id);
  std::pair<drogon::HttpStatusCode, Json::Value> delete_answer(
      const int school_id, const std::string &user_token,
      const CassUuid question_id, const CassUuid answer_id);

 private:
  UsersCqlManager *_users_cql_manager;
  TokensCqlManager *_tokens_cql_manager;
  FilesCqlManager *_files_cql_manager;
  GradesCqlManager *_grades_cql_manager;
  CoursesCqlManager *_courses_cql_manager;
  UsersByCourseCqlManager *_users_by_course_cql_manager;
  CoursesByUserCqlManager *_courses_by_user_cql_manager;
  LecturesCqlManager *_lectures_cql_manager;
  TagsCqlManager *_tags_cql_manager;
  UsersByTagCqlManager *_users_by_tag_cql_manager;
  QuestionsCqlManager *_questions_cql_manager;
  AnswersCqlManager *_answers_cql_manager;
  AnswersByAnnouncementOrQuestionCqlManager
      *_answers_by_announcement_or_question_cql_manager;
  QuestionsByCourseCqlManager *_questions_by_course_cql_manager;

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

  /**
   * @brief Get a list of files (of a folder) in a json format
   *
   * @param school_id  - The id of the school.
   * @param files      - The files that we want to get.
   * @param user_type  - The type of the user.
   * @return std::pair<drogon::HttpStatusCode, Json::Value>
   */
  std::pair<drogon::HttpStatusCode, Json::Value> get_folder_files(
      const int school_id, const std::vector<CassUuid> files,
      const UserType user_type);

  std::pair<drogon::HttpStatusCode, Json::Value>
  delete_questions_and_answers_of_user(const int school_id,
                                       const CassUuid course_id,
                                       const CassUuid user_id);
};

#endif