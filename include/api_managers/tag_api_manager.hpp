/**
 * @file tag_api_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief The purpose of this file is to define the manager of
 * the apis for the tag related objects
 * @version 0.1
 * @date 2023-02-07
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef TAG_API_MANAGER_H
#define TAG_API_MANAGER_H

#include "../relations_managers/tag_manager.hpp"
#include "jwt-cpp/jwt.h"

using namespace drogon;

class TagAPIManager : public HttpController<TagAPIManager, false> {
 public:
  TagAPIManager(TagManager *tag_manager, std::string public_key)
      : _tag_manager(tag_manager), _public_key(public_key) {
    app().registerHandler("/api/tags", &TagAPIManager::create_tag, {Post});
    app().registerHandler("/api/tags", &TagAPIManager::get_tags, {Get});
    app().registerHandler("/api/tags/{tag-id}", &TagAPIManager::get_tag, {Get});
    app().registerHandler("/api/tags/{tag-id}", &TagAPIManager::update_tag,
                          {Put});
    app().registerHandler("/api/tags/{tag-id}", &TagAPIManager::delete_tag,
                          {Delete});

    app().registerHandler("/api/tags/{tag-id}/add_user?user_id={user-id}",
                          &TagAPIManager::add_user_to_tag, {Post});
    app().registerHandler("/api/tags/{tag-id}/users",
                          &TagAPIManager::get_users_by_tag, {Get});
    app().registerHandler("/api/tags/personal_tags?user_id={user-id}",
                          &TagAPIManager::get_tags_by_user, {Get});
    app().registerHandler("/api/tags/{tag-id}/remove_user?user_id={user-id}",
                          &TagAPIManager::remove_user_from_tag, {Delete});
  }
  ~TagAPIManager(){};

  METHOD_LIST_BEGIN
  METHOD_LIST_END

 protected:
  void create_tag(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback);
  void get_tags(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback);
  void get_tag(const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback,
               std::string tag_id);
  void update_tag(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback,
                  std::string tag_id);
  void delete_tag(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback,
                  std::string tag_id);

  void add_user_to_tag(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback,
                       std::string tag_id, std::string user_id);
  void get_users_by_tag(const HttpRequestPtr &req,
                        std::function<void(const HttpResponsePtr &)> &&callback,
                        std::string tag_id);
  void get_tags_by_user(const HttpRequestPtr &req,
                        std::function<void(const HttpResponsePtr &)> &&callback,
                        std::string user_id);
  void remove_user_from_tag(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      std::string tag_id, std::string user_id);

 private:
  TagManager *_tag_manager;
  std::string _public_key;

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

#endif