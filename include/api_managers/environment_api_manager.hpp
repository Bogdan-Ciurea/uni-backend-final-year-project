/**
 * @file environment_api_manager.hpp
 * @author Bogdan-Alexandru Ciurea (sc20bac@leeds.ac.uk)
 * @brief The purpose of this file is to define the manager of
 * the apis for the environment related objects
 * @version 0.1
 * @date 2023-01-18
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef ENVIRONMENT_API_MANAGER_H
#define ENVIRONMENT_API_MANAGER_H

#include <drogon/drogon.h>

#include "../relations_managers/environment_manager.hpp"

using namespace drogon;

class EnvironmentAPIManager
    : public HttpController<EnvironmentAPIManager, false> {
 public:
  EnvironmentAPIManager(EnvironmentManager *environment_manager)
      : _environment_manager(environment_manager) {
    // School related endpoints
    app().registerHandler("/api/environment/school",
                          &EnvironmentAPIManager::create_school, {Post});
    app().registerHandler("/api/environment/school?id={school-id}",
                          &EnvironmentAPIManager::get_school, {Get});
    app().registerHandler("/api/environment/school?id={school-id}",
                          &EnvironmentAPIManager::update_school, {Put});
    app().registerHandler("/api/environment/school?id={school-id}",
                          &EnvironmentAPIManager::delete_school, {Delete});

    // Country related endpoints
    app().registerHandler("/api/environment/country",
                          &EnvironmentAPIManager::create_country, {Post});
    app().registerHandler("/api/environment/country?id={country-id}",
                          &EnvironmentAPIManager::get_country, {Get});
    app().registerHandler("/api/environment/country?id={country-id}",
                          &EnvironmentAPIManager::update_country, {Put});
    app().registerHandler("/api/environment/country?id={country-id}",
                          &EnvironmentAPIManager::delete_country, {Delete});

    // Holiday related endpoints
    app().registerHandler("/api/environment/holidays?school_id={school-id}",
                          &EnvironmentAPIManager::create_holiday, {Post});
    app().registerHandler("/api/environment/holidays?school_id={school-id}",
                          &EnvironmentAPIManager::get_holidays, {Get});
    app().registerHandler(
        "/environment/holidays?school_id={school-id}&date={date}",
        &EnvironmentAPIManager::delete_holiday, {Delete});
  };
  ~EnvironmentAPIManager(){};
  METHOD_LIST_BEGIN
  METHOD_LIST_END

 protected:
  // School related methods
  void create_school(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);
  void get_school(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback,
                  int school_id);
  void get_all_schools(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback);
  void update_school(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback,
                     int school_id);
  void delete_school(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback,
                     int school_id);

  // Country related methods
  void create_country(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback);
  void get_country(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback,
                   int country_id);
  void get_all_countries(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback);
  void update_country(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback,
                      int country_id);
  void delete_country(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback,
                      int country_id);

  // Holiday related methods
  void create_holiday(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback,
                      int school_id);
  void get_holidays(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    int school_id);
  void delete_holiday(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback,
                      int school_id, time_t date);

 private:
  EnvironmentManager *_environment_manager;

  void send_response(std::function<void(const HttpResponsePtr &)> &&callback,
                     const drogon::HttpStatusCode status_code,
                     const std::string &message);
};

#endif