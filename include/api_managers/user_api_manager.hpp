/**
 * @file user_api_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief The purpose of this file is to define the manager of
 * the apis for the user related objects
 * @version 0.1
 * @date 2023-02-12
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef USER_API_MANAGER_H
#define USER_API_MANAGER_H

#include "../relations_managers/user_manager.hpp"
#include "email/email_manager.hpp"
#include "jwt-cpp/jwt.h"

using namespace drogon;

class UserAPIManager : public HttpController<UserAPIManager, false> {
 public:
  UserAPIManager(UserManager *user_manager, std::string private_key,
                 std::string public_key, EmailManager *email_manager)
      : _user_manager(user_manager),
        _private_key(private_key),
        _public_key(public_key) {
    this->email_manager = email_manager;

    app().registerHandler("/api/users", &UserAPIManager::create_user, {Post});
    app().registerHandler("/api/users", &UserAPIManager::get_users, {Get});
    app().registerHandler("/api/users/{user-id}", &UserAPIManager::get_user,
                          {Get});
    app().registerHandler("/api/users/{user-id}", &UserAPIManager::update_user,
                          {Put});
    app().registerHandler("/api/users/{user-id}", &UserAPIManager::delete_user,
                          {Delete});
    app().registerHandler("/api/login", &UserAPIManager::log_in, {Post});
    app().registerHandler("/api/logout", &UserAPIManager::log_out, {Post});
  }
  ~UserAPIManager(){};

  METHOD_LIST_BEGIN
  METHOD_LIST_END

 protected:
  void create_user(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback);
  void get_users(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback);
  void get_user(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback,
                std::string user_id);
  void update_user(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback,
                   std::string user_id);
  void delete_user(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback,
                   std::string user_id);

  void log_in(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback);
  void log_out(const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback);

 private:
  UserManager *_user_manager;
  EmailManager *email_manager;
  std::string _private_key;
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