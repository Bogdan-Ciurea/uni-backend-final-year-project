/**
 * @file tag_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief The purpose of this class is to manage the relations related to the
 * tag objects in relation with the other objects in the database.
 * @version 0.1
 * @date 2023-01-31
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef TAG_MANAGER_H
#define TAG_MANAGER_H

#include "../cql_helpers/tags_by_user_cql_manager.hpp"
#include "../cql_helpers/tags_cql_manager.hpp"
#include "../cql_helpers/tokens_cql_manager.hpp"
#include "../cql_helpers/users_by_tag_cql_manager.hpp"
#include "../cql_helpers/users_cql_manager.hpp"
#include "drogon/drogon.h"

class TagManager {
 public:
  TagManager(TagsCqlManager *tagsCqlManager, UsersCqlManager *usersCqlManager,
             TokensCqlManager *tokensCqlManager,
             UsersByTagCqlManager *usersByTagCqlManager,
             TagsByUserCqlManager *tagsByUserCqlManager)
      : _tags_cql_manager(tagsCqlManager),
        _users_cql_manager(usersCqlManager),
        _tokens_cql_manager(tokensCqlManager),
        _users_by_tag_cql_manager(usersByTagCqlManager),
        _tags_by_user_cql_manager(tagsByUserCqlManager) {}
  ~TagManager() {}

  ///
  /// The methods bellow are used to manage the tags and the relations between
  /// them and the users. They are used to create, read, update and delete the
  /// tags and the relations between them and the users. If the tag or the
  /// relation is valid, the method will return a pair with the HTTP status code
  /// 200 or 201 and an empty JSON object. If the tag or the relation is not
  /// valid, the method will return a pair with the HTTP status code 400 or 404
  /// and a JSON object containing the error message. If the tag or the relation
  /// could not be created, read, updated or deleted due to internal errors, the
  /// method will return a pair with the HTTP status code 500 and a JSON object
  /// containing the error message.
  ///

  // Tag related methods
  std::pair<drogon::HttpStatusCode, Json::Value> create_tag(
      const int school_id, const std::string &creator_token,
      const std::string &value, const std::string &colour);
  std::pair<drogon::HttpStatusCode, Json::Value> get_tag(
      const int school_id, const CassUuid tag_id,
      const std::string &user_token);
  std::pair<drogon::HttpStatusCode, Json::Value> get_all_tags(
      const int school_id, const std::string &user_token);
  std::pair<drogon::HttpStatusCode, Json::Value> update_tag(
      const int school_id, const CassUuid tag_id, const std::string &user_token,
      const std::optional<std::string> &value,
      const std::optional<std::string> &colour);
  std::pair<drogon::HttpStatusCode, Json::Value> delete_tag(
      const int school_id, const CassUuid tag_id,
      const std::string &user_token);

  // Relation between tag and user related methods
  std::pair<drogon::HttpStatusCode, Json::Value> create_tag_user_relation(
      const int school_id, const std::string &user_token, const CassUuid tag_id,
      const CassUuid user_id);
  std::pair<drogon::HttpStatusCode, Json::Value> get_users_by_tag(
      const int school_id, const std::string &user_token,
      const CassUuid tag_id);
  std::pair<drogon::HttpStatusCode, Json::Value> get_tags_by_user(
      const int school_id, const std::string &user_token,
      const std::optional<CassUuid> user_id);
  std::pair<drogon::HttpStatusCode, Json::Value> delete_tag_user_relation(
      const int school_id, const std::string &user_token, const CassUuid tag_id,
      const CassUuid user_id);

 private:
  TagsCqlManager *_tags_cql_manager;
  UsersCqlManager *_users_cql_manager;
  TokensCqlManager *_tokens_cql_manager;
  UsersByTagCqlManager *_users_by_tag_cql_manager;
  TagsByUserCqlManager *_tags_by_user_cql_manager;

  /**
   * @brief Get the user by token object
   *
   * @param school_id  - The id of the school.
   * @param user_token - The token of the user.
   * @param user       - The user object that we want to fill.
   * @return std::pair<drogon::HttpStatusCode, Json::Value>
   */
  std::pair<drogon::HttpStatusCode, Json::Value> get_user_by_token(
      const int school_id, const std::string &user_token, UserObject &user);

  /**
   * @brief Validates the tag value and colour.
   *
   * @param value  - The value of the tag.
   * @param colour - The colour of the tag.
   * @return int   - 0 if the tag is valid, 1 if the value is invalid, 2 if the
   */
  int validate_tag(const std::string &value, const std::string &colour);

  // The allowed colours for the tags.
  const std::vector<std::string> allowed_colours = {
      "whiteAlpha", "blackAlpha", "gray",     "red",      "orange",
      "yellow",     "green",      "teal",     "blue",     "cyan",
      "purple",     "pink",       "linkedin", "facebook", "messenger",
      "whatsapp",   "twitter",    "telegram", "gray"};
};

#endif
