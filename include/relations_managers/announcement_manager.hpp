/**
 * @file announcement_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief The purpose of this class is to manage the relations related to the
 * announcement objects in relation with the other objects in the database.
 * @version 0.1
 * @date 2023-02-04
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef ANNOUNCEMENT_MANAGER_HPP
#define ANNOUNCEMENT_MANAGER_HPP

#include <drogon/drogon.h>

#include "../cql_helpers/announcements_by_tag_cql_manager.hpp"
#include "../cql_helpers/announcements_cql_manager.hpp"
#include "../cql_helpers/answers_by_announcement_or_question_cql_manager.hpp"
#include "../cql_helpers/answers_cql_manager.hpp"
#include "../cql_helpers/files_cql_manager.hpp"
#include "../cql_helpers/tags_by_user_cql_manager.hpp"
#include "../cql_helpers/tags_cql_manager.hpp"
#include "../cql_helpers/tokens_cql_manager.hpp"
#include "../cql_helpers/users_cql_manager.hpp"

class AnnouncementManager {
 public:
  AnnouncementManager(
      AnnouncementsCqlManager *announcementCqlManager,
      AnnouncementsByTagCqlManager *announcementsByTagCqlManager,
      TagsCqlManager *tagsCqlManager, TokensCqlManager *tokensCqlManager,
      UsersCqlManager *usersCqlManager,
      AnswersByAnnouncementOrQuestionCqlManager
          *answersByAnnouncementOrQuestionCqlManager,
      AnswersCqlManager *answersCqlManager, FilesCqlManager *filesCqlManager,
      TagsByUserCqlManager *tagsByUserCqlManager)
      : _announcements_cql_manager(announcementCqlManager),
        _announcements_by_tag_cql_manager(announcementsByTagCqlManager),
        _tags_cql_manager(tagsCqlManager),
        _tokens_cql_manager(tokensCqlManager),
        _users_cql_manager(usersCqlManager),
        _answers_by_announcement_or_question_cql_manager(
            answersByAnnouncementOrQuestionCqlManager),
        _answers_cql_manager(answersCqlManager),
        _files_cql_manager(filesCqlManager),
        _tags_by_user_cql_manager(tagsByUserCqlManager) {}
  ~AnnouncementManager() {}

  ///
  /// The methods bellow are used to manage the announcements and the relations
  /// between them and the users. They are used to create, read, update and
  /// delete the announcements and the relations between them and the users. If
  /// the announcement or the relation is valid, the method will return a pair
  /// with the HTTP status code 200 or 201 and an empty JSON object. If the
  /// announcement or the relation is not valid, the method will return a pair
  /// with the HTTP status code 400 or 404 and a JSON object containing the
  /// error message. If the announcement or the relation could not be created,
  /// read, updated or deleted due to internal errors, the method will return a
  /// pair with the HTTP status code 500 and a JSON object containing the error
  /// message.
  ///

  ///
  /// Functions that would be used to manage the announcement
  ///

  // Will add the announcement to the database
  std::pair<drogon::HttpStatusCode, Json::Value> create_announcement(
      const int school_id, const std::string &creator_token,
      const std::string &title, const std::string &content,
      const bool allow_answers);
  // Will get the announcements that a user has access to
  std::pair<drogon::HttpStatusCode, Json::Value> get_announcements(
      const int school_id, const std::string &token);
  // Will delete the announcement with the given id
  std::pair<drogon::HttpStatusCode, Json::Value> delete_announcement(
      const int school_id, const std::string &token,
      const CassUuid &announcement_id);

  ///
  /// Functions that add/remove the relation between the announcement and the
  /// tags
  ///

  // Will add the tag to the announcement
  std::pair<drogon::HttpStatusCode, Json::Value> add_tag_to_announcement(
      const int school_id, const std::string &token,
      const CassUuid &announcement_id, const CassUuid &tag_id);
  // Will get the tags that are attached to the announcement
  std::pair<drogon::HttpStatusCode, Json::Value> get_announcement_tags(
      const int school_id, const std::string &token,
      const CassUuid &announcement_id);
  // Will remove the tag from the announcement
  std::pair<drogon::HttpStatusCode, Json::Value> remove_tag_from_announcement(
      const int school_id, const std::string &token,
      const CassUuid &announcement_id, const CassUuid &tag_id);

  ///
  /// Functions that would be able to manage the announcements files
  ///

  // Will add the file to the announcement (just the fact that the announcement
  // has the file attached to it, not the file itself)
  std::pair<drogon::HttpStatusCode, Json::Value> create_announcement_file(
      const int school_id, const std::string &user_token,
      const CassUuid announcement_id, const std::string &file_name,
      std::string file_extension);

  // Deletes the file from the announcement. Not the actual file, just the
  // information about it.
  std::pair<drogon::HttpStatusCode, Json::Value> delete_announcement_file(
      const int school_id, const std::string &user_token,
      const CassUuid announcement_id, const CassUuid file_id);

  /**
   * @brief Checks if the user has access to the file.
   *
   * @param school_id  - The id of the school
   * @param user_token - The token of the user that is sending the request
   * @param announcement_id  - The id of the announcement
   * @param file_id    - The id of the file
   * @return true      - If the user has access to the file return the path
   * @return false     - If the user does not have access to the file or
   *                    the file does not exist or there was an error :)
   */
  std::pair<drogon::HttpStatusCode, Json::Value> has_permission_to_get_file(
      const int school_id, const std::string &user_token,
      const CassUuid announcement_id, const CassUuid &file_id);

  ///
  /// Functions that will take care of the answers
  ///
  std::pair<drogon::HttpStatusCode, Json::Value> create_answer(
      const int school_id, const std::string &user_token,
      const CassUuid announcement_id, const std::string &content);
  std::pair<drogon::HttpStatusCode, Json::Value> get_answers(
      const int school_id, const CassUuid announcement_id);
  std::pair<drogon::HttpStatusCode, Json::Value> delete_answer(
      const int school_id, const std::string &user_token,
      const CassUuid question_id, const CassUuid answer_id);

 private:
  AnnouncementsCqlManager *_announcements_cql_manager;
  AnnouncementsByTagCqlManager *_announcements_by_tag_cql_manager;
  TagsCqlManager *_tags_cql_manager;
  TokensCqlManager *_tokens_cql_manager;
  UsersCqlManager *_users_cql_manager;
  AnswersByAnnouncementOrQuestionCqlManager
      *_answers_by_announcement_or_question_cql_manager;
  AnswersCqlManager *_answers_cql_manager;
  FilesCqlManager *_files_cql_manager;
  TagsByUserCqlManager *_tags_by_user_cql_manager;

  std::pair<drogon::HttpStatusCode, Json::Value> get_user_by_token(
      const int school_id, const std::string &user_token, UserObject &user);

  std::pair<drogon::HttpStatusCode, Json::Value>
  user_has_access_to_announcement(const int school_id, const UserObject &user,
                                  const CassUuid &announcement_id);

  std::pair<drogon::HttpStatusCode, Json::Value> get_file_json(
      const int school_id, const CassUuid &file_id);
};

#endif  // ANNOUNCEMENT_MANAGER_HPP